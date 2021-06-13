/***********************************************************************************/
/* File Name: MHL_SiI8334.c */
/* File Description: this file is used to make sii8334 driver to be added in kernel or module. */

/*  Copyright (c) 2002-2010, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/kthread.h>

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>

#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/spinlock_types.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "MHL_SiI8334.h"
#include "si_mhl_tx_api.h"
#include "si_mhl_tx.h"
#include "si_drv_mhl_tx.h"
#include "si_mhl_defs.h"
//yichangming

#include <linux/regulator/consumer.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/usb/msm_hsusb.h>

#define COMPATIBLE_NAME "qcom,mhl-sii8334"
//interrupt mode or polling mode for 8334 driver (if you want to use polling mode, pls comment below sentense)
#define SiI8334DRIVER_INTERRUPT_MODE   1


#define MHL_DRIVER_NAME "sii8334drv"

#define MHL_DRIVER_MINOR_MAX   1
#define EVENT_POLL_INTERVAL_30_MS	30
unsigned res_gpio;
unsigned int_gpio;

struct mhl_tx_ctrl *mhl_ctrl = NULL;
/***** public type definitions ***********************************************/

typedef struct {
	struct task_struct	*pTaskStruct;
	uint8_t				pendingEvent;		// event data wait for retrieval
	uint8_t				pendingEventData;	// by user mode application

} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;


/***** global variables ********************************************/

MHL_DRIVER_CONTEXT_T gDriverContext;


//struct platform_data {
//	void (*reset) (void);
//};

//static struct platform_data *Sii8334_plat_data;


bool_t	vbusPowerState = true;		// false: 0 = vbus output on; true: 1 = vbus output off;
//yichangming
//bool mhl_is_connected(void)
//{
//	return true;
//}
/*  USB_HANDSHAKING FUNCTIONS */




static bool_t match_id(const struct i2c_device_id *id, const struct i2c_client *client)
{
	if (strcmp(client->name, id->name) == 0)
		return true;

	return false;
}


static void Sii8334_mhl_reset(struct i2c_client *client)
{
	static int bFirst=1;
	int rc;
	static struct regulator *reg_8941_l12, *reg_8941_s4;
	if(bFirst)
	{
		if (!reg_8941_l12) {
			
			reg_8941_l12 = regulator_get(&client->dev, "avcc_12");
			if (IS_ERR(reg_8941_l12)) 
				pr_err("could not get reg_8941_l12, rc = %ld\n",PTR_ERR(reg_8941_l12));

			rc = regulator_set_voltage(reg_8941_l12, 1200000, 1200000);
			if (rc) 
				pr_err("set_voltage failed for 8921_l12, rc=%d\n", rc);

		}
			
		if (!reg_8941_s4) {
			
			reg_8941_s4 = regulator_get(&client->dev, "avcc_18");
			if (IS_ERR(reg_8941_s4)) 
				pr_err("could not get reg_8941_s4, rc = %ld\n",PTR_ERR(reg_8941_s4));
			
			rc = regulator_set_voltage(reg_8941_s4, 1800000, 1800000);
			if (rc) 
				pr_err("set_voltage failed for 8941_s4, rc=%d\n", rc);
			
		}
		
		printk("%s,power on!\n", __func__);
		
		rc = regulator_enable(reg_8941_l12);
		if (rc) 
			pr_err("%s: regulator_enable of reg_8941_l12 failed(%d)\n", __func__, rc);
		
		rc = regulator_enable(reg_8941_s4);
		if (rc) 
			pr_err("%s: regulator_enable of reg_8941_s4 failed(%d)\n",__func__, rc);

		bFirst = 0;
	}	
	msleep(10);
	gpio_direction_output(res_gpio, 1);
	msleep(10);
	gpio_direction_output(res_gpio, 0);
	msleep(10);
	gpio_direction_output(res_gpio, 1);



}

//------------------------------------------------------------------------------
// Function:    HalTimerWait
// Description: Waits for the specified number of milliseconds, using timer 0.
//------------------------------------------------------------------------------

void HalTimerWait ( uint16_t ms )
{
	msleep(ms);
}

/*************************************RCP function report added by garyyuan*********************************/
struct input_dev *rmt_input=NULL;

void mhl_init_rmt_input_dev(void)
{
	int error;
	printk(KERN_INFO "%s:%d:.................................................\n", __func__,__LINE__);

	rmt_input = input_allocate_device();	
    rmt_input->name = "mhl_rcp";
	
    set_bit(EV_KEY,rmt_input->evbit);
    set_bit(KEY_SELECT, rmt_input->keybit);
    set_bit(KEY_UP, rmt_input->keybit);
    set_bit(KEY_DOWN, rmt_input->keybit);
    set_bit(KEY_LEFT, rmt_input->keybit);
    set_bit(KEY_RIGHT, rmt_input->keybit);

    set_bit(KEY_MENU, rmt_input->keybit);

    set_bit(KEY_EXIT, rmt_input->keybit);
	
    set_bit(KEY_NUMERIC_0, rmt_input->keybit);
    set_bit(KEY_NUMERIC_1,rmt_input->keybit);
    set_bit(KEY_NUMERIC_2, rmt_input->keybit);
    set_bit(KEY_NUMERIC_3, rmt_input->keybit);
    set_bit(KEY_NUMERIC_4, rmt_input->keybit);
    set_bit(KEY_NUMERIC_5, rmt_input->keybit);
    set_bit(KEY_NUMERIC_6,rmt_input->keybit);
    set_bit(KEY_NUMERIC_7, rmt_input->keybit);
    set_bit(KEY_NUMERIC_8, rmt_input->keybit);
    set_bit(KEY_NUMERIC_9, rmt_input->keybit);
	
    set_bit(KEY_ENTER, rmt_input->keybit);
    set_bit(KEY_CLEAR, rmt_input->keybit);
  
    set_bit(KEY_PLAY, rmt_input->keybit);
    set_bit(KEY_STOP, rmt_input->keybit);
    set_bit(KEY_PAUSE, rmt_input->keybit);
    
    set_bit(KEY_REWIND, rmt_input->keybit);
    set_bit(KEY_FASTFORWARD, rmt_input->keybit);
    set_bit(KEY_EJECTCD, rmt_input->keybit);
    set_bit(KEY_FORWARD, rmt_input->keybit);
    set_bit(KEY_BACK, rmt_input->keybit);
   	error = input_register_device(rmt_input);
   	if (error) {
		printk("Failed to register input device, err: %d\n", error);
	}
}

void input_report_rcp_key(uint8_t rcp_keycode, int up_down)
{
    rcp_keycode &= 0x7F;
    switch ( rcp_keycode )
    {
    case MHL_RCP_CMD_SELECT:
        input_report_key(rmt_input, KEY_SELECT, up_down);
        TX_DEBUG_PRINT(( "\nSelect received\n\n" ));
        break;
    case MHL_RCP_CMD_UP:
        input_report_key(rmt_input, KEY_UP, up_down);
        TX_DEBUG_PRINT(( "\nUp received\n\n" ));
        break;
    case MHL_RCP_CMD_DOWN:
        input_report_key(rmt_input, KEY_DOWN, up_down);
        TX_DEBUG_PRINT(( "\nDown received\n\n" ));
        break;
    case MHL_RCP_CMD_LEFT:
        input_report_key(rmt_input, KEY_LEFT, up_down);
        TX_DEBUG_PRINT(( "\nLeft received\n\n" ));
        break;
    case MHL_RCP_CMD_RIGHT:
        input_report_key(rmt_input, KEY_RIGHT, up_down);
        TX_DEBUG_PRINT(( "\nRight received\n\n" ));
        break;
    case MHL_RCP_CMD_ROOT_MENU:
        input_report_key(rmt_input, KEY_MENU, up_down);
        TX_DEBUG_PRINT(( "\nRoot Menu received\n\n" ));
        break;
    case MHL_RCP_CMD_EXIT:
        input_report_key(rmt_input, KEY_EXIT, up_down);
        TX_DEBUG_PRINT(( "\nExit received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_0:
        input_report_key(rmt_input, KEY_NUMERIC_0, up_down);
        TX_DEBUG_PRINT(( "\nNumber 0 received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_1:
        input_report_key(rmt_input, KEY_NUMERIC_1, up_down);
        TX_DEBUG_PRINT(( "\nNumber 1 received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_2:
        input_report_key(rmt_input, KEY_NUMERIC_2, up_down);
        TX_DEBUG_PRINT(( "\nNumber 2 received\n\n" ));
        break;	
    case MHL_RCP_CMD_NUM_3:
        input_report_key(rmt_input, KEY_NUMERIC_3, up_down);
        TX_DEBUG_PRINT(( "\nNumber 3 received\n\n" ));
        break;	
    case MHL_RCP_CMD_NUM_4:
        input_report_key(rmt_input, KEY_NUMERIC_4, up_down);
        TX_DEBUG_PRINT(( "\nNumber 4 received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_5:
        input_report_key(rmt_input, KEY_NUMERIC_5, up_down);
        TX_DEBUG_PRINT(( "\nNumber 5 received\n\n" ));
        break;	
    case MHL_RCP_CMD_NUM_6:
        input_report_key(rmt_input, KEY_NUMERIC_6, up_down);
        TX_DEBUG_PRINT(( "\nNumber 6 received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_7:
        input_report_key(rmt_input, KEY_NUMERIC_7, up_down);
        TX_DEBUG_PRINT(( "\nNumber 7 received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_8:
        input_report_key(rmt_input, KEY_NUMERIC_8, up_down);
        TX_DEBUG_PRINT(( "\nNumber 8 received\n\n" ));
        break;
    case MHL_RCP_CMD_NUM_9:
        input_report_key(rmt_input, KEY_NUMERIC_9, up_down);
        TX_DEBUG_PRINT(( "\nNumber 9 received\n\n" ));
        break;
    case MHL_RCP_CMD_DOT:
        input_report_key(rmt_input, KEY_DOT, up_down);
        TX_DEBUG_PRINT(( "\nDot received\n\n" ));
        break;
    case MHL_RCP_CMD_ENTER:
        input_report_key(rmt_input, KEY_ENTER, up_down);
        TX_DEBUG_PRINT(( "\nEnter received\n\n" ));
        break;
    case MHL_RCP_CMD_CLEAR:
        input_report_key(rmt_input, KEY_CLEAR, up_down);
        TX_DEBUG_PRINT(( "\nClear received\n\n" ));
        break;
    case MHL_RCP_CMD_SOUND_SELECT:
        input_report_key(rmt_input, KEY_SOUND, up_down);
        TX_DEBUG_PRINT(( "\nSound Select received\n\n" ));
        break;
    case MHL_RCP_CMD_PLAY:
        input_report_key(rmt_input, KEY_PLAY, up_down);
        TX_DEBUG_PRINT(( "\nPlay received\n\n" ));
        break;
    case MHL_RCP_CMD_PAUSE:
        input_report_key(rmt_input, KEY_PAUSE, up_down);
        TX_DEBUG_PRINT(( "\nPause received\n\n" ));
        break;
    case MHL_RCP_CMD_STOP:
        input_report_key(rmt_input, KEY_STOP, up_down);
        TX_DEBUG_PRINT(( "\nStop received\n\n" ));
        break;
    case MHL_RCP_CMD_FAST_FWD:
        input_report_key(rmt_input, KEY_FASTFORWARD, up_down);
        TX_DEBUG_PRINT(( "\nFastfwd received\n\n" ));
        break;
    case MHL_RCP_CMD_REWIND:
        input_report_key(rmt_input, KEY_REWIND, up_down);
        TX_DEBUG_PRINT(( "\nRewind received\n\n" ));
        break;
    case MHL_RCP_CMD_EJECT:
        input_report_key(rmt_input, KEY_EJECTCD, up_down);
        TX_DEBUG_PRINT(( "\nEject received\n\n" ));
        break;
    case MHL_RCP_CMD_FWD:
        input_report_key(rmt_input, KEY_FORWARD, up_down);
        TX_DEBUG_PRINT(( "\nForward received\n\n" ));
        break;
    case MHL_RCP_CMD_BKWD:
        input_report_key(rmt_input, KEY_BACK, up_down);
        TX_DEBUG_PRINT(( "\nBackward received\n\n" ));
        break;
    case MHL_RCP_CMD_PLAY_FUNC:
        //input_report_key(rmt_input, KEY_PL, up_down);
		input_report_key(rmt_input, KEY_PLAY, up_down);
        TX_DEBUG_PRINT(( "\nPlay Function received\n\n" ));
    break;
    case MHL_RCP_CMD_PAUSE_PLAY_FUNC:
        input_report_key(rmt_input, KEY_PLAYPAUSE, up_down);
        TX_DEBUG_PRINT(( "\nPause_Play Function received\n\n" ));
        break;
    case MHL_RCP_CMD_STOP_FUNC:
        input_report_key(rmt_input, KEY_STOP, up_down);
        TX_DEBUG_PRINT(( "\nStop Function received\n\n" ));
        break;
    default:
        break;
    }	
		
    //added  for improving mhl RCP start
    input_sync(rmt_input);
    //added  for improving mhl RCP end
}
void input_report_mhl_rcp_key(uint8_t rcp_keycode)
{
    //added  for improving mhl RCP start
    input_report_rcp_key(rcp_keycode & 0x7F, 1);
    input_report_rcp_key(rcp_keycode & 0x7F, 0);
    //added  for improving mhl RCP end
}

/*************************************RCP function report added by garyyuan*********************************/


#if (VBUS_POWER_CHK == ENABLE)
///////////////////////////////////////////////////////////////////////////////
//
// AppVbusControl
//
// This function or macro is invoked from MhlTx driver to ask application to
// control the VBUS power. If powerOn is sent as non-zero, one should assume
// peer does not need power so quickly remove VBUS power.
//
// if value of "powerOn" is 0, then application must turn the VBUS power on
// within 50ms of this call to meet MHL specs timing.
//
// Application module must provide this function.
///////////////////////////////////////////
void	AppVbusControl( bool_t powerOn )
{
	if( powerOn )
	{
		MHLSinkOrDonglePowerStatusCheck();
		printk("App: Peer's POW bit is set. Turn the VBUS power OFF here.\n");
	}
	else
	{
		printk("App: Peer's POW bit is cleared. Turn the VBUS power ON here.\n");
	}
}
#endif

#ifdef SiI8334DRIVER_INTERRUPT_MODE

struct work_struct	*sii8334work;
static spinlock_t sii8334_lock ;
extern uint8_t	fwPowerState;

static void work_queue(struct work_struct *work)
{	

	//for(Int_count=0;Int_count<15;Int_count++){
		printk(KERN_INFO "%s:%d:Int_count=::::::Sii8334 interrupt happened\n", __func__,__LINE__);
		
		SiiMhlTxDeviceIsr();
	enable_irq(sii8334_PAGE_TPI->irq);
}

static irqreturn_t Sii8334_mhl_interrupt(int irq, void *dev_id)
{
	unsigned long lock_flags = 0;	 
	disable_irq_nosync(irq);
	spin_lock_irqsave(&sii8334_lock, lock_flags);	
	//printk("The sii8334 interrupt handeler is working..\n");  
	printk("The most of sii8334 interrupt work will be done by following tasklet..\n");  

	schedule_work(sii8334work);

	//printk("The sii8334 interrupt's top_half has been done and bottom_half will be processed..\n");  
	spin_unlock_irqrestore(&sii8334_lock, lock_flags);
	return IRQ_HANDLED;
}
#else
/*****************************************************************************/
/**
 *  @brief Thread function that periodically polls for MHLTx events.
 *
 *  @param[in]	data	Pointer to driver context structure
 *
 *  @return		Always returns zero when the thread exits.
 *
 *****************************************************************************/
static int EventThread(void *data)
{
	printk("%s EventThread starting up\n", MHL_DRIVER_NAME);

	while (true)
	{
		if (kthread_should_stop())
		{
			printk("%s EventThread exiting\n", MHL_DRIVER_NAME);
			break;
		}

		HalTimerWait(EVENT_POLL_INTERVAL_30_MS);
		SiiMhlTxDeviceIsr();
	}
	return 0;
}


/***** public functions ******************************************************/


/*****************************************************************************/
/**
 * @brief Start drivers event monitoring thread.
 *
 *****************************************************************************/
void StartEventThread(void)
{
	gDriverContext.pTaskStruct = kthread_run(EventThread,
											 &gDriverContext,
											 MHL_DRIVER_NAME);
}



/*****************************************************************************/
/**
 * @brief Stop driver's event monitoring thread.
 *
 *****************************************************************************/
void  StopEventThread(void)
{
	kthread_stop(gDriverContext.pTaskStruct);

}
#endif

static struct i2c_device_id mhl_Sii8334_idtable[] = {
	{"sii8334_PAGE_TPI", 0},
	{"sii8334_PAGE_TX_L0", 0},
	{"sii8334_PAGE_TX_L1", 0},
	{"sii8334_PAGE_TX_2", 0},
	{"sii8334_PAGE_TX_3", 0},
	{"sii8334_PAGE_CBUS", 0},
	{},
};


MODULE_DEVICE_TABLE(i2c, mhl_Sii8334_idtable);

static int mhl_tx_get_dt_data(struct device *dev)
{

	struct device_node *of_node = NULL;
	struct device_node *hdmi_tx_node = NULL;
	struct platform_device *hdmi_pdev = NULL;
	int rc = 0;
	
	if (!dev ) {
		pr_err("%s: invalid input\n", __func__);
		return -EINVAL;
	}

	of_node = dev->of_node;
	if (!of_node) {
		pr_err("%s: invalid of_node\n", __func__);
	}

	pr_debug("%s: id=%d\n", __func__, dev->id);

	res_gpio= of_get_named_gpio(of_node, "mhl-rst-gpio", 0);
	if(res_gpio){
		rc = gpio_request(res_gpio, "MHL_RESET");
		if (rc) 
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n", "MHL_RESET", res_gpio, rc);
	}

	int_gpio= of_get_named_gpio(of_node, "mhl-intr-gpio", 0);
	if(int_gpio){
		rc = gpio_request(int_gpio, "MHL_INTR");
		if (rc) 
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n", "MHL_INTR", int_gpio, rc);
	}

		/* parse phandle for hdmi tx */
	hdmi_tx_node = of_parse_phandle(of_node, "qcom,hdmi-tx-map", 0);
	if (!hdmi_tx_node) {
		pr_err("%s: can''t find hdmi phandle\n", __func__);
	}

	hdmi_pdev = of_find_device_by_node(hdmi_tx_node);
	if (!hdmi_pdev) {
		pr_err("%s: can't find the device by node\n", __func__);
		return -1;
	}
	pr_debug("%s: hdmi_pdev [0X%x] to pdata->pdev\n",
	       __func__, (unsigned int)hdmi_pdev);
	
	mhl_ctrl->hdmi_pdev = hdmi_pdev;
	return 0;

} /* mhl_tx_get_dt_data */
static int mhl_sii_wait_for_rgnd(struct mhl_tx_ctrl *mhl_ctrl)
{
	int timeout;
	/* let isr handle RGND interrupt */
	pr_debug("%s:%u\n", __func__, __LINE__);
	INIT_COMPLETION(mhl_ctrl->rgnd_done);
	timeout = wait_for_completion_interruptible_timeout
		(&mhl_ctrl->rgnd_done, HZ/2);
	if (!timeout) {
		/* most likely nothing plugged in USB */
		/* USB HOST connected or already in USB mode */
		pr_warn("%s:%u timedout\n", __func__, __LINE__);
		return -ENODEV;
	}
	return mhl_ctrl->mhl_mode ? 0 : 1;
}
/*  USB_HANDSHAKING FUNCTIONS */
#define	POWER_STATE_D3				3
static int mhl_sii_device_discovery(void *data, int id,
			      void (*usb_notify_cb)(void *, int), void *ctx)
{
	int rc;
	struct mhl_tx_ctrl *mhl_ctrl = data;

	if (id) {
		/* When MHL cable is disconnected we get a sii8334
		 * mhl_disconnect interrupt which is handled separately.
		 */
		pr_debug("%s: USB ID pin high\n", __func__);
		return id;
	}

	if (!mhl_ctrl || !usb_notify_cb) {
		pr_warn("%s: cb || ctrl is NULL\n", __func__);
		/* return "USB" so caller can proceed */
		return -EINVAL;
	}

	if (!mhl_ctrl->notify_usb_online){
		mhl_ctrl->notify_usb_online = usb_notify_cb;
		mhl_ctrl->notify_ctx = ctx;
	}


	if (*(mhl_ctrl->cur_state) == POWER_STATE_D3) {
			rc = mhl_sii_wait_for_rgnd(mhl_ctrl);
	} else {
			/* in MHL mode */
			pr_debug("%s:%u\n", __func__, __LINE__);
			rc = 0;
	}

	pr_debug("%s: ret result: %s\n", __func__, rc ? "usb" : " mhl");
	return rc;
}
static int mhl_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	struct mhl_tx_ctrl *mhl_ctrl =
		container_of(psy, struct mhl_tx_ctrl, mhl_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = mhl_ctrl->current_val;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = mhl_ctrl->vbus_active;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = mhl_ctrl->vbus_active && mhl_ctrl->mhl_mode;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int mhl_power_set_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  const union power_supply_propval *val)
{
	struct mhl_tx_ctrl *mhl_ctrl =
		container_of(psy, struct mhl_tx_ctrl, mhl_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		mhl_ctrl->vbus_active = val->intval;
		if (mhl_ctrl->vbus_active)
			mhl_ctrl->current_val = MAX_CURRENT;
		else
			mhl_ctrl->current_val = 0;
		power_supply_changed(psy);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static char *mhl_pm_power_supplied_to[] = {
	"usb",
};

static enum power_supply_property mhl_pm_power_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};
/*
 * i2c client ftn.
 */
static int __devinit mhl_Sii8334_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	struct msm_hdmi_mhl_ops *hdmi_mhl_ops = NULL;
	struct usb_ext_notification *mhl_info = NULL;
	int ret = 0;

	if(match_id(&mhl_Sii8334_idtable[0], client))
	{
		sii8334_PAGE_TPI = client;
		dev_info(&client->adapter->dev, "attached %s "
			"into i2c adapter successfully\n", dev_id->name);
		
		mhl_ctrl = devm_kzalloc(&client->dev, sizeof(struct mhl_tx_ctrl), GFP_KERNEL);
		if (!mhl_ctrl) {
			pr_err("%s: FAILED: cannot alloc hdmi tx ctrl\n", __func__);
			return -ENOMEM;
		}
		
		ret = mhl_tx_get_dt_data(&client->dev);
		if (ret) {
			pr_err("%s: FAILED: parsing device tree data; ret=%d\n",
				__func__, ret);
		}
		sii8334_PAGE_TPI->irq = gpio_to_irq(int_gpio);
	}
	/*
	else if(match_id(&mhl_Sii8334_idtable[1], client))
	{
		sii8334_PAGE_TX_L0 = client;
		dev_info(&client->adapter->dev, "attached %s "
			"into i2c adapter successfully \n", dev_id->name);
	}
	*/
	else if(match_id(&mhl_Sii8334_idtable[2], client))
	{
		sii8334_PAGE_TX_L1 = client;
		dev_info(&client->adapter->dev, "attached %s "
			"into i2c adapter successfully \n", dev_id->name);

	}
	else if(match_id(&mhl_Sii8334_idtable[3], client))
	{
		sii8334_PAGE_TX_2 = client;
		dev_info(&client->adapter->dev, "attached %s "
			"into i2c adapter successfully\n", dev_id->name);
	}
	else if(match_id(&mhl_Sii8334_idtable[4], client))
	{
		sii8334_PAGE_TX_3 = client;
		dev_info(&client->adapter->dev, "attached %s "
			"into i2c adapter successfully\n", dev_id->name);
	}
	else if(match_id(&mhl_Sii8334_idtable[5], client))
	{
		sii8334_PAGE_CBUS = client;
		dev_info(&client->adapter->dev, "attached %s "
			"into i2c adapter successfully\n", dev_id->name);
	}
	else
	{
		dev_info(&client->adapter->dev, "invalid i2c adapter: can not found dev_id matched\n");
		return -EIO;
	}


	if(sii8334_PAGE_TPI != NULL 
		//&&sii8334_PAGE_TX_L0 != NULL 
		&&sii8334_PAGE_TX_L1 != NULL 
		&& sii8334_PAGE_TX_2 != NULL
		&& sii8334_PAGE_TX_3 != NULL
		&& sii8334_PAGE_CBUS != NULL)
	{
		// Announce on RS232c port.
		//
		printk("\n============================================\n");
		printk("SiI-8334 Driver Version based on 8051 driver Version 1.0066 \n");
		printk("============================================\n");

		if(!mhl_ctrl){
			pr_err("%s: alloc mhl_ctrl failed\n", __func__);
			return -ENOMEM;
		}
		hdmi_mhl_ops = devm_kzalloc(&sii8334_PAGE_TPI->dev,
				    sizeof(struct msm_hdmi_mhl_ops),
				    GFP_KERNEL);
		if (!hdmi_mhl_ops) {
			pr_err("%s: alloc hdmi mhl ops failed\n", __func__);
			return -ENOMEM;
		}

		if (mhl_ctrl->hdmi_pdev) {
			ret = msm_hdmi_register_mhl(mhl_ctrl->hdmi_pdev,
					  	 hdmi_mhl_ops, NULL);
			if (ret) 
				pr_err("%s: register with hdmi failed\n", __func__);
		}

		if (!hdmi_mhl_ops || !hdmi_mhl_ops->tmds_enabled ||
	   		 !hdmi_mhl_ops->set_mhl_max_pclk) {
			pr_err("%s: func ptr is NULL\n", __func__);
		}
		mhl_ctrl->hdmi_mhl_ops = hdmi_mhl_ops;
		ret = hdmi_mhl_ops->set_mhl_max_pclk(mhl_ctrl->hdmi_pdev, MAX_MHL_PCLK);
		if (ret) {
			pr_err("%s: can't set max mhl pclk\n", __func__);
		}

		mhl_ctrl->mhl_mode = false;
		mhl_ctrl->mhl_psy.name = "ext-vbus";
		mhl_ctrl->mhl_psy.type = POWER_SUPPLY_TYPE_USB_DCP;
		mhl_ctrl->mhl_psy.supplied_to = mhl_pm_power_supplied_to;
		mhl_ctrl->mhl_psy.num_supplicants = ARRAY_SIZE(
					mhl_pm_power_supplied_to);
		mhl_ctrl->mhl_psy.properties = mhl_pm_power_props;
		mhl_ctrl->mhl_psy.num_properties = ARRAY_SIZE(mhl_pm_power_props);
		mhl_ctrl->mhl_psy.get_property = mhl_power_get_property;
		mhl_ctrl->mhl_psy.set_property = mhl_power_set_property;

		ret = power_supply_register(&client->dev, &mhl_ctrl->mhl_psy);
		if (ret < 0) {
			dev_err(&client->dev, "%s:power_supply_register ext_vbus_psy failed\n",
				__func__);
			return -EPROBE_DEFER;
		}
		init_completion(&mhl_ctrl->rgnd_done);

		mhl_info = devm_kzalloc(&sii8334_PAGE_TPI->dev, sizeof(*mhl_info), GFP_KERNEL);
		if (!mhl_info) {
			pr_err("%s: alloc mhl info failed\n", __func__);
			return -ENOMEM;
		}
		mhl_info->ctxt = mhl_ctrl;
		mhl_info->notify = mhl_sii_device_discovery;
		if (msm_register_usb_ext_notification(mhl_info)) {
			pr_err("%s: register for usb notifcn failed\n", __func__);
			return  -EPROBE_DEFER;

		}
		mhl_ctrl->mhl_info = mhl_info;
		mhl_ctrl->cur_state = &fwPowerState;
		
		Sii8334_mhl_reset(sii8334_PAGE_TPI);

		//
		// Initialize the registers as required. Setup firmware vars.
		//

		
		//for RCP report function by garyyuan
		mhl_init_rmt_input_dev();		

		SiiMhlTxInitialize();
	
		#ifdef SiI8334DRIVER_INTERRUPT_MODE
		sii8334work = kmalloc(sizeof(*sii8334work), GFP_ATOMIC);
		INIT_WORK(sii8334work, work_queue); 
		spin_lock_init(&sii8334_lock);
	
		ret = request_irq(sii8334_PAGE_TPI->irq, Sii8334_mhl_interrupt, IRQ_TYPE_LEVEL_LOW,
					  sii8334_PAGE_TPI->name, sii8334_PAGE_TPI);

		if (ret)
			printk(KERN_INFO "%s:%d:Sii8334 interrupt failed\n", __func__,__LINE__);	
		//	free_irq(irq, iface);
		else{
			enable_irq_wake(sii8334_PAGE_TPI->irq);	
			printk(KERN_INFO "%s:%d:Sii8334 interrupt successed\n", __func__,__LINE__);	
			}
		#else
		StartEventThread();		/* begin monitoring for events if using polling mode*/
		#endif
	}
	return ret;
}

static int mhl_Sii8334_remove(struct i2c_client *client)
{	
	struct i2c_client *data = i2c_get_clientdata(client);
		
	i2c_set_clientdata(client, NULL);
	kfree(data);
	dev_info(&client->adapter->dev, "detached %s from i2c adapter successfully\n",client->name);
	
	return 0;
}

static struct of_device_id mhl_match_table[] = {
	{.compatible = COMPATIBLE_NAME,},
	{ },
};

static struct i2c_driver mhl_sii_i2c_driver = {
	.driver = {
		.name = MHL_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mhl_match_table,
	},
	.probe = mhl_Sii8334_probe,
	.remove =  mhl_Sii8334_remove,
	.id_table = mhl_Sii8334_idtable,
};

module_i2c_driver(mhl_sii_i2c_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MHL SII 8334 TX Driver");


