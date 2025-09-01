/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-30     18452       the first version
 */

#ifndef APPLICATIONS_MACBSP_INC_BSP_LORA_DRIVER_H_
#define APPLICATIONS_MACBSP_INC_BSP_LORA_DRIVER_H_

#include "bsp_nrf24l01_spi.h"



/***
 *! 数据通信管道的枚举
 *! Pipe0 ~ Pipe1 : 5字节完整地址，可自定义
 *! Pipe2 ~ Pipe5 : 1字节低位地址，高位公用 Pipe1 的前4字节
 *! Pipe8         : 无管道
 */
#define NRF24_DEFAULT_PIPE      NRF24_PIPE_0
enum
{
    NRF24_PIPE_NONE = 8,//!< NRF24_PIPE_NONE
    NRF24_PIPE_0 = 0,   //!< NRF24_PIPE_0
    NRF24_PIPE_1,       //!< NRF24_PIPE_1
    NRF24_PIPE_2,       //!< NRF24_PIPE_2
    NRF24_PIPE_3,       //!< NRF24_PIPE_3
    NRF24_PIPE_4,       //!< NRF24_PIPE_4
    NRF24_PIPE_5,       //!< NRF24_PIPE_5
};


/***
 *! nRF24L01的收发器模式的枚举
 *! ROLE_NONE   : 尚未设置是接收端还是发送端
 *! ROLE_PTX    : 作为发送器
 *! ROLE_PRX    : 作为接收器
 */
typedef enum
{
    ROLE_NONE = 2,
    ROLE_PTX = 0,
    ROLE_PRX = 1,
} nrf24_role_et;

/***
 *! 四种工作模式的美剧
 *! MODE_POWER_DOWN : 最省电，晶振关闭，SPI 仍可访问寄存器，典型电流 ≈ 900 nA
 *! MODE_STANDBY    : 晶振运行但射频不工作，功耗 ≈ 26 µA，切换 TX/RX 前必须停留在此
 *! MODE_TX         : 发送状态，CE 拉高后芯片开始发射 FIFO 中的数据
 *! MODE_RX         ：接收状态，CE 拉高后芯片开始监听空中数据并写入 RX FIFO
 */
typedef enum
{
    MODE_POWER_DOWN,
    MODE_STANDBY,
    MODE_TX,
    MODE_RX,
} nrf24_mode_et;

/***
 *!  CRC 校验长度模式枚举
 *! CRC_1_BYTE : 短包/低延迟场景，可节省 1 字节空中时间
 *! CRC_2_BYTE : 默认推荐，抗干扰能力更强，几乎成了标准用法
 */
typedef enum
{
    // CRC_NONE = 2,
    CRC_1_BYTE = 0,
    CRC_2_BYTE = 1,
} nrf24_crc_et;


/***
 *! nRF24L01 的发射功率模式枚举
 *! RF_POWER_N18dBm : 实际功率：-18 dBm，典型电流：7.0 mA，  适用场景：极近距离/省电
 *! RF_POWER_N12dBm : 实际功率：-12 dBm，典型电流：7.5 mA，  适用场景：中等距离
 *! RF_POWER_N6dBm  ：实际功率：-6 dBm，  典型电流：9.0 mA，  适用场景：一般室内
 *! RF_POWER_0dBm   ：实际功率：0 dBm，   典型电流：11.3 mA，适用场景：默认/远距离
 */
typedef enum
{
    RF_POWER_N18dBm = 0,
    RF_POWER_N12dBm = 0x1,
    RF_POWER_N6dBm  = 0x2,
    RF_POWER_0dBm   = 0x3,
} nrf24_power_et;


/***
 *! nRF24L01 的空中数据速率模式枚举
 *! ADR_1Mbps : 距离更远，抗干扰好
 *! ADR_2Mbps : 延迟更低，带宽更高
 */
typedef enum
{
    ADR_1Mbps = 0,
    ADR_2Mbps = 1,
} nrf24_adr_et;



/* User-oriented configuration */
struct nrf24_cfg
{
    nrf24_role_et  role;    /*! 收发器模式 */
    nrf24_power_et power;   /*! 射频发射功率模式 */
    nrf24_crc_et   crc;     /*! CRC校验模式 */
    nrf24_adr_et   adr;     /*! 空中数据传输速率模式 */
    uint8_t        channel :7;    //range: 0 ~ 127 (frequency:)

    int _irq_pin;

    uint8_t txaddr[5];

    struct {
        uint8_t bl_enabled;
        uint8_t addr[5];
    } rxpipe0;

    struct {
        uint8_t bl_enabled;
        uint8_t addr[5];
    } rxpipe1;

    struct {
        uint8_t bl_enabled;
        uint8_t addr;
    } rxpipe2;

    struct {
        uint8_t bl_enabled;
        uint8_t addr;
    } rxpipe3;

    struct {
        uint8_t bl_enabled;
        uint8_t addr;
    } rxpipe4;

    struct {
        uint8_t bl_enabled;
        uint8_t addr;
    } rxpipe5;
};


typedef struct nrf24_cfg *nrf24_cfg_t;


typedef struct nrf24 *nrf24_t;

struct nrf24_callback
{
    void (*rx_ind)(nrf24_t nrf24, uint8_t *data, uint8_t len, int pipe);
    void (*tx_done)(nrf24_t nrf24, int pipe);
};

struct nrf24
{
    struct nrf24_port halport;
    struct nrf24_cfg  cfg;
    struct nrf24_callback cb;

    struct {
        uint8_t activated_features      :1;
        uint8_t using_irq               :1;
    } flags;

    uint8_t status;

    rt_sem_t sem;   // irq
    rt_sem_t send_sem;
};


int nrf24_init(nrf24_t nrf24, char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, const nrf24_cfg_t cfg);
nrf24_t nrf24_create(char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, const nrf24_cfg_t cfg);

int nrf24_default_init(nrf24_t nrf24, char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, nrf24_role_et role);
nrf24_t nrf24_default_create(char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, nrf24_role_et role);

void nrf24_enter_power_down_mode(nrf24_t nrf24);
void nrf24_enter_power_up_mode(nrf24_t nrf24);

int nrf24_fill_default_config_on(nrf24_cfg_t cfg);
int nrf24_send_data(nrf24_t nrf24, uint8_t *data, uint8_t len, uint8_t pipe);
int nrf24_run(nrf24_t nrf24);


#endif /* APPLICATIONS_MACBSP_INC_BSP_LORA_DRIVER_H_ */
