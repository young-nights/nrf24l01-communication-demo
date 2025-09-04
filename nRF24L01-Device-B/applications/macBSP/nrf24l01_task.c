/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-02     Administrator       the first version
 */
#include "bsp_sys.h"
#include <rtdbg.h>
#include "bsp_nrf24l01_driver.h"

/* 创建nRF24L01发送数据的二值信号量 */
rt_sem_t nrf24_send_sem = RT_NULL;
/* 创建nRF24L01进入中断的二值信号量 */
rt_sem_t nrf24_irq_sem = RT_NULL;

/**
  * @brief  This thread entry is used for nRF24L01
  * @retval void
  */
void nRF24L01_Thread_entry(void* parameter)
{
    /*0. 创建初始化结构体 */
    nrf24_send_sem = rt_sem_create("nrf24_send", 0, RT_IPC_FLAG_FIFO);
    if(nrf24_send_sem == RT_NULL){
        LOG_E("Failed to create nrf24l01 send semaphore. \r\n");
    }
    else{
        LOG_I("Succeed to create nrf24l01 send semaphore. \r\n");
    }

    nrf24_irq_sem = rt_sem_create("nrf24_irq", 0, RT_IPC_FLAG_FIFO);
    if(nrf24_irq_sem != RT_EOK){
        LOG_E("Failed to create nrf24l01 irq semaphore. \r\n");
    }
    else{
        LOG_I("Succeed to create nrf24l01 irq semaphore. \r\n");
//        nrf24->nrf24_flags.using_irq = RT_TRUE;
    }



    for(;;)
    {
        rt_thread_mdelay(50);
    }
}



/**
  * @brief  This is a Initialization for nRF24L01
  * @retval int
  */
rt_thread_t nRF24L01_Task_Handle = RT_NULL;
int nRF24L01_Thread_Init(void)
{
    nRF24L01_Task_Handle = rt_thread_create("nRF24L01_Thread_entry", nRF24L01_Thread_entry, RT_NULL, 4096, 9, 50);
    /* 检查是否创建成功,成功就启动线程 */
    if(nRF24L01_Task_Handle != RT_NULL)
    {
        LOG_I("[nRF24L01]nRF24L01_Thread_entry is Succeed!! \r\n");
        rt_thread_startup(nRF24L01_Task_Handle);
    }
    else {
        LOG_E("[nRF24L01]nRF24L01_Thread_entry is Failed \r\n");
    }

    return RT_EOK;
}
INIT_APP_EXPORT(nRF24L01_Thread_Init);




























