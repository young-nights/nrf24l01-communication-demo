/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-03     RT-Thread    first version
 */

#include <rtthread.h>
#include "bsp_nrf24l01_spi.h"
#include <rtdbg.h>
#include "bsp_sys.h"
#include "bsp_nrf24l01_driver.h"



/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  /*1. 获取中断引脚编号 */
  /* 创建nRF24L01全局结构体接口 */
  nrf24_t nrf24 = malloc(sizeof(nrf24_t));
  if (nrf24 == NULL) {
      LOG_E("nrf24 malloc error. \r\n");
  }
  else{
      LOG_I("nrf24 malloc successful. \r\n");
  }
  nrf24->port_api.nRF24L01_IRQ_Pin_Num = GET_PIN(C, 5);
  /*2. 初始化SPI */
  nRF24L01_SPI_Init(&nrf24->port_api);
//  /*3. 配置nRF24L01的参数*/
//  nRF24L01_Param_Config(&nrf24->nrf24_cfg);
//  /*4. 配置启用中断引脚和中断回调函数 */
//  nRF24L01_IQR_GPIO_Config(&nrf24->port_api);
//  /*5. 通过回环通信，检测SPI硬件链路是否有误 */
//  if (nRF24L01_Check_SPI_Community(nrf24) != RT_EOK){
//      LOG_E("nRF24L01 check spi hardware false.\r\n");
//  }
//  else{
//      LOG_I("nRF24L01 check spi hardware successful.\r\n");
//  }
//  /*6. 先进入掉电模式 */
//  nRF24L01_Enter_Power_Down_Mode(nrf24);
//  /*7. 更新寄存器参数 */
//  if (nRF24L01_Update_Parameter(nrf24) != RT_EOK){
//      LOG_E("nRF24L01 update_onchip_config false.\r\n");
//  }
//  else{
//      LOG_E("nRF24L01 update_onchip_config successful.\r\n");
//  }
//  /*8. 检测寄存器参数 */
//  if (nRF24L01_Read_Onchip_Parameter(nrf24) != RT_EOK){
//      LOG_E("nRF24L01 check parameter false.\r\n");
//  }
//  else{
//      LOG_I("nRF24L01 check parameter successful.\r\n");
//  }
//  /*9. 清空"发送/接收"队列 */
//  nRF24L01_Flush_RX_FIFO(nrf24);
//  nRF24L01_Flush_TX_FIFO(nrf24);
//  /*10. 清除中断标志*/
//  nRF24L01_Clear_IRQ_Flags(nrf24);
//  /*11. 清除重发计数和丢包计数*/
//  nRF24L01_Clear_Observe_TX(nrf24);
//  /*12. 配置完成，进入上电模式 */
//  nRF24L01_Enter_Power_Up_Mode(nrf24);
//
//  nrf24->nrf24_ops.nrf24_set_ce();
//  LOG_I("Successfully initialized");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
      rt_thread_mdelay(500);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
