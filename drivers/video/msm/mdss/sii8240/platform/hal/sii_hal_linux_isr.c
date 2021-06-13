/**********************************************************************************
 * Si8240 Linux Driver
 *
 * Copyright (C) 2011-2012 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **********************************************************************************/

/**
 * @file sii_hal_linux_isr.c
 *
 * @brief Linux implementation of interrupt support used by Silicon Image
 *        MHL devices.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 31, 2011
 *
 *****************************************************************************/

#define SII_HAL_LINUX_ISR_C

/***** #include statements ***************************************************/
#include "sii_hal.h"
#include "sii_hal_priv.h"
#include "si_c99support.h"
#include "si_osdebug.h"
#include <linux/slab.h>
#include <linux/wakelock.h>


#if defined(__KERNEL__)
//#define DEBUG
#if defined(DEBUG)
#define DEBUG_PRINT(str, args...) printk("sii_hal_linux_isr.c:%d, "str, __LINE__, args)
#else
#define DEBUG_PRINT(str, args...)
#endif
#else
#define DEBUG_PRINT(str, args...)
#endif

/***** local macro definitions ***********************************************/

/***** local type definitions ************************************************/

/***** local variable declarations *******************************************/

/***** local function prototypes *********************************************/

/***** global variable declarations *******************************************/
extern uint8_t	fwPowerState ;
int i2c_state = 3;//MSM_I2C_SYS_SUSPENDED
static int gMhlWakeLockInitilized;

#define imx51_board 1
#if imx51_board
#include <linux/irq.h>
extern void SiiMhlTxDeviceIsr( void );

struct work_struct	*sii8240work;
static spinlock_t sii8240_lock;//= SPIN_LOCK_UNLOCKED;
static DEFINE_SPINLOCK(sii8240_lock);

void mhl_wait_timeout(int count)
{
	if (gMhlWakeLockInitilized)
		wake_lock_timeout(&gMhlDevice.mhl_wakelock, HZ);
 	while(count--)
 	{ 		
	 	if (i2c_state<2)//MSM_I2C_SYS_SUSPENDING
	 	{
	 		break;
	 	}
		msleep(10);
 	}
}

static void work_queue(struct work_struct *work)
{	
	// if ((3 == fwPowerState)||(0xff==fwPowerState))
	 {
		mhl_wait_timeout(300);
	 }
	//printk(KERN_INFO "%s:%d::::::::Sii8240 interrupt happened\n", __func__,__LINE__);
	SiiMhlTxDeviceIsr();
	enable_irq(gMhlDevice.pI2cClient->irq);	
	
}

static irqreturn_t Sii8240_mhl_interrupt(int irq, void *dev_id)
{
	unsigned long lock_flags = 0;	 
	disable_irq_nosync(irq);
	spin_lock_irqsave(&sii8240_lock, lock_flags);	
	//printk("The sii8240 interrupt handeler is working..\n");  
	//printk("The most of sii8240 interrupt work will be done by following tasklet..\n");  
	schedule_work(sii8240work);

	//printk("The sii8240 interrupt's top_half has been done and bottom_half will be processed..\n");  
	spin_unlock_irqrestore(&sii8240_lock, lock_flags);
	return IRQ_HANDLED;
}
#else

/***** local functions *******************************************************/

/*****************************************************************************/
/*
 *  @brief Interrupt handler for MHL transmitter interrupts.
 *
 *  @param[in]		irq		The number of the asserted IRQ line that caused
 *  						this handler to be called.
 *  @param[in]		data	Data pointer passed when the interrupt was enabled,
 *  						which in this case is a pointer to the
 *  						MhlDeviceContext of the I2c device.
 *
 *  @return     Always returns IRQ_HANDLED.
 *
 *****************************************************************************/
static irqreturn_t HalThreadedIrqHandler(int irq, void *data)
{
	pMhlDeviceContext	pMhlDevContext = (pMhlDeviceContext)data;

	DEBUG_PRINT("HalThreadedIrqHandler called %s\n", " ");
	if (HalAcquireIsrLock() == HAL_RET_SUCCESS) {
		(pMhlDevContext->irqHandler)();
		HalReleaseIsrLock();
	}

	return IRQ_HANDLED;
}
#endif

/***** public functions ******************************************************/


/*****************************************************************************/
/**
 * @brief Install IRQ handler.
 *
 *****************************************************************************/
halReturn_t HalInstallIrqHandler(fwIrqHandler_t irqHandler)
{
	int				retStatus;
	halReturn_t 	halRet;

	DEBUG_PRINT("InstallIrqHandler: Enter %s \n", " ");

	if(irqHandler == NULL)
	{
		DEBUG_PRINT("HalInstallIrqHandler: irqHandler cannot be NULL! %s\n", "");
		return HAL_RET_PARAMETER_ERROR;
	}

	halRet = I2cAccessCheck();
	if (halRet != HAL_RET_SUCCESS)
	{
		return halRet;
	}

	if(gMhlDevice.pI2cClient->irq == 0)
	{
		DEBUG_PRINT("HalInstallIrqHandler: No IRQ assigned to I2C device!%s\n", "");
		return HAL_RET_FAILURE;
	}

	gMhlDevice.irqHandler = irqHandler;

	
#if imx51_board
	   sii8240work = kmalloc(sizeof(*sii8240work), GFP_ATOMIC);
	   INIT_WORK(sii8240work, work_queue); 
	   wake_lock_init(&gMhlDevice.mhl_wakelock,WAKE_LOCK_SUSPEND, "mhl_input_wakelock");	
	   gMhlWakeLockInitilized = 1;


	   retStatus = request_irq(gMhlDevice.pI2cClient->irq, Sii8240_mhl_interrupt,
										IRQ_TYPE_LEVEL_LOW,//IRQF_TRIGGER_LOW| IRQF_ONESHOT,
										gMhlI2cIdTable[0].name,
										&gMhlDevice);
	     enable_irq_wake(gMhlDevice.pI2cClient->irq);

#else
	   retStatus = request_threaded_irq(gMhlDevice.pI2cClient->irq, NULL,
										HalThreadedIrqHandler,
										IRQF_TRIGGER_LOW | IRQF_ONESHOT,
										gMhlI2cIdTable[0].name,
										&gMhlDevice);
#endif
	
	if(retStatus != 0)
	{
		DEBUG_PRINT("HalInstallIrqHandler: request_threaded_irq failed, status: %d\n", retStatus);
		gMhlDevice.irqHandler = NULL;
		return HAL_RET_FAILURE;
	}

	return HAL_RET_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Remove IRQ handler.
 *
 *****************************************************************************/
halReturn_t HalRemoveIrqHandler(void)
{
	halReturn_t 	halRet;

	halRet = I2cAccessCheck();
	if (halRet != HAL_RET_SUCCESS)
	{
		return halRet;
	}

	if(gMhlDevice.irqHandler == NULL)
	{
		DEBUG_PRINT("HalRemoveIrqHandler: no irqHandler installed!%s\n", "");
		return HAL_RET_FAILURE;
	}
	wake_lock_destroy(&gMhlDevice.mhl_wakelock);
	gMhlWakeLockInitilized = 0;
	free_irq(gMhlDevice.pI2cClient->irq, &gMhlDevice);

	gMhlDevice.irqHandler = NULL;

	return HAL_RET_SUCCESS;
}

