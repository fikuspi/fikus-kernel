/*******************************************************************************
 * Agere Systems Inc.
 * Wireless device driver for Fikus (wlags49).
 *
 * Copyright (c) 1998-2003 Agere Systems Inc.
 * All rights reserved.
 *   http://www.agere.com
 *
 * Initially developed by TriplePoint, Inc.
 *   http://www.triplepoint.com
 *
 *------------------------------------------------------------------------------
 *
 *   Header describing information required for the driver to support PCI.
 *
 *------------------------------------------------------------------------------
 *
 * SOFTWARE LICENSE
 *
 * This software is provided subject to the following terms and conditions,
 * which you should read carefully before using the software.  Using this
 * software indicates your acceptance of these terms and conditions.  If you do
 * not agree with these terms and conditions, do not use the software.
 *
 * Copyright © 2003 Agere Systems Inc.
 * All rights reserved.
 *
 * Redistribution and use in source or binary forms, with or without
 * modifications, are permitted provided that the following conditions are met:
 *
 * . Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following Disclaimer as comments in the code as
 *    well as in the documentation and/or other materials provided with the
 *    distribution.
 *
 * . Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following Disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * . Neither the name of Agere Systems Inc. nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Disclaimer
 *
 * THIS SOFTWARE IS PROVIDED AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, INFRINGEMENT AND THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  ANY
 * USE, MODIFICATION OR DISTRIBUTION OF THIS SOFTWARE IS SOLELY AT THE USERS OWN
 * RISK. IN NO EVENT SHALL AGERE SYSTEMS INC. OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, INCLUDING, BUT NOT LIMITED TO, CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 ******************************************************************************/

#ifndef __WL_PCI_H__
#define __WL_PCI_H__




/*******************************************************************************
 *  constant definitions
 ******************************************************************************/
#define PCI_VENDOR_IDWL_LKM     0x11C1  /* Lucent Microelectronics */
#define PCI_DEVICE_ID_WL_LKM_0  0xAB30  /* Mini PCI */
#define PCI_DEVICE_ID_WL_LKM_1  0xAB34  /* Mini PCI */
#define PCI_DEVICE_ID_WL_LKM_2  0xAB11  /* WARP CardBus */




/*******************************************************************************
 *  function prototypes
 ******************************************************************************/
int wl_adapter_init_module( void );

void wl_adapter_cleanup_module( void );

int wl_adapter_insert( struct net_device *dev );

int wl_adapter_open( struct net_device *dev );

int wl_adapter_close( struct net_device *dev );

int wl_adapter_is_open( struct net_device *dev );


#ifdef ENABLE_DMA

void wl_pci_dma_hcf_supply( struct wl_private *lp );

void wl_pci_dma_hcf_reclaim( struct wl_private *lp );

DESC_STRCT * wl_pci_dma_get_tx_packet( struct wl_private *lp );

void wl_pci_dma_put_tx_packet( struct wl_private *lp, DESC_STRCT *desc );

void wl_pci_dma_hcf_reclaim_tx( struct wl_private *lp );

#endif  // ENABLE_DMA


#endif  // __WL_PCI_H__
