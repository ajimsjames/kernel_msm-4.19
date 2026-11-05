#ifndef _MDP3_DMA_H_
#define _MDP3_DMA_H_

struct mdp3_notification {
	void (*handler)(void *arg);
	void *arg;
};

#endif

/* Compatibility aliases for 4.19 dss_io_util */
#include <linux/mdss_io_util.h>

#define mdss_module_power dss_module_power
#define mdss_vreg dss_vreg
#define mdss_gpio dss_gpio
#define mdss_clk dss_clk
#define mdss_io_data dss_io_data

#define msm_mdss_enable_vreg msm_dss_enable_vreg
#define msm_mdss_config_vreg msm_dss_config_vreg
#define msm_mdss_enable_gpio msm_dss_enable_gpio
#define msm_mdss_gpio_enable msm_dss_gpio_enable
#define msm_mdss_ioremap_byname msm_dss_ioremap_byname
#define msm_mdss_iounmap msm_dss_iounmap

