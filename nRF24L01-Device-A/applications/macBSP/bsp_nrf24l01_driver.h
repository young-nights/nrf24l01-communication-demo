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
#include "bsp_sys.h"


// 命令映射
#define NRF24CMD_R_REG          0x00  // 读寄存器
#define NRF24CMD_W_REG          0x20  // 写寄存器
#define NRF24CMD_R_RX_PAYLOAD   0x61  // 读接收缓冲区
#define NRF24CMD_W_TX_PAYLOAD   0xA0  // 写发送缓冲区
#define NRF24CMD_FLUSH_TX       0xE1  // 清空发送FIFO
#define NRF24CMD_FLUSH_RX       0xE2  // 清空接收FIFO
#define NRF24CMD_REUSE_TX_PL    0xE3  // PTX模式下使用，重装载发送缓冲区
#define NRF24CMD_ACTIVATE       0x50  // 使能命令，后接数据 0x73
#define NRF24CMD_R_RX_PL_WID    0x60  // 读顶层接收FIFO大小
#define NRF24CMD_W_ACK_PAYLOAD  0xA8  // RX模式下使用，写应答发送缓冲区

// 寄存器映射
#define NRF24REG_CONFIG         0x00  // 配置收发状态，CRC校验模式以及收发状态响应方式
#define NRF24REG_EN_AA          0x01  // 自动应答功能设置
#define NRF24REG_EN_RXADDR      0x02  // 可用信道设置
#define NRF24REG_SETUP_AW       0x03  // 收发地址宽度设置
#define NRF24REG_SETUP_RETR     0x04  // 自动重发功能设置
#define NRF24REG_RF_CH          0x05  // 工作频率设置
#define NRF24REG_RF_SETUP       0x06  // 发射速率、功耗功能设置
#define NRF24REG_STATUS         0x07  // 状态寄存器
#define NRF24REG_OBSERVE_TX     0x08  // 发送监测功能
#define NRF24REG_RPD            0x09  // 接收功率检测
#define NRF24REG_RX_ADDR_P0     0x0A  // 频道0接收数据地址
#define NRF24REG_RX_ADDR_P1     0x0B  // 频道1接收数据地址
#define NRF24REG_RX_ADDR_P2     0x0C  // 频道2接收数据地址
#define NRF24REG_RX_ADDR_P3     0x0D  // 频道3接收数据地址
#define NRF24REG_RX_ADDR_P4     0x0E  // 频道4接收数据地址
#define NRF24REG_RX_ADDR_P5     0x0F  // 频道5接收数据地址
#define NRF24REG_TX_ADDR        0x10  // 发送地址寄存器
#define NRF24REG_RX_PW_P0       0x11  // 接收频道0接收数据长度
#define NRF24REG_RX_PW_P1       0x12  // 接收频道1接收数据长度
#define NRF24REG_RX_PW_P2       0x13  // 接收频道2接收数据长度
#define NRF24REG_RX_PW_P3       0x14  // 接收频道3接收数据长度
#define NRF24REG_RX_PW_P4       0x15  // 接收频道4接收数据长度
#define NRF24REG_RX_PW_P5       0x16  // 接收频道5接收数据长度
#define NRF24REG_FIFO_STATUS    0x17  // FIFO栈入栈出状态寄存器设置
#define NRF24REG_DYNPD          0x1C  // 动态数据包长度
#define NRF24REG_FEATURE        0x1D  // 特点寄存器

// 寄存器功能位掩码部分映射
// CONFIG
#define NRF24BITMASK_RX_DR      ((uint8_t)(1<<6))  // 接收完成中断使能位
#define NRF24BITMASK_TX_DS      ((uint8_t)(1<<5))  // 发送完成中断使能位
#define NRF24BITMASK_MAX_RT     ((uint8_t)(1<<4))  // 达最大重发次数中断使能位
#define NRF24BITMASK_EN_CRC     ((uint8_t)(1<<3))  // CRC使能位
#define NRF24BITMASK_CRCO       ((uint8_t)(1<<2))  // CRC编码方式 （1B or 2B）
#define NRF24BITMASK_PWR_UP     ((uint8_t)(1<<1))  // 上（掉）电
#define NRF24BITMASK_PRIM_RX    ((uint8_t)(1))     // PR（T）X
//SETUP_AW
#define NRF24BITMASK_AW         ((uint8_t)(0x03))  // RX/TX地址宽度
//SETUP_RETR
#define NRF24BITMASK_ARD        ((uint8_t)(0xF0))  // 重发延时
#define NRF24BITMASK_ARC        ((uint8_t)(0x0F))  // 重发最大次数
//RF_CH
#define NRF24BITMASK_RF_CH      ((uint8_t)(0x7F))  // 射频频道
//RF_SETUP
#define NRF24BITMASK_RF_DR      ((uint8_t)(1<<3))  // 空中速率
#define NRF24BITMASK_RF_PWR     ((uint8_t)(0x06))  // 发射功率
//STATUS
#define NRF24BITMASK_RX_DR      ((uint8_t)(1<<6))  // 接收完成标志位
#define NRF24BITMASK_TX_DS      ((uint8_t)(1<<5))  // 发送完成标志位
#define NRF24BITMASK_MAX_RT     ((uint8_t)(1<<4))  // 最大重发次数标志位
#define NRF24BITMASK_RX_P_NO    ((uint8_t)(0x0E))  // RX_FIFO状态标志区位
#define NRF24BITMASK_TX_FULL    ((uint8_t)(1))     // TX_FIFO满标志位
//OBSERVE_TX
#define NRF24BITMASK_PLOS_CNT   ((uint8_t)(0xF0))  // 丢包计数
#define NRF24BITMASK_ARC_CNT    ((uint8_t)(0x0F))  // 重发计数
//CD
#define NRF24BITMASK_CD         ((uint8_t)(1))     // 载波检测标志位
//通用掩码，RX_PW_P[0::5] 掩码相同
#define NRF24BITMASK_RX_PW_P_   ((uint8_t)(0x3F))  // 数据管道RX-Payload中的字节数
//FIFO_STATUS
#define NRF24BITMASK_TX_REUSE   ((uint8_t)(1<<6))  //
#define NRF24BITMASK_TX_FULL2    ((uint8_t)(1<<5)) //
#define NRF24BITMASK_TX_EMPTY   ((uint8_t)(1<<4))  //
#define NRF24BITMASK_RX_RXFULL  ((uint8_t)(1<<1))  //
#define NRF24BITMASK_RX_EMPTY   ((uint8_t)(1))     //
//FEATURE
#define NRF24BITMASK_EN_DPL     ((uint8_t)(1<<2))  // 动态长度使能位
#define NRF24BITMASK_EN_ACK_PAY ((uint8_t)(1<<1))  // Payload with ACK 使能位
#define NRF24BITMASK_EN_DYN_ACK ((uint8_t)(1))     // W_TX_PAYLOAD_NOACK 命令使能位
//通用掩码，适用于多个寄存器： EN_AA, EN_RXADDR, DYNPD
#define NRF24BITMASK_PIPE_0     ((uint8_t)(1))     //
#define NRF24BITMASK_PIPE_1     ((uint8_t)(1<<1))  //
#define NRF24BITMASK_PIPE_2     ((uint8_t)(1<<2))  //
#define NRF24BITMASK_PIPE_3     ((uint8_t)(1<<3))  //
#define NRF24BITMASK_PIPE_4     ((uint8_t)(1<<4))  //
#define NRF24BITMASK_PIPE_5     ((uint8_t)(1<<5))  //


#define NRF24_DEFAULT_PIPE      NRF24_PIPE_0

enum
{
    NRF24_PIPE_NONE = 8,
    NRF24_PIPE_0 = 0,
    NRF24_PIPE_1,
    NRF24_PIPE_2,
    NRF24_PIPE_3,
    NRF24_PIPE_4,
    NRF24_PIPE_5,
};

typedef enum
{
    ROLE_NONE = 2,
    ROLE_PTX = 0,
    ROLE_PRX = 1,
} nrf24_role_et;

typedef enum
{
    MODE_POWER_DOWN,
    MODE_STANDBY,
    MODE_TX,
    MODE_RX,
} nrf24_mode_et;

typedef enum
{
    // CRC_NONE = 2,
    CRC_1_BYTE = 0,
    CRC_2_BYTE = 1,
} nrf24_crc_et;

typedef enum
{
    RF_POWER_N18dBm = 0,
    RF_POWER_N12dBm = 0x1,
    RF_POWER_N6dBm  = 0x2,
    RF_POWER_0dBm   = 0x3,
} nrf24_power_et;

typedef enum
{
    ADR_1Mbps = 0,
    ADR_2Mbps = 1,
} nrf24_adr_et;

/* User-oriented configuration */
struct nrf24_cfg
{
    nrf24_role_et role;
    nrf24_power_et power;
    nrf24_crc_et crc;
    nrf24_adr_et adr;
    uint8_t channel :7;    //range: 0 ~ 127 (frequency:)

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
    struct hal_nrf24_port halport;
    struct nrf24_cfg cfg;
    struct nrf24_callback cb;

    struct {
        uint8_t activated_features      :1;
        uint8_t using_irq               :1;
    } flags;

    uint8_t status;

    rt_sem_t sem;   // irq
    rt_sem_t send_sem;
};


#endif /* APPLICATIONS_MACBSP_INC_BSP_LORA_DRIVER_H_ */
