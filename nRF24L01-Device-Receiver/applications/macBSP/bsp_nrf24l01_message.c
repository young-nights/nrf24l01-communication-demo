/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-12     18452       the first version
 */
#include "bsp_nrf24l01_message.h"




/*****************************************************************************
* 功能:       CRC校验码计算（采用Crc16Modbus标准多项式）
* 说明:       校验步骤：
            1、 设置 CRC 寄存器， 并给其赋值 FFFF(hex)
            2、 将数据的第一个 8-bit 字符与 16 位 CRC 寄存器的低 8 位进行异或， 并把结果 存入 CRC 寄存器
            3、 CRC 寄存器向右移一位， MSB 补零， 移出并检查 LSB
            4、 如果 LSB 为 0， 重复第三步； 若 LSB 为 1， CRC 寄存器与多项式码相异或
            5、 重复第 3 与第 4 步直到 8 次移位全部完成。 此时一个 8-bit 数据处理完毕
            6、 重复第 2 至第 5 步直到所有数据全部处理完成
            7、 最终 CRC 寄存器的内容即为 CRC 值
            8、 CRC(16 位)多项式为 X16+X15+X2+1， 其对应校验二进制位列为 1 1000 0000 0000 0101
*****************************************************************************/
uint16_t CrcCalc_Crc16Modbus(uint8_t *dat,uint8_t len)
{
    uint16_t    CRC_index = 0xffff;
    uint16_t    buffer;
    volatile    uint8_t i = 0, j = 0;
    for(i = 0; i < len; i++){
        buffer = dat[i];                            // 把数据取出来放在缓冲区
        CRC_index ^= buffer;
        for(j = 0; j < 8; j++){
            if(CRC_index & 0x0001){
                CRC_index >>= 1;
                CRC_index ^= 0xa001;
            }else{
                CRC_index >>= 1;
            }
        }
    }
    return CRC_index;
}





/****
 * @brief 构建nRF24L01的数据包
 * @param cmd_type  --> 帧类型
 *        cmd_status--> 帧状态
 *        *data     --> 数据指针
 *        data_len  --> 数据长度
 *        *out_frame--> 数据包缓冲数组指针
 * @return 总帧长
 */
rt_uint8_t nrf24l01_build_frame(uint8_t cmd_type, uint8_t cmd_status,uint8_t *data, uint8_t data_len,uint8_t *out_frame)
{
    rt_uint8_t  index = 0;
    rt_uint16_t crc = 0;

    out_frame[index++] = 0x55;
    out_frame[index++] = 0xAA;
    out_frame[index++] = 4 + data_len; // 长度 = ID(2) + cmd_type + cmd_status + data
    out_frame[index++] = DEVICE_ID_H;
    out_frame[index++] = DEVICE_ID_L;
    out_frame[index++] = cmd_type;
    out_frame[index++] = cmd_status;

    for(uint8_t i = 0; i < data_len; i++){
        out_frame[index++] = data[i];
    }

    crc = CrcCalc_Crc16Modbus(&out_frame[2], index - 2);
    out_frame[index++] = (crc >> 8) & 0xFF;
    out_frame[index++] = crc & 0xFF;

    return index; // 返回总帧长
}



/**
 * @brief 尝试获取一条指令
 * @param command 指令存放指针
 * @return 获取的指令长度
 * @retval 0 没有获取到指令
 */
static uint8_t  Decode_Step = 0;
static uint8_t  CMD_Length = 0;
static uint8_t  CMD_buffer[30] = {0};
static uint8_t  CMD_DataCnt = 0;
static uint8_t  CRC16_H,CRC16_L = 0;
static uint16_t CRC16_Value = 0;
uint8_t nrf24l01_portocol_get_command(const uint8_t *cmdBuf,const uint16_t cmdLength)
{
    uint8_t i = 0;
    /* 如果未处理的数据长度小于指令长度 则不可能有完整的指令 */
    if(cmdLength < CMD_MINI_LENGTH)
    {
        return CMD_ERROR;
    }

    /* 然后可以进行正常的数据解析流程 */
    /*--------------------------------*/
    /*****************                第一步数据解析               ****************************/
    if(Decode_Step == Decode_Step_0)
    {
        if(*cmdBuf != 0x55)
        {
            /* 然后跳出此次for循环，直接进入下一次循环 */
            return CMD_ERROR;
        }
        else
        {
            /* 正确的话数据解析第一步完成，进行变量赋值 */
            Decode_Step = Decode_Step_1;
        }
    }
    /*****************                第二步数据解析               ****************************/
    if(Decode_Step == Decode_Step_1)
    {
        if(*(cmdBuf + Decode_Step_1) != 0xAA)
        {
            /* 包头的第二个数据不是0xAA -- 给步骤变量赋0值，重新回到0x55判断 */
            Decode_Step = Decode_Step_0;
            /* 然后直接跳出本次循环，重新进入 */
            return CMD_ERROR;
        }
        else
        {
            /* 正确的话数据解析第二步完成，进行变量赋值 */
            Decode_Step = Decode_Step_2;
        }
    }
    /*****************                第三步数据解析               ****************************/
    if(Decode_Step == Decode_Step_2)
    {
        /* 获取指令长度数据（除指令包头的2个字节，长度1字节以及CRC校验的2字节以外的长度） */
        CMD_Length = *(cmdBuf + Decode_Step_2);
        CMD_DataCnt = 0;
        CMD_buffer[CMD_DataCnt] = CMD_Length;
        CMD_DataCnt++;
        Decode_Step = Decode_Step_3;
    }
    /*****************                第四步数据解析               ****************************/
    if(Decode_Step == Decode_Step_3)
    {
        /* 对比设备码ID高位进行判断 */
        if(*(cmdBuf + Decode_Step_3) != DEVICE_ID_H)
        {
            Decode_Step = Decode_Step_0;
            CMD_DataCnt = 0;
            return CMD_ERROR;
        }
        else
        {
            CMD_buffer[CMD_DataCnt] = *(cmdBuf + Decode_Step_3);
            CMD_DataCnt++;
            Decode_Step = Decode_Step_4;
        }
    }
    /*****************                第五步数据解析               ****************************/
    if(Decode_Step == Decode_Step_4)
    {
        /* 对比设备码ID低位进行判断 */
        if(*(cmdBuf + Decode_Step_4) != DEVICE_ID_L)
        {
            Decode_Step = Decode_Step_0;
            CMD_DataCnt = 0;
            return CMD_ERROR;
        }
        else
        {
            CMD_buffer[CMD_DataCnt] = *(cmdBuf + Decode_Step_4);
            CMD_DataCnt++;
            Decode_Step = Decode_Step_5;
        }
    }
    /*****************                第六步数据解析               ****************************/
    if(Decode_Step == Decode_Step_5)
    {
        /* 接收数据 */
        for(i = 0; CMD_DataCnt < (CMD_Length + 1); CMD_DataCnt++,i++)
        {
            CMD_buffer[CMD_DataCnt] = *(cmdBuf + Decode_Step_5 + i);
        }

        CRC16_H = *(cmdBuf + Decode_Step_5 + i);
        Decode_Step = Decode_Step_6;
        i++;
    }
    /*****************                第七步数据解析               ****************************/
    if(Decode_Step == Decode_Step_6)
    {
        CRC16_L = *(cmdBuf + Decode_Step_5 + i);
        Decode_Step = Decode_Step_0;
        CRC16_Value = CrcCalc_Crc16Modbus(CMD_buffer, CMD_Length + 1);
        if(((CRC16_H << 8) | CRC16_L) == CRC16_Value)
        {
//            nrf24l01_protocol_operation(dev,CMD_buffer);
            return CMD_TRUE;
        }
    }
}




/**
 * @brief   解析数据域指令，执行响应的函数
 * @param   CmdBuf  数据域存放的指针
 * @retval  void
 */
void nrf24l01_protocol_operation(uint8_t* CmdBuf)
{
    /*以 06 00 61 31 02 01 01 数据域指令为例*/
    /*长度 + 设备ID_H + 设备ID_L + 指令类型 + 指令状态 + 实际指令宏 + 指令数据 */
    switch(*(CmdBuf + 3))
    {

        /*********************************************************************************/
        /*    控制指令码 0x31  */
        /*********************************************************************************/
        case FRAME_TYPE_ACT:
        {
            switch(*(CmdBuf + 5))
            {
                //----------------------------------------------------------------------------------------------------

                //----------------------------------------------------------------------------------------------------

                default:    break;
            }
        }break;


        default:    break;
    }
}







/**
 * @brief   nRF24L01向指定管道发送指令
 * @param   order   指令码
 * @retval  None
 */
extern rt_uint8_t largeid_buf[8];
extern rt_uint8_t smallid_buf[8];
void nrf24l01_order_to_pipe(uint8_t order, uint8_t pipe_num)
{
    uint8_t emptyBuf[20] = {0};
    uint8_t frame_package[30] = { 0 };
    uint8_t package_len = 0;
    switch(order)
    {
        // 0x31指令集-----------------------------------------------------------------------------------------------------------------

        case Order_nRF24L01_Connect_Control_Panel:
        {
            rt_memset(emptyBuf, 0, sizeof(emptyBuf));
            emptyBuf[0] = FRAME_NRF24_CONNECT_CTRL_PANEL_CMD;
            package_len = nrf24l01_build_frame(FRAME_TYPE_ACT,FRAME_STATE_ASK,emptyBuf,1,frame_package);
            nRF24L01_Send_Packet(_nrf24, frame_package, package_len, 0, nRF24_SEND_NEED_ACK);
            _nrf24->nrf24_ops.nrf24_set_ce();
            rt_thread_mdelay(1);

        }break;


        default: break;
    }
}


