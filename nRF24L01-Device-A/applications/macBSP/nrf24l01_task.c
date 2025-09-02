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
#include <bsp_nrf24l01_spi.h>
#include <bsp_nrf24l01_driver.h>

#define NRF24_DEMO_SPI_DEV_NAME "spi1"
#define PKG_NRF24L01_VERSION    "latest"
#define NRF24_CE_PIN            17
#define NRF24_IRQ_PIN           37


const static char *ROLE_TABLE[] = {"PTX", "PRX"};

static void rx_ind(nrf24_t nrf24, uint8_t *data, uint8_t len, int pipe)
{
    /*! Don't need to care the pipe if the role is ROLE_PTX */
    rt_kprintf("(p%d): ", pipe);
    rt_kprintf((char *)data);
}

static void tx_done(nrf24_t nrf24, int pipe)
{
    static int cnt = 0;
    static char tbuf[32];

    cnt++;

    /*! Here just want to tell the user when the role is ROLE_PTX
    the pipe have no special meaning except indicating (send) FAILED or OK
        However, it will matter when the role is ROLE_PRX*/
    if (nrf24->cfg.role == ROLE_PTX)
    {
        if (pipe == NRF24_PIPE_NONE)
            rt_kprintf("tx_done failed");
        else
            rt_kprintf("tx_done ok");
    }
    else
    {
        rt_kprintf("tx_done ok");
    }

    rt_kprintf(" (pipe%d)\n", pipe);

    rt_sprintf(tbuf, "My role is %s [%dth]\n", ROLE_TABLE[nrf24->cfg.role], cnt);
    nrf24_send_data(nrf24, (uint8_t *)tbuf, rt_strlen(tbuf), pipe);
#ifdef PKG_NRF24L01_DEMO_ROLE_PTX
    rt_thread_mdelay(NRF24_DEMO_SEND_INTERVAL);
#endif
}

const static struct nrf24_callback _cb = {
    .rx_ind = rx_ind,
    .tx_done = tx_done,
};


static void nRF24L01_entry(void *param)
{
    // 初始化一个nrf24l01的结构体实例
    nrf24_t nrf24;
    // 打印版本信息
    rt_kprintf("[nrf24/demo] Version:%s\n", PKG_NRF24L01_VERSION);
    /***
     *  按照默认参数进行nrf24的实例创建
     *  NRF24_DEMO_SPI_DEV_NAME : spi1
     *  NRF24_CE_PIN            : PB1
     *  NRF24_IRQ_PIN           : PC5
     *  _cb                     : 回调函数结构体指针首地址
     *  ROLE_PRX                : 作为接收器使用
     */
    nrf24 = nrf24_default_create(NRF24_DEMO_SPI_DEV_NAME, NRF24_CE_PIN, NRF24_IRQ_PIN, &_cb, ROLE_PRX);

    if (nrf24 == RT_NULL)
    {
        rt_kprintf("\n[nrf24/demo] Failed to create nrf24. stop!\n");
        for(;;) rt_thread_mdelay(10000);
    }
    else
    {
        rt_kprintf("[nrf24/demo] running.\r\n");
    }

    nrf24_send_data(nrf24, (uint8_t *)"Hi\n", 3, NRF24_DEFAULT_PIPE);

    while (1)
    {
        nrf24_run(nrf24);

        if(!nrf24->flags.using_irq){
            rt_thread_mdelay(10);
        }
        else {
            rt_thread_mdelay(50);
        }
    }

}



/***
 * @brief   nRF24L01 线程初始化
 * @return int
 */
static int nrf24l01_thread_init(void)
{
    rt_thread_t nrf24_thread_dev;

    nrf24_thread_dev = rt_thread_create("nRF24L01_entry", nRF24L01_entry, RT_NULL, 1024, 16, 20);
    rt_thread_startup(nrf24_thread_dev);

    return RT_EOK;
}

INIT_APP_EXPORT(nrf24l01_thread_init);










