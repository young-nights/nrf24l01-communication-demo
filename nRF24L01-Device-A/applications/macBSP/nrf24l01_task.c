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

#define NRF24_DEMO_SPI_DEV_NAME "spi1"



static void thread_entry(void *param)
{
    nrf24_t nrf24;

    rt_kprintf("[nrf24/demo] Version:%s\n", PKG_NRF24L01_VERSION);

    nrf24 = nrf24_default_create(NRF24_DEMO_SPI_DEV_NAME, NRF24_DEMO_CE_PIN, NRF24_DEMO_IRQ_PIN, &_cb, NRF24_DEMO_ROLE);

    if (nrf24 == RT_NULL)
    {
        rt_kprintf("\n[nrf24/demo] Failed to create nrf24. stop!\n");
        for(;;) rt_thread_mdelay(10000);
    }
    else
    {
        rt_kprintf("[nrf24/demo] running.");
    }

    nrf24_send_data(nrf24, (uint8_t *)"Hi\n", 3, NRF24_DEFAULT_PIPE);

    while (1)
    {
        nrf24_run(nrf24);

        if(!nrf24->flags.using_irq)
            rt_thread_mdelay(10);
    }

}

static int nrf24l01_sample_init(void)
{
    rt_thread_t thread;

    thread = rt_thread_create("nrfDemo", thread_entry, RT_NULL, 1024, RT_THREAD_PRIORITY_MAX/2, 20);
    rt_thread_startup(thread);

    return RT_EOK;
}

INIT_APP_EXPORT(nrf24l01_sample_init);










