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
int nRF24L01_Param_Config(nrf24_param_t param)
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
    param->en_aa.p2 = 0;
    param->en_aa.p3 = 0;
    param->en_aa.p4 = 0;
    param->en_aa.p5 = 0;

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
    param->rf_setup.rf_dr_low   = 0;
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

    return RT_EOK;

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
 *  @brief 把参数更新到芯片的寄存器当中
 *  @note
 */
int nRF24L01_Update_Parameter(nrf24_t nrf24)
{
    uint8_t cmd;

    rt_kprintf("----------------------------------\r\n");


    // 3. 写EN_AA寄存器
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_EN_AA,           *((uint8_t *)&nrf24->nrf24_cfg.en_aa));
    rt_kprintf("[WRITE]ccfg->en_aa           = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.en_aa));

    // 4. 设置可用的接收信道
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_EN_RXADDR,       *((uint8_t *)&nrf24->nrf24_cfg.en_rxaddr));
    rt_kprintf("[WRITE]ccfg->en_rxaddr       = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.en_rxaddr));

    // 5. 设置地址宽度，这个指令针对所有信道
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_SETUP_AW,        *((uint8_t *)&nrf24->nrf24_cfg.setup_aw));
    rt_kprintf("[WRITE]ccfg->setup_aw        = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.setup_aw));

    // 6. 设置建立自动重发机制
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_SETUP_RETR,      *((uint8_t *)&nrf24->nrf24_cfg.setup_retr));
    rt_kprintf("[WRITE]ccfg->setup_retr      = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.setup_retr));

    // 7. RF频段通道设置
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_RF_CH,           *((uint8_t *)&nrf24->nrf24_cfg.rf_ch));
    rt_kprintf("[WRITE]ccfg->rf_ch           = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.rf_ch));

    // 8. 设置RF射频速率等
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_RF_SETUP,        *((uint8_t *)&nrf24->nrf24_cfg.rf_setup));
    rt_kprintf("[WRITE]ccfg->rf_setup        = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.rf_setup));

    // 9. 设置不同信道数据动态长度使能
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_DYNPD,           *((uint8_t *)&nrf24->nrf24_cfg.dynpd));
    rt_kprintf("[WRITE]ccfg->dynpd           = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.dynpd));

    // 10. 设置扩展功能
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_FEATURE,         *((uint8_t *)&nrf24->nrf24_cfg.feature));
    rt_kprintf("[WRITE]ccfg->feature         = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.feature));

    // 11. 设置CRC等基础配置项
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_CONFIG,          *((uint8_t *)&nrf24->nrf24_cfg.config));
    rt_kprintf("[WRITE]ccfg->config          = 0x%02x.\r\n", *((uint8_t *)&nrf24->nrf24_cfg.config));


    cmd = NRF24CMD_W_REG | NRF24REG_TX_ADDR;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, nrf24->nrf24_cfg.txaddr, 5);
    cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P0;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, nrf24->nrf24_cfg.rx_addr_p0, 5);
    cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P1;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, nrf24->nrf24_cfg.rx_addr_p1, 5);
    cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P2;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &nrf24->nrf24_cfg.rx_addr_p2, 1);
    cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P3;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &nrf24->nrf24_cfg.rx_addr_p3, 1);
    cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P4;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &nrf24->nrf24_cfg.rx_addr_p4, 1);
    cmd = NRF24CMD_W_REG | NRF24REG_RX_ADDR_P5;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &nrf24->nrf24_cfg.rx_addr_p5, 1);


    return RT_EOK;
}



/**
 *  @brief 把写入后芯片的配置参数读出来和写入的参数进行对比
 */
int nRF24L01_Read_Onchip_Parameter(nrf24_t nrf24)
{
    struct nRF24L01_PARAMETER_STRUCT real_cfg;
    uint8_t tmp;
    rt_kprintf("----------------------------------\r\n");

    *((uint8_t *)&real_cfg.en_aa)           =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_EN_AA);
    rt_kprintf("[READ]real_cfg.en_aa        = 0x%02x.\r\n", *((uint8_t *)&real_cfg.en_aa));

    *((uint8_t *)&real_cfg.en_rxaddr)       =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_EN_RXADDR);
    rt_kprintf("[READ]real_cfg.en_rxaddr    = 0x%02x.\r\n", *((uint8_t *)&real_cfg.en_rxaddr));

    *((uint8_t *)&real_cfg.setup_aw)        =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_SETUP_AW);
    rt_kprintf("[READ]real_cfg.setup_aw     = 0x%02x.\r\n", *((uint8_t *)&real_cfg.setup_aw));

    *((uint8_t *)&real_cfg.setup_retr)      =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_SETUP_RETR);
    rt_kprintf("[READ]real_cfg.setup_retr   = 0x%02x.\r\n", *((uint8_t *)&real_cfg.setup_retr));

    *((uint8_t *)&real_cfg.rf_ch)           =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_RF_CH);
    rt_kprintf("[READ]real_cfg.rf_ch        = 0x%02x.\r\n", *((uint8_t *)&real_cfg.rf_ch));

    *((uint8_t *)&real_cfg.rf_setup)        =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_RF_SETUP);
    rt_kprintf("[READ]real_cfg.rf_setup     = 0x%02x.\r\n", *((uint8_t *)&real_cfg.rf_setup));

    *((uint8_t *)&real_cfg.dynpd)           =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_DYNPD);
    rt_kprintf("[READ]real_cfg.dynpd        = 0x%02x.\r\n", *((uint8_t *)&real_cfg.dynpd));

    *((uint8_t *)&real_cfg.feature)         =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_FEATURE);
    rt_kprintf("[READ]real_cfg.feature      = 0x%02x.\r\n", *((uint8_t *)&real_cfg.feature));

    *((uint8_t *)&real_cfg.config)          =  nRF24L01_Read_Reg_Data(nrf24, NRF24REG_CONFIG);
    rt_kprintf("[READ]real_cfg.config       = 0x%02x.\r\n", *((uint8_t *)&real_cfg.config));

    tmp = NRF24CMD_R_REG | NRF24REG_TX_ADDR;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.txaddr, 5);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P0;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.rx_addr_p0, 5);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P1;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.rx_addr_p1, 5);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P2;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.rx_addr_p2, 1);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P3;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.rx_addr_p3, 1);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P4;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.rx_addr_p4, 1);
    tmp = NRF24CMD_R_REG | NRF24REG_RX_ADDR_P5;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tmp, 1, (uint8_t *)&real_cfg.rx_addr_p5, 1);


    return RT_EOK;
}





//--------------------------------------------------------------------------------------


/***
 * @param  reg_addr: 要读的寄存器地址
 * @return 返回读取到的寄存器中的值
 */
uint8_t nRF24L01_Read_Reg_Data(nrf24_t nrf24, uint8_t reg_addr)
{

    uint8_t cmd, rx_empty = 0;

    cmd = NRF24CMD_R_REG | reg_addr;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &rx_empty, 1);

    return rx_empty;
}


/****
 * @param  reg_addr: 要写的寄存器地址
 *         data    : 要写的数据
 * @return NULL
 */
void nRF24L01_Write_Reg_Data(nrf24_t nrf24, uint8_t reg_addr, uint8_t data)
{
    uint8_t empty_buf[2];

    empty_buf[0] = NRF24CMD_W_REG | reg_addr;
    empty_buf[1] = data;
    nrf24->nrf24_ops.nrf24_write(&nrf24->port_api, &empty_buf[0], sizeof(empty_buf));
}


/**
 * @brief   Treat the specified continuous bit as a whole and then set its value
 */
void nRF24L01_Write_Reg_Bits(nrf24_t nrf24, uint8_t reg_addr, uint8_t mask, uint8_t value)
{
    uint8_t tmp, tidx;

    for (tidx = 0; tidx < 8; tidx++)
    {
        if (mask & (1 << tidx))
            break;
    }
    tmp = ~mask & nRF24L01_Read_Reg_Data(nrf24, reg_addr);
    tmp |= mask & (value << tidx);
    nRF24L01_Write_Reg_Data(nrf24, reg_addr, tmp);
}

/***
 * @brief  用来查看芯片当前状态
 * @return status
 */
uint8_t nRF24L01_Read_Status_Register(nrf24_t nrf24)
{
    return nRF24L01_Read_Reg_Data(nrf24, NRF24REG_STATUS);
}

/***
 * @brief   用来清除状态寄存器中的指定位
 * @param   bitmask 寄存器功能掩码
 */
void nRF24L01_Clear_Status_Register(nrf24_t nrf24, uint8_t bitmask)
{
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_STATUS, bitmask);
}

/**
  * @brief  NRF24L01清中断标志位
  * @retval NULL
  */
void nRF24L01_Clear_IRQ_Flags(nrf24_t nrf24)
{
    nRF24L01_Clear_Status_Register(nrf24, NRF24BITMASK_RX_DR | NRF24BITMASK_TX_DS | NRF24BITMASK_MAX_RT);
}

/**
  * @brief  读NRF24L01中断状态标志位
  * @retval NRF24BITMASK_RX_DR | NRF24BITMASK_TX_DS | NRF24BITMASK_MAX_RT 的值
  */
rt_uint8_t nRF24L01_Read_IRQ_Status(nrf24_t nrf24)
{
    rt_uint8_t status = nRF24L01_Read_Reg_Data(nrf24, NRF24REG_STATUS);
    status &=  (uint8_t)(NRF24BITMASK_RX_DR | NRF24BITMASK_TX_DS | NRF24BITMASK_MAX_RT);
    return status;
}


/***
 * @brief   把 OBSERVE_TX 里的丢包计数和重发计数清零
 * @return  NULL
 */
void nRF24L01_Clear_Observe_TX(nrf24_t nrf24)
{
    nRF24L01_Write_Reg_Data(nrf24, NRF24REG_OBSERVE_TX, 0);
}


/***
 * @brief   读取 RX FIFO 顶部数据包长度
 * @note    NRF24L01在接收模式下会把每个到达的数据包先压入RX_FIFO,由于FIFO中占用字节数不固定，因此正真读出数据之前需要先知道当前这包数据有多少字节
 */
uint8_t nRF24L01_Read_Top_RXFIFO_Width(nrf24_t nrf24)
{
    uint8_t cmd = NRF24CMD_R_RX_PL_WID;
    uint8_t width_data = 0;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &cmd, 1, &width_data, 1);

    return width_data;
}


/***
 * @brief 让 NRF24L01 进入掉电模式（Power-Down）
 * @note
 */
void nRF24L01_Enter_Power_Down_Mode(nrf24_t nrf24)
{
    nRF24L01_Write_Reg_Bits(nrf24, NRF24REG_CONFIG, NRF24BITMASK_PWR_UP, 0);
}

/***
 * @brief 让 NRF24L01 退出掉电模式，进入上电待机状态（Standby-I）
 * @note
 */
void nRF24L01_Enter_Power_Up_Mode(nrf24_t nrf24)
{
    nRF24L01_Write_Reg_Bits(nrf24, NRF24REG_CONFIG, NRF24BITMASK_PWR_UP, 1);
#define ENABLE_PWR_LOG 0
#if ENABLE_PWR_LOG
    uint8_t config_reg_data = 0;
    config_reg_data = nRF24L01_Read_Reg_Data(nrf24, NRF24REG_CONFIG);
    LOG_I("LOG:%d. raw config_reg_data = 0x%02x .\r\n",Record.ulog_cnt++, config_reg_data);
#endif
}


/***
 * @brief 发送一包数据，若未收到 ACK，会重发（最多 RETR 次）
 */
void nRF24L01_Write_Tx_Payload_Ack(nrf24_t nrf24, const uint8_t *buf, uint8_t len)
{
    uint8_t cmd = NRF24CMD_W_TX_PLOAD_ACK;
    nrf24->nrf24_ops.nrf24_send_then_send(&nrf24->port_api, &cmd, 1, buf, len);
}

/***
 * @brief 发送一包数据，无需等待对方ACK
 */
void nRF24L01_Write_Tx_Payload_NoAck(nrf24_t nrf24, const uint8_t *buf, uint8_t len)
{
    uint8_t cmd = NRF24CMD_W_TX_PLOAD_NACK;
    nrf24->nrf24_ops.nrf24_send_then_send(&nrf24->port_api, &cmd, 1, buf, len);
}



/***
 * @brief   把 buf 里的 len 字节写入指定管道 pipe 的 ACK Payload 缓冲区，使芯片在下次收到该管道的数据后，能够随 ACK 帧自动把这段数据回传给发送端
 * @note    ACK Payload 是 NRF24L01的 “带数据应答” 功能，接收端收到数据后，不必切换到发送模式，就能把最多 32 B 的数据随 ACK 帧一起送回发送端
 */
void nRF24L01_Write_Tx_Payload_InAck(nrf24_t nrf24, uint8_t pipe, const uint8_t *buf, uint8_t len)
{
    uint8_t cmd;

    if (pipe > 5){
        return;
    }

    cmd = NRF24CMD_W_ACK_PAYLOAD | pipe;
    nrf24->nrf24_ops.nrf24_send_then_send(&nrf24->port_api, &cmd, 1, buf, len);
}


/***
 * @brief   把 RX FIFO 最顶部那包数据真正读出来，并拷贝到用户提供的缓冲区 buf 中，长度为 len 字节
 * @note    NRF24L01收到数据后，先整包压进 RX_FIFO,想拿到数据，必须先通过指令把整包顺序读出，读完这包后芯片会自动把该包从 FIFO 弹出
 */
void nRF24L01_Read_Rx_Payload(nrf24_t nrf24, uint8_t *buf, uint8_t len)
{
    uint8_t tcmd;

    if ((len > 32) || (len == 0)){
        return;
    }

    tcmd = NRF24CMD_R_RX_PAYLOAD;
    nrf24->nrf24_ops.nrf24_send_then_recv(&nrf24->port_api, &tcmd, 1, buf, len);
}


/***
 * @brief   立即清空 NRF24L01的 TX FIFO，把里面所有待发数据包全部丢弃
 * @note    TX FIFO 最多可缓存 3 包待发数据
 */
void nRF24L01_Flush_TX_FIFO(nrf24_t nrf24)
{
    uint8_t cmd = NRF24CMD_FLUSH_TX;
    nrf24->nrf24_ops.nrf24_write(&nrf24->port_api, &cmd, 1);
}

/***
 * @brief   立即清空 NRF24L01的 RX FIFO，把所有已接收但尚未读取的数据包全部丢弃
 * @note    使用场景：
 *          1.初始化时确保接收缓冲区干净
 *          2.出现错误或需要重新开始接收时快速清掉旧数据
 *          3.调试阶段手动复位接收状态
 */
void nRF24L01_Flush_RX_FIFO(nrf24_t nrf24)
{
    uint8_t cmd = NRF24CMD_FLUSH_RX;
    nrf24->nrf24_ops.nrf24_write(&nrf24->port_api, &cmd, 1);
}


/***
 * @brief 设置nRF24L01的发送地址
 * @note
 */
void NRF24L01_Set_TxAddr(nrf24_t nrf24, rt_uint8_t *addr_buf, rt_uint8_t length)
{
    length = ( length > 5 ) ? 5 : length;

    for(rt_uint8_t i = 0; i < 5; i++){
        nrf24->nrf24_cfg.txaddr[i] = *(addr_buf + i);
    }

    uint8_t tmp = NRF24CMD_R_REG | NRF24REG_TX_ADDR;
    nrf24->nrf24_ops.nrf24_send_then_send(&nrf24->port_api, &tmp, 1, (uint8_t *)&nrf24->nrf24_cfg.txaddr, 5);
}



/**
 * @brief  把用户数据写到 TX FIFO（PTX 模式）或 ACK Payload 缓冲区（PRX 模式），并立即触发发送或等待对方读取
 *
 */
int nRF24L01_Send_Packet(nrf24_t nrf24, uint8_t *data, uint8_t len, uint8_t pipe, ack_mode_et ack_mode)
{
    if (len > 32){
        LOG_E("[nRF24L01]Packet datas too large. \r\n");
        return RT_ERROR;
    }


   // 如果是发送端（PTX）
    if (nrf24->nrf24_cfg.config.prim_rx == ROLE_PTX && ack_mode == nRF24_SEND_NEED_ACK){
        nRF24L01_Write_Tx_Payload_Ack(nrf24, data, len);
    }
    else if(nrf24->nrf24_cfg.config.prim_rx == ROLE_PTX && ack_mode == nRF24_SEND_NO_ACK){
        nRF24L01_Write_Tx_Payload_NoAck(nrf24, data, len);
    }
    // 如果是接收端（PRX）
    else if(nrf24->nrf24_cfg.config.prim_rx == ROLE_PRX && ack_mode == nRF24_RECE_IN_ACK){
        nRF24L01_Write_Tx_Payload_InAck(nrf24, pipe, data, len);
        rt_sem_release(nrf24_send_sem);
    }

    return RT_EOK;
}



/**
 * @brief  把用户数据写到 TX FIFO（PTX 模式）或 ACK Payload 缓冲区（PRX 模式），并立即触发发送或等待对方读取
 *
 */
void nRF24L01_Set_Role_Mode(nrf24_t nrf24, nrf24_role_et mode)
{
#define ENABLE_ROLE_LOG 0
#if ENABLE_ROLE_LOG
    uint8_t config_reg_data = 0;
    config_reg_data = nRF24L01_Read_Reg_Data(nrf24, NRF24REG_CONFIG);
    LOG_I("LOG:%d. raw config_reg_data = 0x%02x .\r\n",Record.ulog_cnt++, config_reg_data);
#endif

    if(mode == ROLE_PRX){
        nRF24L01_Write_Reg_Bits(nrf24, NRF24REG_CONFIG, NRF24BITMASK_PRIM_RX, 1);
        nrf24->nrf24_cfg.config.prim_rx = ROLE_PRX;
#if ENABLE_ROLE_LOG
        rt_thread_mdelay(5);
        config_reg_data = nRF24L01_Read_Reg_Data(nrf24, NRF24REG_CONFIG);
        LOG_I("LOG:%d. config_reg_data = 0x%02x .\r\n",Record.ulog_cnt++, config_reg_data);
#endif
    }
    else if(mode == ROLE_PTX){
        nRF24L01_Write_Reg_Bits(nrf24, NRF24REG_CONFIG, NRF24BITMASK_PRIM_RX, 0);
        nrf24->nrf24_cfg.config.prim_rx = ROLE_PTX;
#if ENABLE_ROLE_LOG
        rt_thread_mdelay(5);
        config_reg_data = nRF24L01_Read_Reg_Data(nrf24, NRF24REG_CONFIG);
        LOG_I("LOG:%d. config_reg_data = 0x%02x .\r\n",Record.ulog_cnt++, config_reg_data);
#endif
    }
}



/***
 * @brief   nRF24L01的"解锁"指令，专门用于访问扩展功能寄存器（DYNPD 和 FEATURE）
 * @note    使用场景：
 *          1. 第一次写 FEATURE 寄存器
 *          2. 第一次写 DYNPD 寄存器
 *
 *          [注意]
 *          1. 发一次即可，之后随意读写这两个寄存器，无需重复解锁
 *          2. 若上电后从未发ACTIVATE，写 FEATURE/DYNPD 会被静默忽略
 */
void nRF24L01_Ensure_RWW_Features_Activated(nrf24_t nrf24)
{
    if(nrf24->nrf24_flags.activated_features != 1){
        uint8_t tmp[2] = {NRF24CMD_ACTIVATE, 0x73};
        nrf24->nrf24_ops.nrf24_write(&nrf24->port_api, &tmp[0], 2);
        nrf24->nrf24_flags.activated_features = RT_TRUE;
    }
}


/***
 * @brief
 * @note
 */
int nRF24L01_Run(nrf24_t nrf24)
{
    /* ret_flag: 1->发送完成  2->接收完成 */
    rt_uint8_t ret_flag = 0;

    // 1. 如果使用IRQ中断，则获取信号量等待释放
    if(nrf24->nrf24_flags.using_irq == RT_TRUE){
        rt_sem_take(nrf24_irq_sem, RT_WAITING_FOREVER);
    }

    // 2. 读取status状态标志，并清除中断触发标志位
     nrf24->nrf24_flags.status = nRF24L01_Read_Status_Register(nrf24);
     nRF24L01_Clear_Status_Register(nrf24, NRF24BITMASK_RX_DR | NRF24BITMASK_TX_DS);

     // 3. 分析哪条信道接收的数据
     uint8_t pipe = (nrf24->nrf24_flags.status & NRF24BITMASK_RX_P_NO) >> 1;

     // 4. 角色 = 发送端（PTX）
     if(nrf24->nrf24_cfg.config.prim_rx == ROLE_PTX)
     {
         // 4.1 读取status寄存器的 NRF24BITMASK_MAX_RT位，如果为1，说明达到最大重发次数，发送失败
         if(nrf24->nrf24_flags.status & NRF24BITMASK_MAX_RT){
             nRF24L01_Flush_TX_FIFO(nrf24);
             nRF24L01_Clear_Status_Register(nrf24, NRF24BITMASK_MAX_RT);
             if(nrf24->nrf24_cb.nrf24l01_tx_done){
                 nrf24->nrf24_cb.nrf24l01_tx_done(nrf24, NRF24_PIPE_NONE);
             }
             return -1;
         }

         /* 4.2 收到 ACK 带载荷（PTX 也能收） */
         if(nrf24->nrf24_flags.status & NRF24BITMASK_RX_DR){
             uint8_t rec_data[32];
             uint8_t len = nRF24L01_Read_Top_RXFIFO_Width(nrf24);
             nRF24L01_Read_Rx_Payload(nrf24, rec_data, len);
             if(nrf24->nrf24_cb.nrf24l01_rx_ind){
                 nrf24->nrf24_cb.nrf24l01_rx_ind(nrf24, rec_data, len, pipe);
             }
             ret_flag |= 2;
         }


         /* 4.3 发送完成 */
         if(nrf24->nrf24_flags.status & NRF24BITMASK_TX_DS){
             if(nrf24->nrf24_cb.nrf24l01_tx_done){
                 nrf24->nrf24_cb.nrf24l01_tx_done(nrf24, pipe);
             }
             ret_flag |= 1;
         }
     }


     // 5. 角色 = 接收端（PRX）
     if(nrf24->nrf24_cfg.config.prim_rx == ROLE_PRX)
     {
         if(pipe < 5){
             uint8_t data_buf[32];
             uint8_t length = nRF24L01_Read_Top_RXFIFO_Width(nrf24);
             nRF24L01_Read_Rx_Payload(nrf24, data_buf, length);
             if(nrf24->nrf24_cb.nrf24l01_rx_ind){
                 nrf24->nrf24_cb.nrf24l01_rx_ind(nrf24, data_buf, length, pipe);
             }
             ret_flag |= 2;

             if(rt_sem_trytake(nrf24_send_sem) ==  RT_EOK){
                 if(nrf24->nrf24_cb.nrf24l01_tx_done){
                     nrf24->nrf24_cb.nrf24l01_tx_done(nrf24,pipe);
                 }
                 ret_flag |= 1;
             }

         }
     }
     return ret_flag;
}




