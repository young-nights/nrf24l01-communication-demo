/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-30     18452       the first version
 */
#include <bsp_nrf24l01_driver.h>
#include "bsp_nrf24l01_spi.h"
#include <rtthread.h>


// 参数设置 ------------------------------------------------------------------------------------------
struct nrf24_onchip_cfg
{
    struct {
        uint8_t prim_rx     :1;
        uint8_t pwr_up      :1;
        uint8_t crco        :1;
        uint8_t en_crc      :1;
        uint8_t mask_max_rt :1;
        uint8_t mask_tx_ds  :1;
        uint8_t mask_rx_dr  :1;
    } config;

    struct {
        uint8_t p0          :1;
        uint8_t p1          :1;
        uint8_t p2          :1;
        uint8_t p3          :1;
        uint8_t p4          :1;
        uint8_t p5          :1;
    } en_aa;

    struct {
        uint8_t p0          :1;
        uint8_t p1          :1;
        uint8_t p2          :1;
        uint8_t p3          :1;
        uint8_t p4          :1;
        uint8_t p5          :1;
    } en_rxaddr;

    struct {
        uint8_t aw          :2;
    } setup_aw;

    struct {
        uint8_t arc         :4;
        uint8_t ard         :4;
    } setup_retr;

    struct {
        uint8_t rf_ch       :7;
    } rf_ch;

    struct {
        uint8_t lna_hcurr   :1;
        uint8_t rf_pwr      :2;
        uint8_t rf_dr       :1;
        uint8_t pll_lock    :1;
    } rf_setup;

    struct {
        uint8_t p0          :1;
        uint8_t p1          :1;
        uint8_t p2          :1;
        uint8_t p3          :1;
        uint8_t p4          :1;
        uint8_t p5          :1;
    } dynpd;

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

    uint8_t tx_addr[5];

}ALIGN(1);





/***
 * @param   nrf24 : 结构体句柄
 *          reg   : 要读的寄存器地址
 * @return  返回读取到的寄存器中的值
 */
static uint8_t __read_reg(nrf24_t nrf24, uint8_t reg)
{

    uint8_t tmp, rtmp = 0;

    // 构造命令字节：读寄存器命令 + 寄存器地址
    tmp = NRF24CMD_R_REG | reg;
    // SPI发送命令，并接收1字节返回值
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, &rtmp, 1);

    return rtmp;
}



/****
 * @param nrf24 : 结构体句柄
 *        reg   : 要写的寄存器地址
 *        data  ：要写的数据
 * @note  NULL
 */
static void __write_reg(nrf24_t nrf24, uint8_t reg, uint8_t data)
{
    uint8_t tmp[2];

    // 构造命令字节：写寄存器命令 + 寄存器地址
    tmp[0] = NRF24CMD_W_REG | reg;
    // 要写入的数据
    tmp[1] = data;
    // SPI发送2字节（命令+数据）
    NRF24_HALPORT_WRITE(&tmp[0], 2);
}


/**
 * @brief   Treat the specified continuous bit as a whole and then set its value
 * @param   nrf24_t ：结构体句柄
 *          reg     ：寄存器地址
 *          mask    : 需要操作的位掩码（连续的 1）
 *          value   : 要写入的新值（0或者1，已对齐到最低位）
 *
 * @note    为什么要专门写这个函数？
 *
 *          答：NRF24L01 的寄存器通常一个字节里挤了很多功能位：
 *          -----------------------------------------------------------------------
 *          | 寄存器   | bit7 | bit6 | bit5 | bit4 | bit3 | bit2    | bit1 | bit0  |
 *          | ------ | ---- | ---- | ---- | ---- | ---- | ------- | ---- | ------- |
 *          | CONFIG | –    | –    | –    | –    | –    | EN\_CRC | CRCO | PWR\_UP |
 *
 *          如果你想 只把 PWR_UP 置 1，但 不能影响 EN_CRC、CRCO 等其它位，
 *          就需要“读-改-写”三步，而不是直接写 0x01（那样会把 EN_CRC、CRCO 清 0）
 */
static void __write_reg_bits(nrf24_t nrf24, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t tmp, tidx;

    for (tidx = 0; tidx < 8; tidx++)
    {
        if (mask & (1 << tidx))
            break;
    }
    tmp = ~mask & __read_reg(nrf24, reg);
    tmp |= mask & (value << tidx);
    __write_reg(nrf24, reg, tmp);
}


/***
 * @brief 用来查看芯片当前状态
 *         ----------------------------------------------
 *        | 位名               | 含义                   |
 *        | ------------------ | ---------------------  |
 *        | bit7 (RX\_DR)      | 收到数据中断标志                |
 *        | bit6 (TX\_DS)      | 发送完成中断标志                |
 *        | bit5 (MAX\_RT)     | 达到最大重发次数中断标志 |
 *        | bit4:1 (RX\_P\_NO) | 当前接收 FIFO 中的管道号 |
 *        | bit0 (TX\_FULL)    | TX FIFO 已满                      |
 *
 * @return status
 */
static uint8_t read_status(nrf24_t nrf24)
{
    return __read_reg(nrf24, NRF24REG_STATUS);
}

/***
 * @brief   用来清除状态寄存器中的指定位
 * @param   bitmask   寄存器功能掩码(主要是以下三个)
 *          NRF24BITMASK_RX_DR      ((uint8_t)(1<<6))  // 接收完成中断使能位
 *          NRF24BITMASK_TX_DS      ((uint8_t)(1<<5))  // 发送完成中断使能位
 *          NRF24BITMASK_MAX_RT     ((uint8_t)(1<<4))  // 达最大重发次数中断使能位
 */
// bit: RX_DR, TX_DS, MAX_RT
static void clear_status(nrf24_t nrf24, uint8_t bitmask)
{
    __write_reg(nrf24, NRF24REG_STATUS, bitmask);
}

/***
 * @brief   把 OBSERVE_TX 里的丢包计数和重发计数清零
 * @param   OBSERVE_TX 寄存器
 *
<<<<<<< Updated upstream
 *          | 位  |   字段  |                       含义                   |
=======
 *          | 位  |   字段    |                       含义                 |
>>>>>>> Stashed changes
 *          | --- | --------- | ------------------------------------------ |
 *          | 7:4 | PLOS\_CNT | 发送失败导致丢包的计数值（到达 15 后不再增加） |
 *          | 3:0 | ARC\_CNT  | 最近一次发送时的自动重发次数                                 |
 *
 * @note    这个函数的作用等价于 芯片收到后把 PLOS_CNT 和 ARC_CNT 全部清零
 *          即，SPI 发送 0xE8 0x00（写 OBSERVE_TX 命令 + 数据 0）
 */
static void clear_observe_tx(nrf24_t nrf24)
{
    __write_reg(nrf24, NRF24REG_OBSERVE_TX, 0);
}


/***
 * @brief   读取 RX FIFO 顶部数据包长度
 * @note    NRF24L01 在接收模式下会把每个到达的数据包先压入 RX FIFO
 *          每个包在 FIFO 里占用的字节数 并不固定（除非关掉了 Dynamic Payload）
 *          因此，在真正读出数据之前，需要先知道 “当前 FIFO 最顶上的那个包到底有多少字节”
 *
 *          芯片专门为此留了一条命令:
 *          命令字节：0x60（宏名 NRF24CMD_R_RX_PL_WID）
 *          发送该命令后，芯片会立即返回 1 字节——该包的数据长度
 */
static uint8_t read_top_rxfifo_width(nrf24_t nrf24)
{
    // = 0x60
    uint8_t tmp = NRF24CMD_R_RX_PL_WID;


    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, // 发送 1 字节命令
                                 &tmp, 1);// 同时接收 1 字节长度

    return tmp;// 返回长度
}


/***
 * @brief   让 NRF24L01 进入掉电模式（Power-Down）
 * @note    将 CONFIG 寄存器的 PWR_UP 位清 0 即可立即进入功耗最低的掉电状态;
 *          此时芯片停止所有射频活动，SPI 仍可访问寄存器，典型电流仅 900 nA;
 *          唤醒时再将 PWR_UP 置 1，并等待 1.5 ms Tpd2stby 延时即可
 */
void nrf24_enter_power_down_mode(nrf24_t nrf24)
{
    __write_reg_bits(nrf24, NRF24REG_CONFIG, NRF24BITMASK_PWR_UP, 0);
}

/***
 * @brief   让 NRF24L01 退出掉电模式，进入上电待机状态（Standby-I）
 * @note    将 CONFIG 寄存器的 PWR_UP 置 1 后，芯片从 Power-Down 唤醒;
 *          必须等待至少 1.5 ms（Tpd2stby），晶振稳定后才能进行 CE 拉高、
 *          发送或接收等操作。此时功耗约 26 µA（Standby-I）
 */
void nrf24_enter_power_up_mode(nrf24_t nrf24)
{
    __write_reg_bits(nrf24, NRF24REG_CONFIG, NRF24BITMASK_PWR_UP, 1);
}


/***
 * @brief   把用户缓冲区 buf 里的 len 个字节写入芯片的 TX FIFO，作为下一包待发送的数据
 * @note    NRF24L01+ 的 TX FIFO 最多能存 3 包待发数据（每包 ≤32 B）
 *          写 FIFO 之前要先发命令 0xA0（宏名 NRF24CMD_W_TX_PAYLOAD）
 *          写完数据后，只要拉高 CE，芯片就会按配置把最前面的那包发出去
 */
static void write_tx_payload(nrf24_t nrf24, const uint8_t *buf, uint8_t len)
{
    // 0xA0
    uint8_t tmp = NRF24CMD_W_TX_PAYLOAD;
    // 先发 1 B 命令，再发 len B 数据
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, buf, len);
}

/***
 * @brief   把 buf 里的 len 字节写入指定管道 pipe 的 ACK Payload 缓冲区，
 *          使芯片在下次收到该管道的数据后，能够随 ACK 帧自动把这段数据回传给发送端
 *
 * @note    ACK Payload 是 NRF24L01+ 的 “带数据应答” 功能:
 *          接收端收到数据后，不必切换到发送模式，就能把最多 32 B 的数据随 ACK 帧一起送回发送端;
 *
 *          每条管道（0–5）都有独立的 ACK Payload 缓冲区；写之前需要先发送 命令 (0xA8 | pipe)
 *          前提：
 *                  1. 芯片已开启动态载荷（FEATURE.EN_DPL = 1）
 *                  2. 对应管道的 ENAA_Px 和 DYNPD 已置位
 *
 */
static void write_ack_payload(nrf24_t nrf24, uint8_t pipe, const uint8_t *buf, uint8_t len)
{
    uint8_t tmp;

    if (pipe > 5)
        return;

    tmp = NRF24CMD_W_ACK_PAYLOAD | pipe;
    // 先发送 1 B 命令字，告诉芯片“接下来是给管道 pipe 的 ACK 数据
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, buf, len);
}



/***
 * @brief   把 RX FIFO 最顶部那包数据真正读出来，并拷贝到用户提供的缓冲区 buf 中，长度为 len 字节
 *
 * @note    NRF24L01+ 收到数据后，先整包压进 RX FIFO
 *          想拿到数据，必须先通过 R_RX_PAYLOAD 命令（0x61）把整包 顺序读出，读完这包后芯片会自动把该包从 FIFO 弹出
 *
 */
static void read_rx_payload(nrf24_t nrf24, uint8_t *buf, uint8_t len)
{
    uint8_t tcmd;

    if ((len > 32) || (len == 0))
        return;

    tcmd = NRF24CMD_R_RX_PAYLOAD;
    NRF24_HALPORT_SEND_THEN_RECV(&tcmd, 1, buf, len);
}

/***
 * @brief   立即清空 NRF24L01+ 的 TX FIFO，把里面所有待发数据包全部丢弃
 * @note    TX FIFO 最多可缓存 3 包待发数据
 *          当上层想“一键撤销”已写入但尚未发送（或发送失败）的数据时，就发 FLUSH_TX 命令（0xE1）
 *          命令只需 1 字节，无后续数据；芯片收到后瞬间清空 FIFO，并复位对应的状态/指针
 */
static void flush_tx_fifo(nrf24_t nrf24)
{
    uint8_t tmp = NRF24CMD_FLUSH_TX;

    NRF24_HALPORT_WRITE(&tmp, 1);
}

/***
 * @brief   立即清空 NRF24L01+ 的 RX FIFO，把所有已接收但尚未读取的数据包全部丢弃
 * @note    使用场景：
 *          1.初始化时确保接收缓冲区干净
 *          2.出现错误或需要重新开始接收时快速清掉旧数据
 *          3.调试阶段手动复位接收状态
 */
static void flush_rx_fifo(nrf24_t nrf24)
{
    uint8_t tmp = NRF24CMD_FLUSH_RX;

    NRF24_HALPORT_WRITE(&tmp, 1);
}


/***
 * @brief   解锁动态载荷、ACK 带载荷等 RWW（Read-While-Write）扩展功能,并做一次性标记避免重复激活
 * @note    动态载荷    ：Payload 长度可变
 *          ACK带载荷  ：ACK 帧里也能附带数据
 *          为什么要这么做：
 *          1.  芯片上电后，动态载荷（DPL）、ACK 带载荷等功能默认是锁住的
 *          2.  必须先发 ACTIVATE 命令 + 数据 0x73 才能解锁
 *          3.  解锁只需一次；重复发送无意义，因此用 activated_features 标志位记录“是否已经做过”
 */
static void ensure_rww_features_activated(nrf24_t nrf24)
{
    // 如果还没激活过
    if (!nrf24->flags.activated_features)
    {
        // SPI 发 2 字节：0x50 0x73
        uint8_t tmp[2] = {NRF24CMD_ACTIVATE, 0x73};
        NRF24_HALPORT_WRITE(tmp, 2);
        // 标记已激活
        nrf24->flags.activated_features = RT_TRUE;
    }
}

/***
 * @brief   把 整个 NRF24L01 的片上寄存器一次性配置成 ccfg 指定的内容，并确保在 掉电状态下完成，避免寄存器写入过程中出现射频误动作
 * @note    顺序写入：
 *          1.  掉电保护
 *              nrf24_enter_power_down_mode() 先把 PWR_UP 清 0，芯片暂停射频，防止写寄存器时空中乱发数据
 *          2.  解锁高级功能
 *              ensure_rww_features_activated() 只在第一次调用时发 ACTIVATE 0x73，解锁动态载荷、ACK 带载荷等 RWW 特性；
 *              如果已经解锁，函数内会跳过。
 *          3.  批量写控制寄存器
 *              用 __write_reg() 把长度均为 1 字节的寄存器一口气写完
 *          4.  写地址寄存器
 *              TX_ADDR、RX_ADDR_P0/P1 是 5 字节宽，用 SEND_THEN_SEND 一次发 6 字节（命令 + 5 数据）
 *              RX_ADDR_P2~P5 只用到 最低 1 字节（共享高 4 字节），所以各写 1 字节即可
 *          5.  最后写 CONFIG
 *              CONFIG（含 PWR_UP、PRIM_RX 等关键位）放在最后，确保前面所有参数就绪后，再决定芯片是进入 接收模式 还是 发送待机模式
 *          6.  统一返回值
 *              函数固定返回 RT_EOK，表示“配置流程走完，未检测异常”
 */
static int update_onchip_config(nrf24_t nrf24, const struct nrf24_onchip_cfg *ccfg)
{
    uint8_t tmp;

    rt_kprintf("----------------------------------\r\n");

    // 1. 先进入掉电模式
    nrf24_enter_power_down_mode(nrf24);
    ensure_rww_features_activated(nrf24);

    /***
     *  0x01: 设置信道的自动应答模式
     *  ccfg->en_aa.p0 = 1;
     *  ccfg->en_aa.p1 = 1;
     *  ccfg->en_aa.p2 = 1;
     *  ccfg->en_aa.p3 = 1;
     *  ccfg->en_aa.p4 = 1;
     *  ccfg->en_aa.p5 = 1;
     *
     *  0011 1111 --> 0x3F --> 6个信道全部可以自动应答
     */
    __write_reg(nrf24, NRF24REG_EN_AA,      *((uint8_t *)&ccfg->en_aa));
    rt_kprintf("[WRITE]ccfg->en_aa           = 0x%02x.\r\n", *((uint8_t *)&ccfg->en_aa));

    /***
     *  0x02: 设置可用的接收信道
     *  ccfg->en_rxaddr.p0 = ucfg->rxpipe0.bl_enabled = RT_TRUE;
     *  ccfg->en_rxaddr.p1 = ucfg->rxpipe1.bl_enabled = RT_TRUE;
     *  ccfg->en_rxaddr.p2 = ucfg->rxpipe2.bl_enabled = RT_FALSE;
     *  ccfg->en_rxaddr.p3 = ucfg->rxpipe3.bl_enabled = RT_FALSE;
     *  ccfg->en_rxaddr.p4 = ucfg->rxpipe4.bl_enabled = RT_FALSE;
     *  ccfg->en_rxaddr.p5 = ucfg->rxpipe5.bl_enabled = RT_FALSE;
     *
     *  0000 0011 --> 0x03 --> pip1,pipe1这2个信道可以接收
     */
    __write_reg(nrf24, NRF24REG_EN_RXADDR,  *((uint8_t *)&ccfg->en_rxaddr));
    rt_kprintf("[WRITE]ccfg->en_rxaddr       = 0x%02x.\r\n", *((uint8_t *)&ccfg->en_rxaddr));

    /***
     *  0x03: 设置地址宽度，这个指令针对所有信道
     *  ccfg->setup_aw.aw = 3;  -->  0000 0011 --> 0x03 --> 设置为5字节宽度
     */
    __write_reg(nrf24, NRF24REG_SETUP_AW,   *((uint8_t *)&ccfg->setup_aw));
    rt_kprintf("[WRITE]ccfg->setup_aw        = 0x%02x.\r\n", *((uint8_t *)&ccfg->setup_aw));

    /***
     *  0x04: 设置建立自动重发机制
     *  ccfg->setup_retr.ard = 1;   // 500us
     *  ccfg->setup_retr.arc = 15;  // 15 times
     */
    __write_reg(nrf24, NRF24REG_SETUP_RETR, *((uint8_t *)&ccfg->setup_retr));
    rt_kprintf("[WRITE]ccfg->setup_retr      = 0x%02x.\r\n", *((uint8_t *)&ccfg->setup_retr));

    /***
     *  0x05: 设置RF通道
     *  ccfg->rf_ch.rf_ch = ucfg->channel = 100;
     */
    __write_reg(nrf24, NRF24REG_RF_CH,      *((uint8_t *)&ccfg->rf_ch));
    rt_kprintf("[WRITE]ccfg->rf_ch           = 0x%02x.\r\n", *((uint8_t *)&ccfg->rf_ch));

    /***
     *  0x06: 设置RF射频速率等
     *  lna_hcurr= 1
     *  rf_pwr   = ucfg->power  = RF_POWER_0dBm = 3
     *  rf_dr    = ucfg->adr    = ADR_2Mbps     = 1
     *  pll_lock = 0
     */
    __write_reg(nrf24, NRF24REG_RF_SETUP,   *((uint8_t *)&ccfg->rf_setup));
    rt_kprintf("[WRITE]ccfg->rf_setup        = 0x%02x.\r\n", *((uint8_t *)&ccfg->rf_setup));

    /***
     *  0x1C: 设置不同信道数据动态长度使能
     *  ccfg->dynpd.p0 = 1;
     *  ccfg->dynpd.p1 = 1;
     *  ccfg->dynpd.p2 = 1;
     *  ccfg->dynpd.p3 = 1;
     *  ccfg->dynpd.p4 = 1;
     *  ccfg->dynpd.p5 = 1;
     */
    __write_reg(nrf24, NRF24REG_DYNPD,      *((uint8_t *)&ccfg->dynpd));
    rt_kprintf("[WRITE]ccfg->dynpd           = 0x%02x.\r\n", *((uint8_t *)&ccfg->dynpd));

    /***
     *  0x1D: 设置扩展功能
     *  ccfg->feature.en_dyn_ack = 1;
     *  ccfg->feature.en_ack_pay = 1;
     *  ccfg->feature.en_dpl = 1;
     */
    __write_reg(nrf24, NRF24REG_FEATURE,    *((uint8_t *)&ccfg->feature));
    rt_kprintf("[WRITE]ccfg->feature         = 0x%02x.\r\n", *((uint8_t *)&ccfg->feature));

    tmp = NRF24CMD_W_REG | NRF24REG_TX_ADDR;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, ccfg->tx_addr, 5);
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P0;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, ccfg->rx_addr_p0, 5);
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P1;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, ccfg->rx_addr_p1, 5);
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P2;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, &ccfg->rx_addr_p2, 1);
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P3;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, &ccfg->rx_addr_p3, 1);
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P4;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, &ccfg->rx_addr_p4, 1);
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P5;
    NRF24_HALPORT_SEND_THEN_SEND(&tmp, 1, &ccfg->rx_addr_p5, 1);

    __write_reg(nrf24, NRF24REG_CONFIG,     *((uint8_t *)&ccfg->config));
    rt_kprintf("[WRITE]ccfg->config          = 0x%02x.\r\n", *((uint8_t *)&ccfg->config));

    return RT_EOK;
}

/***
 * @brief   把 NRF24L01 当前所有关键寄存器的真实值一次性读出来，塞到 ccfg 结构体里，方便“回读校验”或“现场快照”
 * @note    什么时候用这个函数：
 *          1. 调试：把芯片实际寄存器值打印出来，核对配置是否正确。
 *          2. 保存现场：掉电前先把寄存器快照保存，下次上电恢复。
 *          3. 自检：与“期望配置”做对比，发现差异即报警。
 */
static int read_onchip_config(nrf24_t nrf24, struct nrf24_onchip_cfg *ccfg)
{
    struct nrf24_onchip_cfg real_cfg;
    uint8_t tmp;
    rt_kprintf("----------------------------------\r\n");

    *((uint8_t *)&real_cfg.en_aa)      =  __read_reg(nrf24, NRF24REG_EN_AA);
    rt_kprintf("[READ]real_cfg.en_aa        = 0x%02x.\r\n", *((uint8_t *)&real_cfg.en_aa));

    *((uint8_t *)&real_cfg.en_rxaddr)  =  __read_reg(nrf24, NRF24REG_EN_RXADDR);
    rt_kprintf("[READ]real_cfg.en_rxaddr    = 0x%02x.\r\n", *((uint8_t *)&real_cfg.en_rxaddr));

    *((uint8_t *)&real_cfg.setup_aw)   =  __read_reg(nrf24, NRF24REG_SETUP_AW);
    rt_kprintf("[READ]real_cfg.setup_aw     = 0x%02x.\r\n", *((uint8_t *)&real_cfg.setup_aw));

    *((uint8_t *)&real_cfg.setup_retr) =  __read_reg(nrf24, NRF24REG_SETUP_RETR);
    rt_kprintf("[READ]real_cfg.setup_retr   = 0x%02x.\r\n", *((uint8_t *)&real_cfg.setup_retr));

    *((uint8_t *)&real_cfg.rf_ch)      =  __read_reg(nrf24, NRF24REG_RF_CH);
    rt_kprintf("[READ]real_cfg.rf_ch        = 0x%02x.\r\n", *((uint8_t *)&real_cfg.rf_ch));

    *((uint8_t *)&real_cfg.rf_setup)   =  __read_reg(nrf24, NRF24REG_RF_SETUP);
    rt_kprintf("[READ]real_cfg.rf_setup     = 0x%02x.\r\n", *((uint8_t *)&real_cfg.rf_setup));

    *((uint8_t *)&real_cfg.dynpd)      =  __read_reg(nrf24, NRF24REG_DYNPD);
    rt_kprintf("[READ]real_cfg.dynpd        = 0x%02x.\r\n", *((uint8_t *)&real_cfg.dynpd));

    *((uint8_t *)&real_cfg.feature)    =  __read_reg(nrf24, NRF24REG_FEATURE);
    rt_kprintf("[READ]real_cfg.feature      = 0x%02x.\r\n", *((uint8_t *)&real_cfg.feature));

    *((uint8_t *)&real_cfg.config)     =  __read_reg(nrf24, NRF24REG_CONFIG);
    rt_kprintf("[READ]real_cfg.config       = 0x%02x.\r\n", *((uint8_t *)&real_cfg.config));

    tmp = NRF24CMD_R_REG | NRF24REG_TX_ADDR;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.tx_addr, 5);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P0;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.rx_addr_p0, 5);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P1;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.rx_addr_p1, 5);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P2;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.rx_addr_p2, 1);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P3;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.rx_addr_p3, 1);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P4;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.rx_addr_p4, 1);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P5;
    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, (uint8_t *)&real_cfg.rx_addr_p5, 1);

    rt_memcpy(ccfg, &real_cfg, sizeof(struct nrf24_onchip_cfg));

    return RT_EOK;
}

/***
 * @brief   把芯片里 实际的寄存器值 与 期望的配置 ccfg 逐字节比对，完全一致返回 RT_EOK，否则返回 RT_ERROR，用于快速自检
 */
static int check_onchip_config(nrf24_t nrf24, const struct nrf24_onchip_cfg *ccfg)
{
    struct nrf24_onchip_cfg real_cfg;

    read_onchip_config(nrf24, &real_cfg);

    if (rt_memcmp(&real_cfg, ccfg, sizeof(struct nrf24_onchip_cfg)) == 0)
        return RT_EOK;
    else
        return RT_ERROR;
}

/***
 * @brief   把用户高层配置 ucfg 翻译成芯片寄存器级结构体 ccfg，
 *          先填“安全默认”，再用用户参数覆盖，最终得到一份可直接烧录到 NRF24L01 的完整寄存器表
 */
static int build_onchip_config(struct nrf24_onchip_cfg *ccfg, const struct nrf24_cfg *ucfg)
{
    rt_memset(ccfg, 0, sizeof(struct nrf24_onchip_cfg));

    /* Default config */
    ccfg->setup_retr.ard = 1;   // 500us
    ccfg->setup_retr.arc = 15;  // 15 times
    ccfg->setup_aw.aw = 3;      // 5-byte address width

    ccfg->rf_setup.pll_lock = 0;
    ccfg->rf_setup.lna_hcurr = 1;
    ccfg->rf_setup.rf_pwr = ucfg->power;
    ccfg->rf_setup.rf_dr = ucfg->adr;

    ccfg->en_aa.p0 = 1;
    ccfg->en_aa.p1 = 1;
    ccfg->en_aa.p2 = 1;
    ccfg->en_aa.p3 = 1;
    ccfg->en_aa.p4 = 1;
    ccfg->en_aa.p5 = 1;

    ccfg->dynpd.p0 = 1;
    ccfg->dynpd.p1 = 1;
    ccfg->dynpd.p2 = 1;
    ccfg->dynpd.p3 = 1;
    ccfg->dynpd.p4 = 1;
    ccfg->dynpd.p5 = 1;

    ccfg->feature.en_dyn_ack = 1;
    ccfg->feature.en_ack_pay = 1;
    ccfg->feature.en_dpl = 1;
    /* END Default config*/


    if (ucfg->_irq_pin == nRF24_PIN_NONE)
    {
        ccfg->config.mask_rx_dr = 1;
        ccfg->config.mask_tx_ds = 1;
        ccfg->config.mask_max_rt = 1;
    }

    ccfg->config.pwr_up = 1;
    ccfg->config.prim_rx = ucfg->role;
    ccfg->config.en_crc = 1;
    ccfg->config.crco = ucfg->crc;

    ccfg->rf_ch.rf_ch = ucfg->channel;

    rt_memcpy(ccfg->tx_addr, ucfg->txaddr, sizeof(ucfg->txaddr));

    ccfg->en_rxaddr.p0 = ucfg->rxpipe0.bl_enabled;
    ccfg->en_rxaddr.p1 = ucfg->rxpipe1.bl_enabled;
    ccfg->en_rxaddr.p2 = ucfg->rxpipe2.bl_enabled;
    ccfg->en_rxaddr.p3 = ucfg->rxpipe3.bl_enabled;
    ccfg->en_rxaddr.p4 = ucfg->rxpipe4.bl_enabled;
    ccfg->en_rxaddr.p5 = ucfg->rxpipe5.bl_enabled;

    rt_memcpy(ccfg->rx_addr_p0, ucfg->rxpipe0.addr, sizeof(ucfg->rxpipe0.addr));
    rt_memcpy(ccfg->rx_addr_p1, ucfg->rxpipe1.addr, sizeof(ucfg->rxpipe1.addr));
    ccfg->rx_addr_p2 = ucfg->rxpipe2.addr;
    ccfg->rx_addr_p3 = ucfg->rxpipe3.addr;
    ccfg->rx_addr_p4 = ucfg->rxpipe4.addr;
    ccfg->rx_addr_p5 = ucfg->rxpipe5.addr;

    return RT_EOK;
}

/**
 *  Test the connection with NRF24
 *  @brief  在不依赖任何上层状态的前提下，用最简短的 SPI 读写回路验证“MCU ↔ NRF24L01”硬件连接是否正常
 *  @note   检测思路（经典“回环写读”）：
 *          1. 备份现场：把 RX_ADDR_P1 原有 5 字节地址读到 backup_addr[]，测试完成后原样恢复
 *          2. 写入测试模式:向 RX_ADDR_P1 写入固定模式 {1,2,3,4,5}
 *          3. 立即读回
 *          4. 现场还原
 */
static int check_halport(nrf24_port_t halport)
{
    // addr 固定填 1,2,3,4,5，用来当测试数据
    // backup_addr 用来临时保存原来的地址，测试完后好恢复现场
    uint8_t addr[5] = {1,2,3,4,5}, backup_addr[5];
    uint8_t tmp;

    RT_ASSERT(halport != RT_NULL);
    RT_ASSERT(halport->nrf24_ops != RT_NULL);

    // 备份 RX_ADDR_P1 原有地址 ： 通过 SPI 先发 1 字节命令，再连续读回 5 字节，存到 backup_addr 里
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P1;
    halport->nrf24_ops->nrf24_send_then_recv(halport, &tmp, 1, backup_addr, 5);

    // 写入测试模式，把命令字节拼成 "写 NRF24REG_RX_ADDR_P1 寄存器"（0x2A）
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P1;
    // 通过 SPI 先发 1 字节命令，再把 5 字节测试数据 {1,2,3,4,5} 写进 NRF24REG_RX_ADDR_P1
    halport->nrf24_ops->nrf24_send_then_send(halport, &tmp, 1, addr, 5);

    // 清零 addr 数组，准备读回
    rt_memset(addr, 0, 5);

    // 读回刚才写入的地址,再次拼命令：读 NRF24REG_RX_ADDR_P1
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P1;
    // 通过 SPI 把 NRF24REG_RX_ADDR_P1 的 5 字节读回，放到 addr
    halport->nrf24_ops->nrf24_send_then_recv(halport, &tmp, 1, addr, 5);

    // 逐字节比对
    for (int i = 0; i < 5; i++)
    {
        if (addr[i] != i+1){
            return RT_ERROR;
        }
    }

    // 恢复现场
    tmp = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P1;
    // 把刚才备份的原始地址重新写回 RX_ADDR_P1，保证测试不会破坏原有配置
    halport->nrf24_ops->nrf24_send_then_send(halport, &tmp, 1, backup_addr, 5);

    return RT_EOK;
}

/**
 * Set the user-oriented configuration as the default
 * @brief  给用户提供一份开箱即用的默认配置文件
 */
int nrf24_fill_default_config_on(nrf24_cfg_t cfg)
{
    RT_ASSERT(cfg != RT_NULL);

    // 先把整个配置结构体全部清 0，避免残留旧数据
    rt_memset(cfg, 0, sizeof(struct nrf24_cfg));

    cfg->power = RF_POWER_0dBm; // 发射功率设成 0 dBm（中等强度）
    cfg->crc = CRC_2_BYTE;      // CRC 校验长度设成 2 字节，更可靠
    cfg->adr = ADR_2Mbps;       // 空中速率 2 Mbps，兼顾速度与距离
    cfg->channel = 100;         // 无线频道设为 100（2.500 GHz），远离常用 Wi-Fi 频段
    cfg->role = ROLE_NONE;      // 角色先标记为‘未设定’，后面再决定是发送端还是接收端

    /***
     * 给 5字节地址逐字节赋值
     * 发送端地址:txaddr[i] = {0x00,0x01,0x02,0x03,0x04}
     * 信道0地址: rxpipe0.addr[i] = {0x00,0x01,0x02,0x03,0x04}
     * 信道1地址: rxpipe1.addr[i] = {0x01,0x02,0x03,0x04,0x05}
     * 信道2地址: rxpipe2.addr[i] = 0x02
     * 信道3地址: rxpipe3.addr[i] = 0x03
     * 信道4地址: rxpipe4.addr[i] = 0x04
     * 信道5地址: rxpipe5.addr[i] = 0x05
     */
    for (int i = 0; i < 5; i++)
    {
        cfg->txaddr[i] = i;
        cfg->rxpipe0.addr[i] = i;
        cfg->rxpipe1.addr[i] = i+1;
    }
    cfg->rxpipe2.addr = 2;
    cfg->rxpipe3.addr = 3;
    cfg->rxpipe4.addr = 4;
    cfg->rxpipe5.addr = 5;

    /***
     * 通过以下指令对信道进行使能/失能
     * 启用 管道 0 和管道 1，可以接收数据
     * 禁用 管道 2~5，默认不接收数据（需要时再手动打开）
     */
    cfg->rxpipe0.bl_enabled = RT_TRUE;
    cfg->rxpipe1.bl_enabled = RT_TRUE;
    cfg->rxpipe2.bl_enabled = RT_FALSE;
    cfg->rxpipe3.bl_enabled = RT_FALSE;
    cfg->rxpipe4.bl_enabled = RT_FALSE;
    cfg->rxpipe5.bl_enabled = RT_FALSE;

    return RT_EOK;
}

/**
 * @brief  把用户数据写到 TX FIFO（PTX 模式）或 ACK Payload 缓冲区（PRX 模式），并立即触发发送或等待对方读取
 *
 */
int nrf24_send_data(nrf24_t nrf24, uint8_t *data, uint8_t len, uint8_t pipe)
{
    // 一包最多 32 字节，超了就报错
    if (len > 32)
        return RT_ERROR;

    // 如果是发送端（PTX）
    if (nrf24->cfg.role == ROLE_PTX){
        // 把数据直接塞进 TX FIFO，等拉高 CE 就发出去
        write_tx_payload(nrf24, data, len);
    }
    // 如果是接收端（PRX）
    else{
        // 把数据写进指定管道的 ACK Payload 缓冲，下次收到该管道数据就随 ACK 回发
        write_ack_payload(nrf24, pipe, data, len);
        // 释放信号量
        rt_sem_release(nrf24->send_sem);
    }

    return RT_EOK;
}


/**
 * @brief   NRF24L01+ IRQ 引脚的中断服务函数
 * @note    只要 IRQ 电平触发，它就立刻通过信号量 sem 唤醒等待中的线程
 */
void __irq_handler(nrf24_port_t halport)
{
    nrf24_t nrf24 = (nrf24_t)halport;

    rt_sem_release(nrf24->sem);
}

/**
 *  if try to create sem with the existing name, what will happen
 *  @brief
 *  @param  nrf24       : 对象指针
 *          spi_dev_name: SPI设备名字
 *          ce_pin      ： 芯片使能脚 CE 的 GPIO 编号
 *          irq_pin     ： 中断脚 IRQ 的 GPIO 编号
 *          *cb         ： 事件回调函数指针
 *          cfg         : 用户配置结构体
 */
int nrf24_init(nrf24_t nrf24, char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, const nrf24_cfg_t cfg)
{
    // 准备一个寄存器级配置缓存，稍后用来一次性写入芯片
    struct nrf24_onchip_cfg onchip_cfg;

    RT_ASSERT(nrf24 != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    RT_ASSERT(cb != RT_NULL);

    rt_memset(nrf24, 0, sizeof(struct nrf24));

    // 创建一个发送的二值信号量
    nrf24->send_sem = rt_sem_create("nrfsend", 0, RT_IPC_FLAG_FIFO);
    if (nrf24->send_sem == RT_NULL)
    {
        LOG_E("Failed to create sem");
        return RT_ERROR;
    }
    // 如果启用了中断引脚
    if (irq_pin != nRF24_PIN_NONE)
    {
        // 先创建一个中断的二值信号量
        nrf24->sem = rt_sem_create("nrfirq", 0, RT_IPC_FLAG_FIFO);
        if (nrf24->sem == RT_NULL)
        {
            LOG_E("Failed to create sem");
            return RT_ERROR;
        }
        // 启用IRQ的标志位
        nrf24->flags.using_irq = RT_TRUE;
    }
    else
    {
        nrf24->flags.using_irq = RT_FALSE;
    }

    rt_memcpy(&nrf24->cb, cb, sizeof(struct nrf24_callback));
    rt_memcpy(&nrf24->cfg, cfg, sizeof(struct nrf24_cfg));
    nrf24->cfg._irq_pin = irq_pin;

    if (nrf24_port_init(&nrf24->halport, spi_dev_name, ce_pin, irq_pin, __irq_handler) != RT_EOK){
        LOG_E("nRF24L01 port initialize false.\r\n");
        return RT_ERROR;
    }

    /* 通过 SPI 通讯，检测硬件链路是否有问题 */
    if (check_halport(&nrf24->halport) != RT_EOK){
        LOG_E("nRF24L01 check_halport false.\r\n");
        return RT_ERROR;
    }

    /* 配置寄存器参数 */
    if (build_onchip_config(&onchip_cfg, &nrf24->cfg) != RT_EOK){
        LOG_E("nRF24L01 build_onchip_config false.\r\n");
        return RT_ERROR;
    }

    /* 更新寄存器参数 */
    if (update_onchip_config(nrf24, &onchip_cfg) != RT_EOK){
        LOG_E("nRF24L01 update_onchip_config false.\r\n");
        return RT_ERROR;
    }


    /* 检查寄存器参数 */
    if (check_onchip_config(nrf24, &onchip_cfg) != RT_EOK){
        LOG_E("nRF24L01 check_onchip_config false.\r\n");
        return RT_ERROR;
    }

    /* 清空 发送/接收 队列 */
    flush_tx_fifo(nrf24);
    flush_rx_fifo(nrf24);
    /* 清除中断标志位 */
    clear_status(nrf24, NRF24BITMASK_RX_DR | NRF24BITMASK_TX_DS | NRF24BITMASK_MAX_RT);
    clear_observe_tx(nrf24);

    /* 进入上电模式 */
    nrf24_enter_power_up_mode(nrf24);
    /* 拉高CE引脚 -- 接收模式 */
    nrf24->halport.nrf24_ops->nrf24_set_ce(&nrf24->halport);

    LOG_I("Successfully initialized");

    return RT_EOK;
}

/**
 *  @brief  创建一个全新的 NRF24 无线对象
 *  @param  spi_dev_name: SPI设备名字
 *          ce_pin      ： 芯片使能脚 CE 的 GPIO 编号
 *          irq_pin     ： 中断脚 IRQ 的 GPIO 编号
 *          *cb         ： 事件回调函数指针
 *          cfg         : 用户配置结构体
 */
nrf24_t nrf24_create(char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, const nrf24_cfg_t cfg)
{
    RT_ASSERT(cfg != RT_NULL);

    nrf24_t new_nrf24 = (nrf24_t)rt_malloc(sizeof(struct nrf24));
    if (new_nrf24 == RT_NULL)
    {
        rt_free(new_nrf24);
        LOG_E("Failed to allocate memory!");
    }
    else
    {
        if (nrf24_init(new_nrf24, spi_dev_name, ce_pin, irq_pin, cb, cfg) != RT_EOK)
        {
            rt_free(new_nrf24);
            new_nrf24 = RT_NULL;
        }
    }

    if (new_nrf24 == RT_NULL)
        LOG_E("Failed to create nrf24 instance");

    return new_nrf24;
}



int nrf24_default_init(nrf24_t nrf24, char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, nrf24_role_et role)
{
    struct nrf24_cfg cfg;

    /* 先设置默认参数 */
    nrf24_fill_default_config_on(&cfg);
    cfg.role = role;
    /* 进行初始化，并返回值 */
    return nrf24_init(nrf24, spi_dev_name, ce_pin, irq_pin, cb, &cfg);
}



nrf24_t nrf24_default_create(char *spi_dev_name, int ce_pin, int irq_pin, const struct nrf24_callback *cb, nrf24_role_et role)
{
    nrf24_t new_nrf24 = (nrf24_t)rt_malloc(sizeof(struct nrf24));
    if (new_nrf24 == RT_NULL)
    {
        rt_free(new_nrf24);
        LOG_E("Failed to allocate memory!");
    }
    else
    {
        /* 这里只有这个If里面的函数做初始化操作，其他的都是校验与内存的开辟的操作 */
        if (nrf24_default_init(new_nrf24, spi_dev_name, ce_pin, irq_pin, cb, role) != RT_EOK)
        {
            rt_free(new_nrf24);
            new_nrf24 = RT_NULL;
        }
    }

    if (new_nrf24 == RT_NULL)
        LOG_E("Failed to create nrf24 instance");

    return new_nrf24;
}


/**
 * check status and inform
 * @param nrf24 pointer of nrf24 instance
 * @return -x:error   0:nothing   1:tx_done   2:rx_done   3:tx_rx_done
 * @note 这个函数负责轮询/等待中断，把芯片当前状态翻译成上层能看懂的事件
 */
int nrf24_run(nrf24_t nrf24)
{
    int rvl = 0;

    // 1. 如果使用 IRQ，则阻塞等待中断
    if (nrf24->flags.using_irq){
        // 如初始化了 IRQ 引脚，就在这死等中断信号量,收到中断才往下走
        rt_sem_take(nrf24->sem, RT_WAITING_FOREVER);
    }

    // 2. 读 STATUS 并清标志
    nrf24->status = read_status(nrf24);
    // 拿到 STATUS 寄存器值，顺手把 RX_DR 和 TX_DS 两个中断标志清掉，防止重复触发
    clear_status(nrf24, NRF24BITMASK_RX_DR | NRF24BITMASK_TX_DS);

    // 从 STATUS 里解析出是哪条接收管道（0~5）来的数据
    uint8_t pipe = (nrf24->status & NRF24BITMASK_RX_P_NO) >> 1;

    // 3. 角色 = 发送端（PTX）
    if (nrf24->cfg.role == ROLE_PTX)
    {
        /* 3.1 发送失败：达到最大重发次数 */
        if (nrf24->status & NRF24BITMASK_MAX_RT)
        {
            flush_tx_fifo(nrf24);   //  // 把 TX FIFO 清空
            clear_status(nrf24, NRF24BITMASK_MAX_RT);   // 清失败标志
            if(nrf24->cb.tx_done) nrf24->cb.tx_done(nrf24, NRF24_PIPE_NONE); // 回调通知失败
            return -1;
        }

        /* 3.2 收到 ACK 带载荷（PTX 也能收） */
        if (nrf24->status & NRF24BITMASK_RX_DR)
        {
            uint8_t data[32];
            uint8_t len = read_top_rxfifo_width(nrf24); // 先读长度
            read_rx_payload(nrf24, data, len);   // 再读内容
            //  // 回调给应用
            if (nrf24->cb.rx_ind) {
                nrf24->cb.rx_ind(nrf24, data, len, pipe);
            }
            // 标记“收到”
            rvl |= 2;
        }
        /* 3.3 发送完成 */
        if (nrf24->status & NRF24BITMASK_TX_DS)
        {
            // 回调通知成功
            if (nrf24->cb.tx_done) nrf24->cb.tx_done(nrf24, pipe);
            // 标记“发送完成”
            rvl |= 1;
        }
    }
    // 4. 角色 = 接收端（PRX）
    else
    {   /* 4.1 有数据到达 */
        if (pipe <= 5)
        {
            uint8_t data[32];
            // 读包
            uint8_t len = read_top_rxfifo_width(nrf24);
            read_rx_payload(nrf24, data, len);
            // 回调
            if (nrf24->cb.rx_ind) {
                nrf24->cb.rx_ind(nrf24, data, len, pipe);
            }
            // 标记“收到”
            rvl |= 2;
            /* 4.2 如果之前写过 ACK Payload，则顺带通知“ACK 已随包发完” */
            if (rt_sem_trytake(nrf24->send_sem) == RT_EOK)
            {
                // 回调
                if (nrf24->cb.tx_done) {
                    nrf24->cb.tx_done(nrf24, pipe);
                }
                // 标记“ACK 发送完成”
                rvl |= 1;
            }
        }
    }

    return rvl;
}

