/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-29     Administrator       the first version
 */
#include <bsp_nrf24l01_spi.h>
#include <rtdbg.h>


#if USE_CUSTOMER_NRF24L01

//以下是SPI以及nRF24L01引脚配置相关的函数---------------------------------------------------------------------------------------------

/* 宏定义SPI名称 */
#define     nRF24_SPI_NAME    "nrf24_spi2"
/* 宏定义SPI总线 */
#define     nRF24_SPI_BUS     "spi2"




int nRF24L01_SPI_Init(nrf24_port_api_t port_api)
{
    //--------------------------------------------------------------------------------------
    /* 将SPI设备名挂载到总线 */
    rt_hw_spi_device_attach(nRF24_SPI_BUS, nRF24_SPI_NAME, nRF24_NSS_PORT, nRF24_NSS_PIN);
    /* 查找SPI设备 */
    port_api->spi_dev_nrf24 = (struct rt_spi_device *)rt_device_find(nRF24_SPI_NAME);
    if(port_api->spi_dev_nrf24 == NULL){
        LOG_E("LOG:%d. nRF24 spi device is not found!",Record.ulog_cnt++);
        return RT_ERROR;
    }
    else{
        LOG_I("LOG:%d. nRF24 spi device is successfully!",Record.ulog_cnt++);
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
    nrf24_spi_cfg.max_hz = 1*1000*1000; /* 10M,SPI max 10MHz,lora 4-wire spi */
    nrf24_spi_cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    rt_spi_configure(port_api->spi_dev_nrf24, &nrf24_spi_cfg); /* 使能参数 */

    //--------------------------------------------------------------------------------------

    return RT_EOK;
}





/**
  * @brief  nRF24L01 的IRQ引脚的中断回调函数(把入口挂载到这个里面)
  * @retval void
  */
static void nRF24L01_INT_Callback(void *args)
{
    rt_interrupt_enter();
    rt_sem_release(nrf24_irq_sem);
    rt_interrupt_leave();
}




/**
  * @brief  nRF24L01 的中断引脚初始化
  * @retval void
  */
int nRF24L01_IQR_GPIO_Config(nrf24_port_api_t port_api)
{
    rt_pin_mode(GET_PIN(C, 7), PIN_MODE_INPUT);         /* 保险起见 */
    rt_pin_attach_irq(  GET_PIN(C, 7),
                        PIN_IRQ_MODE_FALLING,           /* 与 CubeMX 极性一致 */
                        nRF24L01_INT_Callback,
                        RT_NULL);                       /* 不需要参数 */
    rt_pin_irq_enable(GET_PIN(C, 7), PIN_IRQ_ENABLE);
    return RT_EOK;
}






//以下是nRF24L01的SPI底层读写函数---------------------------------------------------------------------------------------------


/***
 * 向 nRF24L01 发送 指定字节数据，然后再接收指定字节数据
 */
static int nrf24_send_then_recv(nrf24_port_api_t port_api,
                                const uint8_t *tbuf,
                                uint8_t tlen,
                                uint8_t *rbuf,
                                uint8_t rlen)
{
    return rt_spi_send_then_recv(port_api->spi_dev_nrf24, tbuf, tlen, rbuf, rlen);
}

/***
 * 向 nRF24L01 发送 指定字节数据，然后再发送指定字节数据
 */
static int nrf24_send_then_send(nrf24_port_api_t port_api,
                                const uint8_t *buf1,
                                uint8_t len1,
                                const uint8_t *buf2,
                                uint8_t len2)
{
    return rt_spi_send_then_send(port_api->spi_dev_nrf24, buf1, len1, buf2, len2);
}

/***
 * 向 nRF24L01写入1字节数据
 */
static int nrf24_write(nrf24_port_api_t port_api, const uint8_t *buf, uint8_t len)
{
    return rt_spi_send(port_api->spi_dev_nrf24, buf, len);
}

/***
 * 拉高nRF24L01的CE引脚
 */
static int nrf24_set_ce(void)
{
    nRF24_CS_SET(1);
    return RT_EOK;
}

/***
 * 拉低nRF24L01的CE引脚
 */
static int nrf24_reset_ce(void)
{
    nRF24_CS_SET(0);
    return RT_EOK;
}


const struct nRF24L01_FUNC_OPS g_nrf24_func_ops = {
    .nrf24_send_then_recv = nrf24_send_then_recv,
    .nrf24_send_then_send = nrf24_send_then_send,
    .nrf24_write = nrf24_write,
    .nrf24_set_ce = nrf24_set_ce,
    .nrf24_reset_ce = nrf24_reset_ce,
};



#endif

