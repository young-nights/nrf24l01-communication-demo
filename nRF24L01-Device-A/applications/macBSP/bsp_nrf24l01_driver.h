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

// 以下是寄存器列表 ---------------------------------------------------------------------------------------------

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
#define NRF24BITMASK_TX_REUSE   ((uint8_t)(1<<6))
#define NRF24BITMASK_TX_FULL2    ((uint8_t)(1<<5))
#define NRF24BITMASK_TX_EMPTY   ((uint8_t)(1<<4))
#define NRF24BITMASK_RX_RXFULL  ((uint8_t)(1<<1))
#define NRF24BITMASK_RX_EMPTY   ((uint8_t)(1))
//FEATURE
#define NRF24BITMASK_EN_DPL     ((uint8_t)(1<<2))  // 动态长度使能位
#define NRF24BITMASK_EN_ACK_PAY ((uint8_t)(1<<1))  // Payload with ACK 使能位
#define NRF24BITMASK_EN_DYN_ACK ((uint8_t)(1))     // W_TX_PAYLOAD_NOACK 命令使能位
//通用掩码，适用于多个寄存器： EN_AA, EN_RXADDR, DYNPD
#define NRF24BITMASK_PIPE_0     ((uint8_t)(1))
#define NRF24BITMASK_PIPE_1     ((uint8_t)(1<<1))
#define NRF24BITMASK_PIPE_2     ((uint8_t)(1<<2))
#define NRF24BITMASK_PIPE_3     ((uint8_t)(1<<3))
#define NRF24BITMASK_PIPE_4     ((uint8_t)(1<<4))
#define NRF24BITMASK_PIPE_5     ((uint8_t)(1<<5))






// 以下是一些模式的枚举 ---------------------------------------------------------------------------------------------
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





// 以下是一些参数寄存器以及信道的配置结构体 --------------------------------------------------------------------------------------
/* User-oriented configuration */
struct nrf24_cfg
{
    nrf24_role_et  role;    /*! 收发器模式 */
    nrf24_power_et power;   /*! 射频发射功率模式 */
    nrf24_crc_et   crc;     /*! CRC校验模式 */
    nrf24_adr_et   adr;     /*! 空中数据传输速率模式 */
    uint8_t        channel :7;    //range: 0 ~ 127 (frequency:)

    int _irq_pin;   /*! 中断引脚的 GPIO 编号*/

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
        uint8_t using_irq               :1;     // 是否使用IRQ引脚的标志位
    } flags;

    uint8_t status;

    rt_sem_t sem;   // irq
    rt_sem_t send_sem;
};




// 函数声明 ---------------------------------------------------------------------------------------------
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
