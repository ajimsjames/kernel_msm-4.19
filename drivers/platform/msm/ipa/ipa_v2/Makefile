# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/drivers/platform/msm/ipa/ipa_v2 
CFLAGS_ipa_debugfs.o := -Wno-error
obj-$(CONFIG_IPA) += ipat.o
CFLAGS_ipa_debugfs.o := -Wno-error
ipat-y := ipa.o ipa_debugfs.o ipa_hdr.o ipa_flt.o ipa_rt.o ipa_dp.o ipa_client.o \
	ipa_utils.o ipa_nat.o ipa_intf.o teth_bridge.o ipa_interrupts.o \
	ipa_uc.o ipa_uc_wdi.o ipa_dma.o ipa_uc_mhi.o ipa_mhi.o ipa_uc_ntn.o \
	ipa_wdi3_i.o

obj-$(CONFIG_RMNET_IPA) += rmnet_ipa.o ipa_qmi_service_v01.o ipa_qmi_service.o rmnet_ipa_fd_ioctl.o
