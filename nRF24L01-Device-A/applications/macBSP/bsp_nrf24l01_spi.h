/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-29     Administrator       the first version
 */
#ifndef APPLICATIONS_MACBSP_INC_BSP_LORA_SPI_H_
#define APPLICATIONS_MACBSP_INC_BSP_LORA_SPI_H_
#include "bsp_sys.h"

#define USE_CUSTOMER_NRF24L01 1
#if USE_CUSTOMER_NRF24L01


/* 片选引脚 -- CS */
#define     nRF24_CS_PORT     LORA_CE_GPIO_Port
#define     nRF24_CS_PIN      LORA_CE_Pin

#define     nRF24_CS_SET(bit) if(bit) \
                              HAL_GPIO_WritePin ( nRF24_CS_PORT, nRF24_CS_PIN , GPIO_PIN_SET )\
                              else \
                              HAL_GPIO_WritePin ( nRF24_CS_PORT, nRF24_CS_PIN , GPIO_PIN_RESET )


/* SPI引脚 -- NSS */
#define     nRF24_NSS_PORT     LORA_CSN_GPIO_Port
#define     nRF24_NSS_PIN      LORA_CSN_Pin


#define     nRF24_NSS_SET(bit) if(bit) \
                               HAL_GPIO_WritePin ( nRF24_NSS_PORT, nRF24_NSS_PIN , GPIO_PIN_SET )\
                               else \
                               HAL_GPIO_WritePin ( nRF24_NSS_PORT, nRF24_NSS_PIN , GPIO_PIN_RESET )



typedef struct nrf24_port *nrf24_port_t;

struct nrf24_port
{
    const struct nrf24_func_opts *nrf24_ops;

    int nrf24_ce_pin;
    int nrf24_irq_pin;
    void(*nrf24_irq_cb)(struct nrf24_port *halport);
    struct rt_spi_device *nrf24_spi_dev;
};



struct nrf24_func_opts{
    int (* nrf24_send_then_recv)(struct nrf24_port *halport, const uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t rlen);
    int (* nrf24_send_then_send)(struct nrf24_port *halport, const uint8_t *tbuf_1, uint8_t tlen_1, uint8_t *tbuf_2, uint8_t tlen_2);
    int (* nrf24_write)(struct nrf24_port *halport, const uint8_t *buf, uint8_t len);
    int (* nrf24_set_ce)(struct nrf24_port *halport);
    int (* nrf24_reset_ce)(struct nrf24_port *halport);
    int (* nrf24_read_irq_pin)(struct nrf24_port *halport);
};



//---------spi函数声明-------------------



#endif

#endif /* APPLICATIONS_MACBSP_INC_BSP_LORA_SPI_H_ */
