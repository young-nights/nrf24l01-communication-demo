/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-03     18452       the first version
 */
#ifndef APPLICATIONS_MACBSP_BSP_NRF24L01_DRIVER_H_
#define APPLICATIONS_MACBSP_BSP_NRF24L01_DRIVER_H_

#include "bsp_sys.h"
#include "bsp_nrf24l01_spi.h"


// 以下是一些模式的枚举 ---------------------------------------------------------------------------------------------
/***
 * nRF24L01 数据通信管道的枚举
 * Pipe0 ~ Pipe1 : 5字节完整地址，可自定义
 * Pipe2 ~ Pipe5 : 1字节低位地址，高位公用 Pipe1 的前4字节
 * Pipe8         : 无管道
 */
typedef enum
{
    NRF24_PIPE_NONE = 8,
    NRF24_PIPE_0 = 0,
    NRF24_PIPE_1,
    NRF24_PIPE_2,
    NRF24_PIPE_3,
    NRF24_PIPE_4,
    NRF24_PIPE_5,
}nrf24_pipe_et;

#define NRF24_DEFAULT_PIPE      NRF24_PIPE_0

/***
 * nRF24L01的收发器模式的枚举
 * ROLE_NONE   : 尚未设置是接收端还是发送端
 * ROLE_PTX    : 作为发送器
 * ROLE_PRX    : 作为接收器
 */
typedef enum
{
    ROLE_PTX = 0,
    ROLE_PRX = 1,
    ROLE_NONE = 2,
} nrf24_role_et;

/***
 * nRF24L01 的四种工作模式的枚举
 * MODE_POWER_DOWN : 最省电，晶振关闭，SPI 仍可访问寄存器，典型电流 ≈ 900 nA
 * MODE_STANDBY    : 晶振运行但射频不工作，功耗 ≈ 26 µA，切换 TX/RX 前必须停留在此
 * MODE_TX         : 发送状态，CE 拉高后芯片开始发射 FIFO 中的数据
 * MODE_RX         ：接收状态，CE 拉高后芯片开始监听空中数据并写入 RX FIFO
 */
typedef enum
{
    MODE_POWER_DOWN,
    MODE_STANDBY,
    MODE_TX,
    MODE_RX,
} nrf24_mode_et;



/***
 * nRF24L01 的应答模式
 * NEED_ACK :  发送后，需要应答，否则一直重发
 * NO_ACK   :  发送后，无需应答
 * IN_ACK   :  接收后，发送应答
 */
typedef enum
{
    nRF24_SEND_NEED_ACK,
    nRF24_SEND_NO_ACK,
    nRF24_RECE_IN_ACK,
} ack_mode_et;


/***
 * CRC 校验长度模式枚举
 * CRC_1_BYTE : 短包/低延迟场景，可节省 1 字节空中时间
 * CRC_2_BYTE : 默认推荐，抗干扰能力更强，几乎成了标准用法
 */
typedef enum
{
    CRC_1_BYTE = 0,
    CRC_2_BYTE = 1,
} nrf24_crc_et;

/***
 * nRF24L01 的发射功率模式枚举
 * RF_POWER_N18dBm : 实际功率：-18 dBm，典型电流：7.0 mA，  适用场景：极近距离/省电
 * RF_POWER_N12dBm : 实际功率：-12 dBm，典型电流：7.5 mA，  适用场景：中等距离
 * RF_POWER_N6dBm  ：实际功率：-6 dBm，  典型电流：9.0 mA，  适用场景：一般室内
 * RF_POWER_0dBm   ：实际功率：0 dBm，   典型电流：11.3 mA，适用场景：默认/远距离
 */
typedef enum
{
    RF_POWER_N18dBm = 0,
    RF_POWER_N12dBm = 0x1,
    RF_POWER_N6dBm  = 0x2,
    RF_POWER_0dBm   = 0x3,
} nrf24_power_et;

/***
 * nRF24L01 的空中数据速率模式枚举
 * ADR_1Mbps : 距离更远，抗干扰好
 * ADR_2Mbps : 延迟更低，带宽更高
 */
typedef enum
{
    ADR_1Mbps = 0,
    ADR_2Mbps = 1,
} nrf24_adr_et;

/***
 * nRF24L01的待机模式的枚举
 * Standby-Ⅰ   : PWR_UP = 1 且 CE = 0
 * Standby-Ⅱ  : PWR_UP = 1 且 CE = 1 但 TX FIFO 为空
 * Power-Down: 低功耗模式
 */
typedef enum
{
    Standby_one = 0,
    Standby_two,
    PowerDown,
} nrf24_standby_et;

/***
 * nRF24L01 的参数配置结构体
 */
struct nRF24L01_PARAMETER_STRUCT
{

    uint8_t txaddr[5];

    /* CONFIG */
    struct {
        uint8_t prim_rx     :1;
        uint8_t pwr_up      :1;
        uint8_t crco        :1;
        uint8_t en_crc      :1;
        uint8_t mask_max_rt :1;
        uint8_t mask_tx_ds  :1;
        uint8_t mask_rx_dr  :1;
    } config;

    /* EN_AA */
    struct {
        uint8_t p0          :1;
        uint8_t p1          :1;
        uint8_t p2          :1;
        uint8_t p3          :1;
        uint8_t p4          :1;
        uint8_t p5          :1;
    } en_aa;

    /* EN_RXADDR */
    struct {
        uint8_t p0          :1;
        uint8_t p1          :1;
        uint8_t p2          :1;
        uint8_t p3          :1;
        uint8_t p4          :1;
        uint8_t p5          :1;
    } en_rxaddr;

    /* SETUP_AW */
    struct {
        uint8_t aw          :2;
    } setup_aw;

    /* SETUP_RETR */
    struct {
        uint8_t arc         :4;
        uint8_t ard         :4;
    } setup_retr;

    /* RF_CH */
    struct {
        uint8_t rf_ch       :7;
    } rf_ch;


    /* RF_SETUP */
    struct {
        uint8_t rf_pwr      :2;
        uint8_t rf_dr_high  :1;
        uint8_t pll_lock    :1;
        uint8_t rf_dr_low   :1;
        uint8_t cont_wave   :1;
    } rf_setup;

    /* DYNPD */
    struct {
        uint8_t p0          :1;
        uint8_t p1          :1;
        uint8_t p2          :1;
        uint8_t p3          :1;
        uint8_t p4          :1;
        uint8_t p5          :1;
    } dynpd;

    /* FEATURE */
    struct {
        uint8_t en_dyn_ack  :1;
        uint8_t en_ack_pay  :1;
        uint8_t en_dpl      :1;
    } feature;

    uint8_t rx_addr_p0[5];
    uint8_t rx_addr_p1[5];
    uint8_t rx_addr_p2;
    uint8_t rx_addr_p3;
    uint8_t rx_addr_p4;
    uint8_t rx_addr_p5;

}__attribute__((aligned(1)));

typedef struct nRF24L01_PARAMETER_STRUCT *nrf24_param_t;



/***
 * nRF24L01 的变量和标志位配置结构体
 */
struct nRF24L01_Flag_Struct{
    uint8_t activated_features      :1;
    uint8_t using_irq               :1;
    uint8_t status;
}__attribute__((aligned(1)));




/***
 * nRF24L01 的硬件接口以及API函数接口结构体
 */

struct nRF24L01_PORT_API
{
    /* 创建spi设备句柄 */
    struct rt_spi_device *spi_dev_nrf24;
    /* nRF24L01的IRQ引脚的RTT引脚编号 */
    rt_uint8_t nRF24L01_IRQ_Pin_Num;
};
typedef struct nRF24L01_PORT_API *nrf24_port_api_t;



/***
 * nRF24L01 的 SPI 函数指针结构体
 */
struct nRF24L01_FUNC_OPS
{
    int (* nrf24_send_then_recv)(struct nRF24L01_PORT_API *port_api, const uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t rlen);
    int (* nrf24_send_then_send)(struct nRF24L01_PORT_API *port_api, const uint8_t *tbuf1, uint8_t tlen1, const uint8_t *tbuf2, uint8_t tlen2);
    int (* nrf24_write)(struct nRF24L01_PORT_API *port_api, const uint8_t *buf, uint8_t len);
    int (* nrf24_set_ce)(void);
    int (* nrf24_reset_ce)(void);
};



typedef struct nRF24L01_STRUCT *nrf24_t;

/***
 * nRF24L01的软件回调函数，这个回调与事件相关
 */
struct nrf24_callback
{
    void (*nrf24l01_rx_ind)(nrf24_t nrf24, uint8_t *data, uint8_t len, int pipe);
    void (*nrf24l01_tx_done)(nrf24_t nrf24, rt_uint8_t pipe);
};

/***
 * nRF24L01的最上层的结构体
 */
struct nRF24L01_STRUCT
{
    /* nRF24L01 的硬件接口以及API函数接口结构体 */
    struct nRF24L01_PORT_API port_api;
    /* nRF24L01 的参数配置结构体 */
    struct nRF24L01_PARAMETER_STRUCT nrf24_cfg;
    /* nRF24L01 的标志位的结构体 */
    struct nRF24L01_Flag_Struct nrf24_flags;
    /* 创建nRF24L01的操作函数句柄 */
    struct nRF24L01_FUNC_OPS nrf24_ops;
    /* nRF24L01的事件回调函数句柄 */
    struct nrf24_callback nrf24_cb;
};









// 外部信号量声明 -------------------------------------------------------------------
extern rt_sem_t nrf24_send_sem;
extern rt_sem_t nrf24_irq_sem;
extern nrf24_t _nrf24;

// 函数声明 -------------------------------------------------------------------
int nRF24L01_Param_Config(nrf24_param_t param);
int nRF24L01_Check_SPI_Community(nrf24_t port_ops);
int nRF24L01_Update_Parameter(nrf24_t nrf24);
int nRF24L01_Read_Onchip_Parameter(nrf24_t nrf24);
uint8_t nRF24L01_Read_Reg_Data(nrf24_t nrf24, uint8_t reg_addr);
void nRF24L01_Write_Reg_Data(nrf24_t nrf24, uint8_t reg_addr, uint8_t data);
void nRF24L01_Write_Reg_Bits(nrf24_t nrf24, uint8_t reg_addr, uint8_t mask, uint8_t value);
uint8_t nRF24L01_Read_Status_Register(nrf24_t nrf24);
void nRF24L01_Clear_Status_Register(nrf24_t nrf24, uint8_t bitmask);
void nRF24L01_Clear_IRQ_Flags(nrf24_t nrf24);
rt_uint8_t nRF24L01_Read_IRQ_Status(nrf24_t nrf24);
void nRF24L01_Clear_Observe_TX(nrf24_t nrf24);
uint8_t nRF24L01_Read_Top_RXFIFO_Width(nrf24_t nrf24);
void nRF24L01_Enter_Power_Down_Mode(nrf24_t nrf24);
void nRF24L01_Enter_Power_Up_Mode(nrf24_t nrf24);
void nRF24L01_Standby_Set(nrf24_t nrf24, nrf24_standby_et mode);
void nRF24L01_Write_Tx_Payload_Ack(nrf24_t nrf24, const uint8_t *buf, uint8_t len);
void nRF24L01_Write_Tx_Payload_NoAck(nrf24_t nrf24, const uint8_t *buf, uint8_t len);
void nRF24L01_Write_Tx_Payload_InAck(nrf24_t nrf24, uint8_t pipe, const uint8_t *buf, uint8_t len);
void nRF24L01_Read_Rx_Payload(nrf24_t nrf24, uint8_t *buf, uint8_t len);
void nRF24L01_Flush_TX_FIFO(nrf24_t nrf24);
void nRF24L01_Flush_RX_FIFO(nrf24_t nrf24);
void NRF24L01_Set_TxAddr(nrf24_t nrf24, rt_uint8_t *addr_buf, rt_uint8_t length);
int nRF24L01_Send_Packet(nrf24_t nrf24, uint8_t *data, uint8_t len, uint8_t pipe, ack_mode_et ack_mode);
void nRF24L01_Set_Role_Mode(nrf24_t nrf24, nrf24_role_et mode);
void nRF24L01_Ensure_RWW_Features_Activated(nrf24_t nrf24);
int nRF24L01_Run(nrf24_t nrf24);

// bsp_nrf24l01_spi 文件中函数声明
int nRF24L01_SPI_Init(nrf24_port_api_t port_api);
int nRF24L01_IQR_GPIO_Config(nrf24_port_api_t port_api);

// bsp_nrf24l01_message 文件中函数声明
void nrf24l01_order_to_pipe(nrf24_t nrf24, uint8_t order, nrf24_pipe_et pipe_num);

// 以下是寄存器列表 ---------------------------------------------------------------------------------------------

// 命令映射
#define NRF24CMD_R_REG           0x00  // 读寄存器
#define NRF24CMD_W_REG           0x20  // 写寄存器
#define NRF24CMD_R_RX_PAYLOAD    0x61  // 读接收缓冲区
#define NRF24CMD_W_TX_PLOAD_ACK  0xA0  // 写发送缓冲区（必须对方应答）
#define NRF24CMD_W_TX_PLOAD_NACK 0xB0  // 写发送缓冲区（无需对方应答）
#define NRF24CMD_FLUSH_TX        0xE1  // 清空发送FIFO
#define NRF24CMD_FLUSH_RX        0xE2  // 清空接收FIFO
#define NRF24CMD_REUSE_TX_PL     0xE3  // PTX模式下使用，重装载发送缓冲区
#define NRF24CMD_ACTIVATE        0x50  // 使能命令，后接数据 0x73
#define NRF24CMD_R_RX_PL_WID     0x60  // 读顶层接收FIFO大小
#define NRF24CMD_W_ACK_PAYLOAD   0xA8  // RX模式下使用，写应答发送缓冲区

// 寄存器映射
#define NRF24REG_CONFIG          0x00  // 配置收发状态，CRC校验模式以及收发状态响应方式
#define NRF24REG_EN_AA           0x01  // 自动应答功能设置
#define NRF24REG_EN_RXADDR       0x02  // 可用信道设置
#define NRF24REG_SETUP_AW        0x03  // 收发地址宽度设置
#define NRF24REG_SETUP_RETR      0x04  // 自动重发功能设置
#define NRF24REG_RF_CH           0x05  // 工作频率设置
#define NRF24REG_RF_SETUP        0x06  // 发射速率、功耗功能设置
#define NRF24REG_STATUS          0x07  // 状态寄存器
#define NRF24REG_OBSERVE_TX      0x08  // 发送监测功能
#define NRF24REG_RPD             0x09  // 接收功率检测
#define NRF24REG_RX_ADDR_P0      0x0A  // 频道0接收数据地址
#define NRF24REG_RX_ADDR_P1      0x0B  // 频道1接收数据地址
#define NRF24REG_RX_ADDR_P2      0x0C  // 频道2接收数据地址
#define NRF24REG_RX_ADDR_P3      0x0D  // 频道3接收数据地址
#define NRF24REG_RX_ADDR_P4      0x0E  // 频道4接收数据地址
#define NRF24REG_RX_ADDR_P5      0x0F  // 频道5接收数据地址
#define NRF24REG_TX_ADDR         0x10  // 发送地址寄存器
#define NRF24REG_RX_PW_P0        0x11  // 接收频道0接收数据长度
#define NRF24REG_RX_PW_P1        0x12  // 接收频道1接收数据长度
#define NRF24REG_RX_PW_P2        0x13  // 接收频道2接收数据长度
#define NRF24REG_RX_PW_P3        0x14  // 接收频道3接收数据长度
#define NRF24REG_RX_PW_P4        0x15  // 接收频道4接收数据长度
#define NRF24REG_RX_PW_P5        0x16  // 接收频道5接收数据长度
#define NRF24REG_FIFO_STATUS     0x17  // FIFO栈入栈出状态寄存器设置
#define NRF24REG_DYNPD           0x1C  // 动态数据包长度
#define NRF24REG_FEATURE         0x1D  // 特点寄存器

// 寄存器功能位掩码部分映射
// CONFIG
#define NRF24BITMASK_RX_DR       ((uint8_t)(1<<6))  // 接收完成中断使能位
#define NRF24BITMASK_TX_DS       ((uint8_t)(1<<5))  // 发送完成中断使能位
#define NRF24BITMASK_MAX_RT      ((uint8_t)(1<<4))  // 达最大重发次数中断使能位
#define NRF24BITMASK_EN_CRC      ((uint8_t)(1<<3))  // CRC使能位
#define NRF24BITMASK_CRCO        ((uint8_t)(1<<2))  // CRC编码方式 （1B or 2B）
#define NRF24BITMASK_PWR_UP      ((uint8_t)(1<<1))  // 上（掉）电
#define NRF24BITMASK_PRIM_RX     ((uint8_t)(1))     // PR（T）X
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











#endif /* APPLICATIONS_MACBSP_BSP_NRF24L01_DRIVER_H_ */
