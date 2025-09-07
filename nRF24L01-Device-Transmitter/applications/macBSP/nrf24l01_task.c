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

/* 前向声明一下nrf24l01的事件回调句柄 */
const static struct nrf24_callback g_cb;
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

    /* 0. 给nrf24开创一个实际空间 */
    nrf24_t nrf24 = malloc(sizeof(nrf24_t));
    if (nrf24 == NULL) {
        LOG_E("LOG:%d. nrf24 malloc error.",Record.ulog_cnt++);
    }
    else{
        LOG_I("LOG:%d. nrf24 malloc successful.",Record.ulog_cnt++);
    }


    /* 1. 创建二值信号量 */
    nrf24_send_sem = rt_sem_create("nrf24_send", 0, RT_IPC_FLAG_FIFO);
    if(nrf24_send_sem == RT_NULL){
        LOG_E("Failed to create nrf24l01 send semaphore.");
    }
    else{
        LOG_I("Succeed to create nrf24l01 send semaphore.");
    }

    nrf24_irq_sem = rt_sem_create("nrf24_irq", 0, RT_IPC_FLAG_FIFO);
    if(nrf24_irq_sem == RT_NULL){
        LOG_E("Failed to create nrf24l01 irq semaphore.");
    }
    else{
        LOG_I("Succeed to create nrf24l01 irq semaphore.");
        nrf24->nrf24_flags.using_irq = RT_TRUE;
    }


    /* 2. 获取中断引脚编号 */
    nrf24->port_api.nRF24L01_IRQ_Pin_Num = GET_PIN(C, 5);


    /* 3. 初始化SPI */
    nRF24L01_SPI_Init(&nrf24->port_api);


    /* 4. 把spi底层函数整体拷贝到ops结构体中 */
    nrf24->nrf24_ops = g_nrf24_func_ops;
    nrf24->nrf24_cb  = g_cb;

    /* 5. 配置nRF24L01的参数*/
    if(nRF24L01_Param_Config(&nrf24->nrf24_cfg) != RT_EOK){
        LOG_E("LOG:%d. nrf24 parameter config error.",Record.ulog_cnt++);
    }
    else{
        LOG_I("LOG:%d. nrf24 parameter config successfully.",Record.ulog_cnt++);
    }

    /* 6. 配置启用中断引脚和中断回调函数 */
    if(nRF24L01_IQR_GPIO_Config(&nrf24->port_api) != RT_EOK){
        LOG_E("LOG:%d. nrf24 irq config error.",Record.ulog_cnt++);
    }
    else{
        LOG_I("LOG:%d. nrf24 irq config successfully.",Record.ulog_cnt++);
    }


    /* 7. 通过回环通信，检测SPI硬件链路是否有误 */
    if (nRF24L01_Check_SPI_Community(nrf24) != RT_EOK){
        LOG_E("LOG:%d. nRF24L01 check spi hardware false.",Record.ulog_cnt++);
    }
    else{
        LOG_I("LOG:%d. nRF24L01 check spi hardware successful.",Record.ulog_cnt++);
    }


    /* 8. 先进入掉电模式 */
    nRF24L01_Enter_Power_Down_Mode(nrf24);
    nrf24->nrf24_ops.nrf24_reset_ce();

    /* 9. 解锁高级扩展功能 */
    nRF24L01_Ensure_RWW_Features_Activated(nrf24);

    /* 10. 更新寄存器参数 */
    if (nRF24L01_Update_Parameter(nrf24) != RT_EOK){
        LOG_E("LOG:%d. nRF24L01 update_onchip_config false.",Record.ulog_cnt++);
    }
    else{
        LOG_I("LOG:%d. nRF24L01 update_onchip_config successful.",Record.ulog_cnt++);
    }

    /* 11. 读取寄存器参数 */
    if (nRF24L01_Read_Onchip_Parameter(nrf24) != RT_EOK){
        LOG_E("LOG:%d. nRF24L01 read parameter false.",Record.ulog_cnt++);
    }
    else{
        LOG_I("LOG:%d. nRF24L01 read parameter successful.",Record.ulog_cnt++);
    }

    /* 12. 清空"发送/接收"队列 */
    nRF24L01_Flush_RX_FIFO(nrf24);
    nRF24L01_Flush_TX_FIFO(nrf24);
    /* 13. 清除中断标志*/
    nRF24L01_Clear_IRQ_Flags(nrf24);
    /* 14. 清除重发计数和丢包计数*/
    nRF24L01_Clear_Observe_TX(nrf24);
    /* 15. 配置完成，进入上电模式 */
    nRF24L01_Enter_Power_Up_Mode(nrf24);

    nrf24->nrf24_ops.nrf24_set_ce();
    LOG_I("LOG:%d. Successfully initialized",Record.ulog_cnt++);
    rt_kprintf("\r\n\r\n");
    rt_kprintf("----------------------------------\r\n");
    rt_kprintf("[nrf24/demo] running.\r\n");


    nRF24L01_Send_Packet(nrf24, (uint8_t *)"Hi\r\n", 4, NRF24_DEFAULT_PIPE);

    for(;;)
    {
        nRF24L01_Run(nrf24);

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










const static char *ROLE_TABLE[] = {"PTX", "PRX"};
static void nrf24l01_tx_done(nrf24_t nrf24, rt_uint8_t pipe)
{
    /*! Here just want to tell the user when the role is ROLE_PTX
        the pipe have no special meaning except indicating (send) FAILED or OK
        However, it will matter when the role is ROLE_PRX*/

    static int cnt = 0;
    static char tbuf[32];
    cnt++;

    if(nrf24->nrf24_cfg.config.prim_rx == ROLE_PTX)
    {
        if(pipe == NRF24_PIPE_NONE){
            rt_kprintf("tx_done failed");
        }
        else{
            rt_kprintf("tx_done ok");
        }
    }
    else
    {
        rt_kprintf("tx_done ok");
    }

    rt_kprintf(" (pipe%d)\n", pipe);
    rt_sprintf(tbuf, "My role is %s [%dth]\n", ROLE_TABLE[nrf24->nrf24_cfg.config.prim_rx], cnt);
    nRF24L01_Send_Packet(nrf24, (uint8_t *)tbuf,  rt_strlen(tbuf), pipe);
}




static void nrf24l01_rx_ind(nrf24_t nrf24, uint8_t *data, uint8_t len, int pipe)
{
    /*! Don't need to care the pipe if the role is ROLE_PTX */
    rt_kprintf("(p%d): ", pipe);
    rt_kprintf((char *)data);
}



const static struct nrf24_callback g_cb = {
    .nrf24l01_rx_ind = nrf24l01_rx_ind,
    .nrf24l01_tx_done = nrf24l01_tx_done,
};








