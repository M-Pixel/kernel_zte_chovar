#ifndef __MHL_SII8240_H__
#define __MHL_SII8240_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include "../mdss_hdmi_mhl.h"
#include <linux/power_supply.h>

/* MHL 8334 supports a max HD pixel clk of 75 MHz */
#define MAX_MHL_PCLK 150000
#define MAX_CURRENT 700000
struct mhl_tx_ctrl {
	struct platform_device *hdmi_pdev;
	struct msm_hdmi_mhl_ops *hdmi_mhl_ops;
	struct input_dev *input;
	u16 *rcp_key_code_tbl;
	size_t rcp_key_code_tbl_len;
	uint8_t dwnstream_hpd;
	int mhl_mode;
	uint8_t * cur_state;
	struct completion rgnd_done;
	void (*notify_usb_online)(void *, int);
	void *notify_ctx;
	struct usb_ext_notification *mhl_info;
	struct power_supply mhl_psy;
	bool vbus_active;
	int current_val;
};
#endif