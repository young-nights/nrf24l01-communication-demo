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

/* 失能引脚 */
#define     nRF24_PIN_NONE    -1

/* 片选引脚 -- CS */
#define     nRF24_CS_PORT     nRF24L01_CE_GPIO_Port
#define     nRF24_CS_PIN      nRF24L01_CE_Pin

#define     nRF24_CS_SET(bit) if(bit) \
                              HAL_GPIO_WritePin ( nRF24_CS_PORT, nRF24_CS_PIN , GPIO_PIN_SET )\
                              else \
                              HAL_GPIO_WritePin ( nRF24_CS_PORT, nRF24_CS_PIN , GPIO_PIN_RESET )


/* SPI引脚 -- NSS */
#define     nRF24_NSS_PORT     nRF24L01_CSN_GPIO_Port
#define     nRF24_NSS_PIN      nRF24L01_CSN_Pin


#define     nRF24_NSS_SET(bit) if(bit) \
                               HAL_GPIO_WritePin ( nRF24_NSS_PORT, nRF24_NSS_PIN , GPIO_PIN_SET )\
                               else \
                               HAL_GPIO_WritePin ( nRF24_NSS_PORT, nRF24_NSS_PIN , GPIO_PIN_RESET )




#define SUB_HALPORT_WIRTE(_nrf24, buf, len)                             _nrf24->halport.nrf24_ops->nrf24_write(&_nrf24->halport, buf, len)
#define SUB_HALPORT_SEND_THEN_RECV(_nrf24, tbuf, tlen, rbuf, rlen)      _nrf24->halport.nrf24_ops->nrf24_send_then_recv(&_nrf24->halport, tbuf, tlen, rbuf, rlen)
#define SUB_HALPORT_SEND_THEN_SEND(_nrf24, buf1, len1, buf2, len2)      _nrf24->halport.nrf24_ops->nrf24_send_then_send(&_nrf24->halport, buf1, len1, buf2, len2)
#define SUB_HALPORT_RESET_CE(_nrf24)                                    _nrf24->halport.nrf24_ops->nrf24_reset_ce(&_nrf24->halport)
#define SUB_HALPORT_SET_CE(_nrf24)                                      _nrf24->halport.nrf24_ops->nrf24_set_ce(&_nrf24->halport)

#define NRF24_HALPORT_WRITE(buf, len)                                   SUB_HALPORT_WIRTE(nrf24, buf, len)
#define NRF24_HALPORT_SEND_THEN_RECV(tbuf, tlen, rbuf, rlen)            SUB_HALPORT_SEND_THEN_RECV(nrf24, tbuf, tlen, rbuf, rlen)
#define NRF24_HALPORT_SEND_THEN_SEND(buf1, len1, buf2, len2)            SUB_HALPORT_SEND_THEN_SEND(nrf24, buf1, len1, buf2, len2)
#define NRF24_HALPORT_RESET_CE()                                        SUB_HALPORT_RESET_CE(nrf24)
#define NRF24_HALPORT_SET_CE()                                          SUB_HALPORT_SET_CE(nrf24)



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
    int (* nrf24_send_then_send)(struct nrf24_port *halport, const uint8_t *tbuf_1, uint8_t tlen_1, const uint8_t *tbuf_2, uint8_t tlen_2);
    int (* nrf24_write)(struct nrf24_port *halport, const uint8_t *buf, uint8_t len);
    int (* nrf24_set_ce)(struct nrf24_port *halport);
    int (* nrf24_reset_ce)(struct nrf24_port *halport);
    int (* nrf24_read_irq_pin)(struct nrf24_port *halport);
};




//---------spi函数声明-------------------

int nrf24_port_init(nrf24_port_t halport, char *spi_dev_name, int ce_pin, int irq_pin, void(*irq_callback)(nrf24_port_t halport));

#endif

#endif /* APPLICATIONS_MACBSP_INC_BSP_LORA_SPI_H_ */
