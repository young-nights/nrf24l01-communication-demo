#include <bsp_nrf24l01_spi.h>
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-29     Administrator       the first version
 */


#if USE_CUSTOMER_NRF24L01

//以下是SPI以及LORA引脚配置相关的函数---------------------------------------------------------------------------------------------





static int nrf24_send_then_recv(nrf24_port_t halport, const uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t rlen)
{
    return rt_spi_send_then_recv(halport->nrf24_spi_dev, tbuf, tlen, rbuf, rlen);
}


static int nrf24_send_then_send(nrf24_port_t halport, const uint8_t *buf1, uint8_t len1, const uint8_t *buf2, uint8_t len2)
{
    return rt_spi_send_then_send(halport->nrf24_spi_dev, buf1, len1, buf2, len2);
}

static int nrf24_write(nrf24_port_t halport, const uint8_t *buf, uint8_t len)
{
    return rt_spi_send(halport->nrf24_spi_dev, buf, len);
}

static int nrf24_set_ce(nrf24_port_t halport)
{
    rt_pin_write(halport->nrf24_ce_pin, PIN_HIGH);
    return RT_EOK;
}

static int nrf24_reset_ce(nrf24_port_t halport)
{
    rt_pin_write(halport->nrf24_ce_pin, PIN_LOW);
    return RT_EOK;
}

static int nrf24_read_irq_pin(nrf24_port_t halport)
{
    return rt_pin_read(halport->nrf24_irq_pin);
}


static void nrf24_hw_interrupt(void *args)
{
    nrf24_port_t halport = (nrf24_port_t)args;
    halport->nrf24_irq_cb(halport);
}

const static struct nrf24_func_opts g_nrf24_func_ops = {
    .nrf24_send_then_recv = nrf24_send_then_recv,
    .nrf24_send_then_send = nrf24_send_then_send,
    .nrf24_write = nrf24_write,
    .nrf24_set_ce = nrf24_set_ce,
    .nrf24_reset_ce = nrf24_reset_ce,
    .nrf24_read_irq_pin = nrf24_read_irq_pin,
};




/* 宏定义SPI名称 */
#define     nRF24_SPI_NAME    "nrf24_spi1"
/* 宏定义SPI总线 */
#define     nRF24_SPI_BUS     "spi1"
/* 失能引脚 */
#define     nRF24_PIN_NONE    -1

int nrf24_port_init(nrf24_port_t halport, char *spi_dev_name, int ce_pin, int irq_pin, void(*irq_callback)(nrf24_port_t halport))
{
    RT_ASSERT(halport != RT_NULL);

    //--------------------------------------------------------------------------------------
    struct rt_spi_device *spi_dev_nrf24;

    /* 将SPI设备名挂载到总线 */
    rt_hw_spi_device_attach(nRF24_SPI_BUS, nRF24_SPI_NAME, nRF24_NSS_PORT, nRF24_NSS_PIN);
    /* 查找SPI设备 */
    spi_dev_nrf24 = (struct rt_spi_device *)rt_device_find(nRF24_SPI_NAME);
    if(spi_dev_nrf24 == NULL){
        LOG_E("nRF24 spi device is not created!\r\n");
        return RT_ERROR;
    }
    else{
        rt_kprintf("nRF24 spi device is successfully!\r\n");
    }

    /***
     *! 配置SPI结构体参数
     *! data_width: spi传输数据长度为8bits
     *! max_hz: spi的最大工作频率；
     *! mode-> RT_SPI_MASTER: 主机模式；
     *! mode-> RT_SPI_MODE_0: 工作模式0（ CPOL:0 -- 空闲状态为低电平 ， CPHA:0 -- 第一个边沿采集数据 ）
     *! mode-> RT_SPI_MSB: 通讯数据高位在前低位在后
     * */
    struct rt_spi_configuration nrf24_spi_cfg;

    nrf24_spi_cfg.data_width = 8;
    nrf24_spi_cfg.max_hz = 10*1000*1000; /* 10M,SPI max 10MHz,lora 4-wire spi */
    nrf24_spi_cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    rt_spi_configure(spi_dev_nrf24, &nrf24_spi_cfg); /* 使能参数 */

    //--------------------------------------------------------------------------------------
    /***
     * nrf24_spi_dev: nRF24L01挂载的SPI总线设备
     * nrf24_ce_pin : nRF24L01的CE引脚（用于控制收发模式）
     * nrf24_irq_pin: nRF24L01的IRQ引脚（用于接收中断信号）
     * nrf24_irq_cb : nRF24L01的中断回调函数
     * nrf24_ops    : nRF24l01的底层操作函数
     */
    halport->nrf24_spi_dev = spi_dev_nrf24;
    halport->nrf24_ce_pin = ce_pin;
    halport->nrf24_irq_pin = irq_pin;
    halport->nrf24_irq_cb = irq_callback;
    halport->nrf24_ops = &g_nrf24_func_ops;

    /* 拉低CE引脚，配置成发送模式 */
    rt_pin_mode(ce_pin, PIN_MODE_OUTPUT);
    nrf24_reset_ce(halport);

    /* 如果IRQ引脚为有效值，并且中断回调函数指针不为空 */
    if(irq_pin != nRF24_PIN_NONE && irq_callback != RT_NULL)
    {
        /* 设置成上拉输入 */
        rt_pin_mode(irq_pin, PIN_MODE_INPUT_PULLUP);
        /* 挂载到外部中断，模式：下降沿触发 ，触发函数绑定到 nrf24_hw_interrupt 中，halport 作为参数传入 */
        rt_pin_attach_irq(irq_pin, PIN_IRQ_MODE_FALLING, nrf24_hw_interrupt, halport);
        /* 使能引脚中断 */
        rt_pin_irq_enable(irq_pin, PIN_IRQ_ENABLE);
    }

    return RT_EOK;
}


nrf24_port_t nrf24_port_create(char *spi_dev_name, int ce_pin, int irq_pin, void(*irq_callback)(nrf24_port_t halport))
{
    struct nrf24_port *halport;

    halport = (struct nrf24_port *)rt_malloc(sizeof(struct nrf24_port));
    if (halport == RT_NULL)
    {
        LOG_E("Failed to allocate memory!");
    }
    else
    {
        if (nrf24_port_init(halport, spi_dev_name, ce_pin, irq_pin, irq_callback) != RT_EOK)
        {
            rt_free(halport);
            halport = RT_NULL;
        }
    }

    return halport;
}


#endif









