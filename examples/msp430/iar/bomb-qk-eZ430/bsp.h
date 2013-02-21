/*****************************************************************************
* Product: Time Bomb example
* Last Updated for Version: 4.0.00
* Date of the Last Update:  Apr 05, 2008
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2008 Quantum Leaps, LLC. All rights reserved.
*
* This software may be distributed and modified under the terms of the GNU
* General Public License version 2 (GPL) as published by the Free Software
* Foundation and appearing in the file GPL.TXT included in the packaging of
* this file. Please note that GPL Section 2[b] requires that all works based
* on this software must also be made publicly available under the terms of
* the GPL ("Copyleft").
*
* Alternatively, this software may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GPL and are specifically designed for licensees interested in
* retaining the proprietary status of their code.
*
* Contact information:
* Quantum Leaps Web site:  http://www.quantum-leaps.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#ifndef bsp_h
#define bsp_h

#include <msp430x20x3.h>                             /* MSP430 variant used */

/*--------------------------------------------------------------------------*/
#define BSP_MCK              16000000
#define BSP_SMCLK            1100000
#define BSP_TICKS_PER_SEC    20

void BSP_init(void);

#define LED_on()     (P1OUT |= (uint8_t)1)
#define LED_off()    (P1OUT &= (uint8_t)~1)
#define LED_toggle() (P1OUT ^= (uint8_t)1)

#endif                                                             /* bsp_h */