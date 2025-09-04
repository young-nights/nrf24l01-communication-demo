/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-03     18452       the first version
 */
#include "bsp_nrf24l01_driver.h"




/***
 * @brief 配置nRF24L01的参数值
 * @note  以下参数请所代表的值详见"nRF24L01P Datasheet.pdf"
 */
void nRF24L01_Param_Config(nrf24_param_t param)
{
    RT_ASSERT(param != RT_NULL);
    rt_memset(param, 0, sizeof(struct nRF24L01_PARAMETER_STRUCT));

    /* CONFIG */
    param->config.prim_rx       = ROLE_PTX;
    param->config.pwr_up        = 1;
    param->config.crco          = CRC_2_BYTE;
    param->config.en_crc        = 1;
    param->config.mask_max_rt   = 0;
    param->config.mask_tx_ds    = 0;
    param->config.mask_rx_dr    = 0;

    /* EN_AA */
    param->en_aa.p0 = 1;
    param->en_aa.p1 = 1;
    param->en_aa.p2 = 1;
    param->en_aa.p3 = 1;
    param->en_aa.p4 = 1;
    param->en_aa.p5 = 1;

    /* EN_RXADDR */
    param->en_rxaddr.p0 = RT_TRUE;
    param->en_rxaddr.p1 = RT_TRUE;
    param->en_rxaddr.p2 = RT_FALSE;
    param->en_rxaddr.p3 = RT_FALSE;
    param->en_rxaddr.p4 = RT_FALSE;
    param->en_rxaddr.p5 = RT_FALSE;

    /* SETUP_AW */
    param->setup_aw.aw = 3;

    /* SET_RETR */
    param->setup_retr.arc = 15;
    param->setup_retr.ard = ADR_2Mbps;

    /* RF_CH */
    param->rf_ch.rf_ch = 100; /*! 无线频道设为 100（2.500 GHz） */

    /* RF_SETUP */
    param->rf_setup.rf_pwr      = RF_POWER_0dBm;
    param->rf_setup.rf_dr_high  = 0;
    param->rf_setup.pll_lock    = 0;
    param->rf_setup.rf_dr_low   = 1;
    param->rf_setup.cont_wave   = 0;

    /* DYNPD */
    param->dynpd.p0 = 1;
    param->dynpd.p1 = 1;
    param->dynpd.p2 = 1;
    param->dynpd.p3 = 1;
    param->dynpd.p4 = 1;
    param->dynpd.p5 = 1;

    /* FEATURE */
    param->feature.en_dyn_ack = 1;
    param->feature.en_ack_pay = 1;
    param->feature.en_dpl     = 1;


    for(int16_t i = 0; i < 5; i++){
        param->txaddr[i] = i;
        param->rx_addr_p0[i] = i;
        param->rx_addr_p1[i] = i + 1;
    }
    param->rx_addr_p2 = 2;
    param->rx_addr_p3 = 3;
    param->rx_addr_p4 = 4;
    param->rx_addr_p5 = 5;
}





/**
 *  @brief  在不依赖任何上层状态的前提下，用最简短的 SPI 读写回路验证“MCU ↔ NRF24L01”硬件连接是否正常
 *  @note   检测思路（经典“回环写读”）
 */
int nRF24L01_Check_SPI_Community(nrf24_t port_ops)
{
    /***
     * 1. 创建两个数组缓冲区，一个用来备份当前默认地址数据，一个用于存储测试数据
     *    test_addr[5]  用于存储测试数据
     *    backup_addr[5]用于存储原始数据，测试完后用于恢复现场
     *    send_empty    用于触发连续读取或者写入的变量
     *
     * 2. 读取 NRF24REG_RX_ADDR_P1 寄存器指令中的地址数据，存入backup_addr
     *
     * 3. 写入 NRF24REG_RX_ADDR_P1 寄存器指定的地址数据
     *
     * 4. 清空 test_addr[5] 为读回刚刚写入的数据做准备
     *
     * 5. 读取 NRF24REG_RX_ADDR_P1 寄存器指令中的地址数据，存入test_addr
     *
     * 6. 和 backup_addr[5]中的原始地址数据进行数据对比，如果出错代表SPI写入有误
     *
     * 7. 恢复现场
     */

    RT_ASSERT(port_ops != RT_NULL);

    // 1. 创建变量缓冲区
    uint8_t test_addr[5]    = { 1,2,3,4,5 };
    uint8_t backup_addr[5]  = { 0 };
    uint8_t send_cmd;

    // 2. 读取 rx_addr_p1[5] 的地址数据
    send_cmd = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P1;
    port_ops->nrf24_ops.nrf24_send_then_recv(&port_ops->port_api, &send_cmd, 1, backup_addr, 5);

    // 3. 写入测试
    send_cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P1;
    port_ops->nrf24_ops.nrf24_send_then_send(&port_ops->port_api, &send_cmd, 1, test_addr, 5);

    // 4. 清零 test_addr[5] , 用于读回刚刚写入的数据
    rt_memset(test_addr, 0, sizeof(test_addr));

    // 5. 重复一遍步骤2.
    send_cmd = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P1;
    port_ops->nrf24_ops.nrf24_send_then_recv(&port_ops->port_api, &send_cmd, 1, test_addr, 5);

    // 6. 和 backup_addr[5]中的原始地址数据进行数据对比，如果出错代表SPI写入有误
    for (int i = 0; i < 5; i++)
    {
        if (test_addr[i] != i+1){
            return RT_ERROR;
        }
    }

    // 7. 恢复现场
    send_cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P1;
    port_ops->nrf24_ops.nrf24_send_then_send(&port_ops->port_api, &send_cmd, 1, backup_addr, 5);

    return RT_EOK;
}





/**
 *  @brief
 *  @note
 */
int nRF24L01_Update_Parameter(nrf24_t nrf24)
{
    uint8_t tmp;

    rt_kprintf("----------------------------------\r\n");

    // 1. 先进入掉电模式
    nrf24_enter_power_down_mode(nrf24);

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
}










//--------------------------------------------------------------------------------------


/***
 * @param  reg_addr: 要读的寄存器地址
 * @return 返回读取到的寄存器中的值
 */
uint8_t __read_reg_data(nrf24_t nrf24, uint8_t reg_addr)
{

    uint8_t cmd, rx_empty = 0;

    cmd = NRF24CMD_R_REG | reg_addr;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &rx_empty, 1);

    return rx_empty;
}


/****
 * @param reg_addr: 要写的寄存器地址
 *        data    : 要写的数据
 * @note  NULL
 */
void __write_reg_data(nrf24_t nrf24, uint8_t reg_addr, uint8_t data)
{
    uint8_t empty_buf[2];

    empty_buf[0] = NRF24CMD_W_REG | reg_addr;
    empty_buf[1] = data;
    nrf24->nrf24_ops.nrf24_write(&nrf24->port_api, &empty_buf[0], sizeof(empty_buf));
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
void __write_reg_bits(nrf24_t nrf24, uint8_t reg_addr, uint8_t mask, uint8_t value)
{
    uint8_t tmp, tidx;

    for (tidx = 0; tidx < 8; tidx++)
    {
        if (mask & (1 << tidx))
            break;
    }
    tmp = ~mask & __read_reg_data(nrf24, reg_addr);
    tmp |= mask & (value << tidx);
    __write_reg_data(nrf24, reg_addr, tmp);
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
uint8_t read_status(nrf24_t nrf24)
{
    return __read_reg_data(nrf24, NRF24REG_STATUS);
}

/***
 * @brief   用来清除状态寄存器中的指定位
 * @param   bitmask   寄存器功能掩码(主要是以下三个)
 *          NRF24BITMASK_RX_DR      ((uint8_t)(1<<6))  // 接收完成中断使能位
 *          NRF24BITMASK_TX_DS      ((uint8_t)(1<<5))  // 发送完成中断使能位
 *          NRF24BITMASK_MAX_RT     ((uint8_t)(1<<4))  // 达最大重发次数中断使能位
 */
void clear_status(nrf24_t nrf24, uint8_t bitmask)
{
    __write_reg_data(nrf24, NRF24REG_STATUS, bitmask);
}


/***
 * @brief   把 OBSERVE_TX 里的丢包计数和重发计数清零
 * @param   OBSERVE_TX 寄存器
 *
 *          | 位  |   字段  |                       含义                   |
 *          | --- | --------- | ------------------------------------------ |
 *          | 7:4 | PLOS\_CNT | 发送失败导致丢包的计数值（到达 15 后不再增加） |
 *          | 3:0 | ARC\_CNT  | 最近一次发送时的自动重发次数                                 |
 *
 * @note    这个函数的作用等价于 芯片收到后把 PLOS_CNT 和 ARC_CNT 全部清零
 *                      即，SPI 发送 0x08 0x00（写 OBSERVE_TX 命令 + 数据 0）
 */
void clear_observe_tx(nrf24_t nrf24)
{
    __write_reg_data(nrf24, NRF24REG_OBSERVE_TX, 0);
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
uint8_t read_top_rxfifo_width(nrf24_t nrf24)
{
    uint8_t cmd = NRF24CMD_R_RX_PL_WID;


    NRF24_HALPORT_SEND_THEN_RECV(&tmp, 1, &tmp, 1);

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









