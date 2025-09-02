
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

//以下是SPI以及LORA引脚配置相关的函数---------------------------------------------------------------------------------------------

/* 宏定义SPI名称 */
#define     nRF24_SPI_NAME    "nrf24_spi1"
/* 宏定义SPI总线 */
#define     nRF24_SPI_BUS     "spi1"
/* 创建spi设备句柄 */
struct rt_spi_device *spi_dev_nrf24;




int nRF24L01_SPI_Init(void)
{
    //--------------------------------------------------------------------------------------
    /* 将SPI设备名挂载到总线 */
    rt_hw_spi_device_attach(nRF24_SPI_BUS, nRF24_SPI_NAME, nRF24_NSS_PORT, nRF24_NSS_PIN);
    /* 查找SPI设备 */
    spi_dev_nrf24 = (struct rt_spi_device *)rt_device_find(nRF24_SPI_NAME);
    if(spi_dev_nrf24 == NULL){
        LOG_E("nRF24 spi device is not found!\r\n");
        return RT_ERROR;
    }
    else{
        LOG_I("nRF24 spi device is successfully!\r\n");
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

    return RT_EOK;
}























#endif









