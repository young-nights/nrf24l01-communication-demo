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

}rt_align(1);








