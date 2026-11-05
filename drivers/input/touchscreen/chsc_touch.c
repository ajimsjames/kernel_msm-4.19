/*
 * Chipsemi CHSC Capacitive Touchscreen Driver for Cat S22 Flip
 * Single touch slot pipeline with zero-drop release filter
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>

#define CHSC_DRIVER_NAME		"semi_touch"
#define CHSC_DEFAULT_WIDTH		480
#define CHSC_DEFAULT_HEIGHT		640
#define CHSC_MAX_FINGERS		1
/* Slot range must be >0 or Android treats device as MT protocol A */
#define CHSC_NUM_SLOTS			2

#define CHSC_REG_BOOT_STATE		0x20000018
#define CHSC_REG_POINT_EVENT		0x2000002C

struct chsc_touch_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	int irq_gpio;
	int rst_gpio;
	int irq;
	u32 max_x;
	u32 max_y;
	bool finger_down;
	u16 last_x;
	u16 last_y;
	struct mutex bus_lock;
	struct regulator *vdd;
	struct regulator *vio;
};

/*
 * ChipSemi CHSC requires a STOP condition after the 4-byte address write.
 * Stock kernel explicitly performs two separate i2c_transfer calls of 1 message each.
 */
static int chsc_i2c_read_bytes(struct chsc_touch_data *ts, u32 reg, u8 *buf, u16 len)
{
	int ret;
	u8 reg_buf[4];
	struct i2c_msg msg_write;
	struct i2c_msg msg_read;

	reg_buf[0] = (reg >> 24) & 0xFF;
	reg_buf[1] = (reg >> 16) & 0xFF;
	reg_buf[2] = (reg >> 8) & 0xFF;
	reg_buf[3] = reg & 0xFF;

	msg_write.addr = ts->client->addr;
	msg_write.flags = 0;
	msg_write.len = 4;
	msg_write.buf = reg_buf;

	msg_read.addr = ts->client->addr;
	msg_read.flags = I2C_M_RD;
	msg_read.len = len;
	msg_read.buf = buf;

	mutex_lock(&ts->bus_lock);

	ret = i2c_transfer(ts->client->adapter, &msg_write, 1);
	if (ret != 1) {
		mutex_unlock(&ts->bus_lock);
		return (ret < 0) ? ret : -EIO;
	}

	ret = i2c_transfer(ts->client->adapter, &msg_read, 1);
	if (ret != 1) {
		mutex_unlock(&ts->bus_lock);
		return (ret < 0) ? ret : -EIO;
	}

	mutex_unlock(&ts->bus_lock);
	return 0;
}

static void chsc_hw_reset(struct chsc_touch_data *ts)
{
	if (gpio_is_valid(ts->rst_gpio)) {
		gpio_direction_output(ts->rst_gpio, 0);
		msleep(20);
		gpio_direction_output(ts->rst_gpio, 1);
		msleep(30);
	}
}

static irqreturn_t chsc_touch_irq_thread(int irq, void *dev_id)
{
	struct chsc_touch_data *ts = dev_id;
	u8 buf[28];
	int ret;
	u8 finger_count;
	u16 r1;
	u16 event_flag;
	u16 x, y;
	u8 z;

	ret = chsc_i2c_read_bytes(ts, CHSC_REG_POINT_EVENT, buf, sizeof(buf));
	if (ret < 0) {
		return IRQ_HANDLED;
	}

	/* Validate packet header */
	if (buf[0] != 0xFF)
		return IRQ_HANDLED;

	finger_count = buf[1] & 0x0F;

	if (finger_count == 0) {
		/* All fingers up */
		if (ts->finger_down) {
			input_mt_slot(ts->input_dev, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
			input_report_key(ts->input_dev, BTN_TOUCH, 0);
			input_sync(ts->input_dev);
			ts->finger_down = false;
		}
		return IRQ_HANDLED;
	}

	r1 = (u16)buf[5] | ((u16)buf[6] << 8);
	event_flag = r1 & 0xF000;

	/*
	 * Release packet: event_flag == 0x4000.
	 * When releasing, controller sends 0 coordinates or release flag.
	 * Never report coordinates on release!
	 */
	if (event_flag == 0x4000) {
		if (ts->finger_down) {
			input_mt_slot(ts->input_dev, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
			input_report_key(ts->input_dev, BTN_TOUCH, 0);
			input_sync(ts->input_dev);
			ts->finger_down = false;
		}
		return IRQ_HANDLED;
	}

	x = (u16)buf[2] | (((u16)r1 & 0x000F) << 8);
	y = (u16)buf[3] | (((u16)r1 & 0x00F0) << 4);
	z = buf[4];

	/* Ignore zero coordinate glitch packet if finger is down */
	if (x == 0 && y == 0)
		return IRQ_HANDLED;

	/* Clamp coordinates */
	if (x > ts->max_x) x = ts->max_x;
	if (y > ts->max_y) y = ts->max_y;
	if (z == 0) z = 128;

	input_mt_slot(ts->input_dev, 0);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
	input_report_abs(ts->input_dev, ABS_MT_PRESSURE, z);
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
	input_sync(ts->input_dev);

	ts->finger_down = true;
	ts->last_x = x;
	ts->last_y = y;

	return IRQ_HANDLED;
}

static int chsc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	struct chsc_touch_data *ts;
	int ret;
	u8 boot_buf[16];
	int retry;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(dev, "I2C functionality check failed\n");
		return -ENODEV;
	}

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->client = client;
	ts->finger_down = false;
	mutex_init(&ts->bus_lock);
	i2c_set_clientdata(client, ts);

	ts->max_x = CHSC_DEFAULT_WIDTH;
	ts->max_y = CHSC_DEFAULT_HEIGHT;

	/* Regulators */
	ts->vdd = devm_regulator_get_optional(dev, "vdd");
	if (!IS_ERR(ts->vdd)) {
		ret = regulator_enable(ts->vdd);
		if (ret)
			dev_warn(dev, "Failed to enable vdd: %d\n", ret);
	}

	ts->vio = devm_regulator_get_optional(dev, "vio");
	if (!IS_ERR(ts->vio)) {
		ret = regulator_enable(ts->vio);
		if (ret)
			dev_warn(dev, "Failed to enable vio: %d\n", ret);
	}

	/* GPIO configuration */
	if (np) {
		ts->irq_gpio = of_get_named_gpio(np, "chipsemi,int-gpio", 0);
		ts->rst_gpio = of_get_named_gpio(np, "chipsemi,rst-gpio", 0);
	} else {
		ts->irq_gpio = 65;
		ts->rst_gpio = 64;
	}

	dev_info(dev, "CHSC touch probe: int_gpio=%d, rst_gpio=%d, addr=0x%02x\n",
		 ts->irq_gpio, ts->rst_gpio, client->addr);

	if (gpio_is_valid(ts->rst_gpio)) {
		ret = devm_gpio_request_one(dev, ts->rst_gpio, GPIOF_OUT_INIT_HIGH, "chsc_rst_pin");
		if (ret) {
			dev_err(dev, "Failed to request rst gpio: %d\n", ret);
			return ret;
		}
	}

	if (gpio_is_valid(ts->irq_gpio)) {
		ret = devm_gpio_request_one(dev, ts->irq_gpio, GPIOF_IN, "chsc_int_pin");
		if (ret) {
			dev_err(dev, "Failed to request int gpio: %d\n", ret);
			return ret;
		}
		ts->irq = gpio_to_irq(ts->irq_gpio);
	} else {
		ts->irq = client->irq;
	}

	/* Reset chip */
	chsc_hw_reset(ts);

	/* Check boot state */
	for (retry = 0; retry < 5; retry++) {
		ret = chsc_i2c_read_bytes(ts, CHSC_REG_BOOT_STATE, boot_buf, 4);
		if (ret == 0) {
			dev_info(dev, "CHSC boot state: %02x %02x %02x %02x\n",
				 boot_buf[0], boot_buf[1], boot_buf[2], boot_buf[3]);
			break;
		}
		msleep(20);
	}

	/* Allocate and configure input device */
	ts->input_dev = devm_input_allocate_device(dev);
	if (!ts->input_dev) {
		dev_err(dev, "Failed to allocate input device\n");
		return -ENOMEM;
	}

	ts->input_dev->name = "chsc_cap_touch";
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->dev.parent = dev;

	__set_bit(EV_KEY, ts->input_dev->evbit);
	__set_bit(EV_ABS, ts->input_dev->evbit);
	__set_bit(BTN_TOUCH, ts->input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->max_x, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

	ret = input_mt_init_slots(ts->input_dev, CHSC_NUM_SLOTS, INPUT_MT_DIRECT);
	if (ret) {
		dev_err(dev, "Failed to init MT slots: %d\n", ret);
		return ret;
	}

	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(dev, "Failed to register input device: %d\n", ret);
		return ret;
	}

	/* Register threaded IRQ */
	ret = devm_request_threaded_irq(dev, ts->irq, NULL, chsc_touch_irq_thread,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"chsc_cap_touch", ts);
	if (ret) {
		dev_err(dev, "Failed to request threaded IRQ %d: %d\n", ts->irq, ret);
		return ret;
	}

	dev_info(dev, "CHSC capacitive touch driver probed successfully (irq=%d)\n", ts->irq);
	return 0;
}

static int chsc_remove(struct i2c_client *client)
{
	struct chsc_touch_data *ts = i2c_get_clientdata(client);

	if (ts->vdd && !IS_ERR(ts->vdd))
		regulator_disable(ts->vdd);
	if (ts->vio && !IS_ERR(ts->vio))
		regulator_disable(ts->vio);

	return 0;
}

static const struct i2c_device_id chsc_id[] = {
	{ CHSC_DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, chsc_id);

static const struct of_device_id chsc_of_match[] = {
	{ .compatible = "chipsemi,chsc_cap_touch", },
	{ }
};
MODULE_DEVICE_TABLE(of, chsc_of_match);

static struct i2c_driver chsc_driver = {
	.driver = {
		.name = CHSC_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = chsc_of_match,
	},
	.probe = chsc_probe,
	.remove = chsc_remove,
	.id_table = chsc_id,
};

module_i2c_driver(chsc_driver);

MODULE_AUTHOR("Antigravity Pair Programmer");
MODULE_DESCRIPTION("Chipsemi CHSC Capacitive Touchscreen Driver");
MODULE_LICENSE("GPL v2");
