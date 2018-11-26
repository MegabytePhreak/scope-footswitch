/*
 * This file is part of the scope-footswitch project.
 *
 * Copyright (C) 2018 Paul Roukema <paul@paulroukema.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#ifndef BOARD_H
#define BOARD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * Setup for STMicroelectronics STM32 Nucleo64-F401RE board.
 */

/*
 * Board identifier.
 */
#define BOARD_SCOPE_FOOTSWITCH_V1
#define BOARD_NAME                  "scope-footswitch_v1"

/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0U
#endif

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                8000000U
#endif

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD                   300U

/*
 * MCU type as defined in the ST header.
 */
#define STM32F401xE

/*
 * IO pins assignments.
 */
#define GPIOA_LED_DATA_LS           1U
#define GPIOA_UART_TX               2U
#define GPIOA_UART_RX               3U
#define GPIOA_MODE_LED_R            6U
#define GPIOA_MODE_LED_G            7U
#define GPIOA_HOST_VBUS_EN          9U
#define GPIOA_STM32_DM              11U
#define GPIOA_STM32_DP              12U
#define GPIOA_SWDIO                 13U
#define GPIOA_SWCLK                 14U

#define GPIOB_FOOTSW_BTN1           0U
#define GPIOB_FOOTSW_BTN2           1U
#define GPIOB_BOOT1                 2U
#define GPIOB_EN_DEVICE             5U
#define GPIOB_I2C_SCL               6U
#define GPIOB_I2C_SDA               9U
#define GPIOB_HOST_FAULT_N          15U

#define GPIOC_BTN                   13U
#define GPIOC_MODE                  14U

#define GPIOH_OSC_IN                0U
#define GPIOH_OSC_OUT               1U

/*
 * IO lines assignments.
 */
#define LINE_LED_DATA_LS            PAL_LINE(GPIOA, GPIOA_LED_DATA_LS)
#define LINE_UART_TX                PAL_LINE(GPIOA, GPIOA_UART_TX)
#define LINE_UART_RX                PAL_LINE(GPIOA, GPIOA_UART_RX)
#define LINE_MODE_LED_R             PAL_LINE(GPIOA, GPIOA_MODE_LED_R)
#define LINE_MODE_LED_G             PAL_LINE(GPIOA, GPIOA_MODE_LED_G)
#define LINE_HOST_VBUS_EN           PAL_LINE(GPIOA, GPIOA_HOST_VBUS_EN)
#define LINE_STM32_DM               PAL_LINE(GPIOA, GPIOA_STM32_DM)
#define LINE_STM32_DP               PAL_LINE(GPIOA, GPIOA_STM32_DP)
#define LINE_SWDIO                  PAL_LINE(GPIOA, GPIOA_SWDIO)
#define LINE_SWCLK                  PAL_LINE(GPIOA, GPIOA_SWCLK)

#define LINE_FOOTSW_BTN1            PAL_LINE(GPIOB, GPIOB_FOOTSW_BTN1)
#define LINE_FOOTSW_BTN2            PAL_LINE(GPIOB, GPIOB_FOOTSW_BTN2)
#define LINE_BOOT1                  PAL_LINE(GPIOB, GPIOB_BOOT1)
#define LINE_EN_DEVICE              PAL_LINE(GPIOB, GPIOB_EN_DEVICE)
#define LINE_I2C_SCL                PAL_LINE(GPIOB, GPIOB_I2C_SCL)
#define LINE_I2C_SDA                PAL_LINE(GPIOB, GPIOB_I2C_SDA)
#define LINE_HOST_FAULT_N           PAL_LINE(GPIOB, GPIOB_HOST_FAULT_N)

#define LINE_BTN                    PAL_LINE(GPIOC, GPIOC_BTN)
#define LINE_MODE                   PAL_LINE(GPIOC, GPIOC_MODE)

#define LINE_OSC_IN                 PAL_LINE(GPIOH, GPIOH_OSC_IN)
#define LINE_OSC_OUT                PAL_LINE(GPIOH, GPIOH_OSC_OUT)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2U))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2U))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2U))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2U))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_VERYLOW(n)       (0U << ((n) * 2U))
#define PIN_OSPEED_LOW(n)           (1U << ((n) * 2U))
#define PIN_OSPEED_MEDIUM(n)        (2U << ((n) * 2U))
#define PIN_OSPEED_HIGH(n)          (3U << ((n) * 2U))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2U))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2U))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2U))
#define PIN_AFIO_AF(n, v)           ((v) << (((n) % 8U) * 4U))

/*
 * GPIOA setup:
 *
 * PA0  - UNUSED                    (input pullup).
 * PA1  - GPIOA_LED_DATA_LS         (alternate 2 - TIM5_CH2).
 * PA2  - UART_TX                   (alternate 7 - USART2_TX).
 * PA3  - UART_RX                   (alternate 7 - USART2_RX).
 * PA4  - UNUSED                    (input pullup).
 * PA5  - UNUSED                    (input pullup).
 * PA6  - MODE_LED_R                (alternate 3 - TIM3_CH1).
 * PA7  - MODE_LED_G                (alternate 3 - TIM3_CH2).
 * PA8  - UNUSED                    (input pullup).
 * PA9  - HOST_VBUS_EN              (output pushpull).
 * PA10 - UNUSED                    (input pullup).
 * PA11 - STM32_DM                  (alternate 10 - OTG_FS_DM).
 * PA12 - STM32_DP                  (alternate 10 - OTG_FS_DP).
 * PA13 - SWDIO                     (alternate 0 - SWDIO).
 * PA14 - SWCLK                     (alternate 0 - SWCLK).
 * PA15 - UNUSED                    (input pullup).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_INPUT(0U) |                   \
                                     PIN_MODE_ALTERNATE(GPIOA_LED_DATA_LS) |\
                                     PIN_MODE_ALTERNATE(GPIOA_UART_TX) |    \
                                     PIN_MODE_ALTERNATE(GPIOA_UART_RX) |    \
                                     PIN_MODE_INPUT(4U) |                   \
                                     PIN_MODE_INPUT(5U) |                   \
                                     PIN_MODE_ALTERNATE(GPIOA_MODE_LED_R) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_MODE_LED_G) |     \
                                     PIN_MODE_INPUT(8U) |                   \
                                     PIN_MODE_OUTPUT(GPIOA_HOST_VBUS_EN) |   \
                                     PIN_MODE_INPUT(10U) |                  \
                                     PIN_MODE_ALTERNATE(GPIOA_STM32_DM) |   \
                                     PIN_MODE_ALTERNATE(GPIOA_STM32_DP) |   \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_INPUT(15U))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(0U) |                   \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LED_DATA_LS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_UART_TX) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOA_UART_RX) |        \
                                     PIN_OTYPE_PUSHPULL(4U) |                   \
                                     PIN_OTYPE_PUSHPULL(5U) |                   \
                                     PIN_OTYPE_PUSHPULL(GPIOA_MODE_LED_R) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_MODE_LED_G) |     \
                                     PIN_OTYPE_PUSHPULL(8U) |                   \
                                     PIN_OTYPE_PUSHPULL(GPIOA_HOST_VBUS_EN) |   \
                                     PIN_OTYPE_PUSHPULL(10U) |                  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_STM32_DM) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_STM32_DP) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO) |          \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |          \
                                     PIN_OTYPE_PUSHPULL(15U))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_HIGH(0U) |                   \
                                     PIN_OSPEED_HIGH(GPIOA_LED_DATA_LS) |    \
                                     PIN_OSPEED_HIGH(GPIOA_UART_TX) |        \
                                     PIN_OSPEED_HIGH(GPIOA_UART_RX) |        \
                                     PIN_OSPEED_HIGH(4U) |                   \
                                     PIN_OSPEED_HIGH(5U) |                   \
                                     PIN_OSPEED_HIGH(GPIOA_MODE_LED_R) |     \
                                     PIN_OSPEED_HIGH(GPIOA_MODE_LED_G) |     \
                                     PIN_OSPEED_HIGH(8U) |                   \
                                     PIN_OSPEED_HIGH(GPIOA_HOST_VBUS_EN) |   \
                                     PIN_OSPEED_HIGH(10U) |                  \
                                     PIN_OSPEED_HIGH(GPIOA_STM32_DM) |       \
                                     PIN_OSPEED_HIGH(GPIOA_STM32_DP) |       \
                                     PIN_OSPEED_HIGH(GPIOA_SWDIO) |          \
                                     PIN_OSPEED_HIGH(GPIOA_SWCLK) |          \
                                     PIN_OSPEED_HIGH(15U))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_PULLUP(0U) |                     \
                                     PIN_PUPDR_PULLUP(GPIOA_LED_DATA_LS) |      \
                                     PIN_PUPDR_FLOATING(GPIOA_UART_TX) |        \
                                     PIN_PUPDR_FLOATING(GPIOA_UART_RX) |        \
                                     PIN_PUPDR_PULLUP(4U) |                     \
                                     PIN_PUPDR_PULLUP(5U) |                     \
                                     PIN_PUPDR_PULLDOWN(GPIOA_MODE_LED_R) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOA_MODE_LED_G) |     \
                                     PIN_PUPDR_PULLUP(8U) |                     \
                                     PIN_PUPDR_PULLDOWN(GPIOA_HOST_VBUS_EN) |   \
                                     PIN_PUPDR_PULLUP(10U) |                    \
                                     PIN_PUPDR_FLOATING(GPIOA_STM32_DM) |       \
                                     PIN_PUPDR_FLOATING(GPIOA_STM32_DP) |       \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDIO) |            \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK) |          \
                                     PIN_PUPDR_PULLUP(15U))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(0U) |                   \
                                     PIN_ODR_HIGH(GPIOA_LED_DATA_LS) |    \
                                     PIN_ODR_HIGH(GPIOA_UART_TX) |        \
                                     PIN_ODR_HIGH(GPIOA_UART_RX) |        \
                                     PIN_ODR_HIGH(4U) |                   \
                                     PIN_ODR_HIGH(5U) |                   \
                                     PIN_ODR_HIGH(GPIOA_MODE_LED_R) |     \
                                     PIN_ODR_HIGH(GPIOA_MODE_LED_G) |     \
                                     PIN_ODR_HIGH(8U) |                   \
                                     PIN_ODR_HIGH(GPIOA_HOST_VBUS_EN) |   \
                                     PIN_ODR_HIGH(10U) |                  \
                                     PIN_ODR_HIGH(GPIOA_STM32_DM) |       \
                                     PIN_ODR_HIGH(GPIOA_STM32_DP) |       \
                                     PIN_ODR_HIGH(GPIOA_SWDIO) |          \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |          \
                                     PIN_ODR_HIGH(15U))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_LED_DATA_LS, 2U) | \
                                     PIN_AFIO_AF(GPIOA_UART_TX, 7U) |     \
                                     PIN_AFIO_AF(GPIOA_UART_RX, 7U) |     \
                                     PIN_AFIO_AF(GPIOA_MODE_LED_R, 2U) |  \
                                     PIN_AFIO_AF(GPIOA_MODE_LED_G, 2U))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_STM32_DM, 10U) |   \
                                     PIN_AFIO_AF(GPIOA_STM32_DP, 10U) |   \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0U) |       \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0U))

/*
 * GPIOB setup:
 *
 * PB0  - FOOTSW_BTN1               (input pullup).
 * PB1  - FOOTSW_BTN2               (input pullup).
 * PB2  - BOOT1                     (input pullup).
 * PB3  - UNUSED                    (input pullup).
 * PB4  - UNUSED                    (input pullup).
 * PB5  - EN_DEVICE                 (input pullup).
 * PB6  - I2C_SCL                   (alternate 4 - I2C1_SCL).
 * PB7  - UNUSED                    (input pullup).
 * PB8  - UNUSED                    (input pullup).
 * PB9  - I2C_SDA                   (alternate 4 - I2C1_SDA).
 * PB10 - UNUSED                    (input pullup).
 * PB11 - UNUSED                    (input pullup).
 * PB12 - UNUSED                    (input pullup).
 * PB13 - UNUSED                    (input pullup).
 * PB14 - UNUSED                    (input pullup).
 * PB15 - HOST_FAULT_N              (input pullup).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_INPUT(GPIOB_FOOTSW_BTN1) |    \
                                     PIN_MODE_INPUT(GPIOB_FOOTSW_BTN2) |    \
                                     PIN_MODE_INPUT(GPIOB_BOOT1) |          \
                                     PIN_MODE_INPUT(3U) |                   \
                                     PIN_MODE_INPUT(4U) |                   \
                                     PIN_MODE_OUTPUT(GPIOB_EN_DEVICE) |      \
                                     PIN_MODE_INPUT(GPIOB_I2C_SCL) |        \
                                     PIN_MODE_INPUT(7U) |                   \
                                     PIN_MODE_INPUT(8U) |                   \
                                     PIN_MODE_INPUT(GPIOB_I2C_SDA) |        \
                                     PIN_MODE_INPUT(10U) |                  \
                                     PIN_MODE_INPUT(11U) |                  \
                                     PIN_MODE_INPUT(12U) |                  \
                                     PIN_MODE_INPUT(13U) |                  \
                                     PIN_MODE_INPUT(14U) |                  \
                                     PIN_MODE_INPUT(GPIOB_HOST_FAULT_N))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_FOOTSW_BTN1) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_FOOTSW_BTN2) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_BOOT1) |          \
                                     PIN_OTYPE_PUSHPULL(3U) |                   \
                                     PIN_OTYPE_PUSHPULL(4U) |                   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_EN_DEVICE) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_I2C_SCL) |        \
                                     PIN_OTYPE_PUSHPULL(7U) |                   \
                                     PIN_OTYPE_PUSHPULL(8U) |                   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_I2C_SDA) |        \
                                     PIN_OTYPE_PUSHPULL(10U) |                  \
                                     PIN_OTYPE_PUSHPULL(11U) |                  \
                                     PIN_OTYPE_PUSHPULL(12U) |                  \
                                     PIN_OTYPE_PUSHPULL(13U) |                  \
                                     PIN_OTYPE_PUSHPULL(14U) |                  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_HOST_FAULT_N))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_HIGH(GPIOB_FOOTSW_BTN1) |    \
                                     PIN_OSPEED_HIGH(GPIOB_FOOTSW_BTN2) |    \
                                     PIN_OSPEED_HIGH(GPIOB_BOOT1) |          \
                                     PIN_OSPEED_HIGH(3U) |                   \
                                     PIN_OSPEED_HIGH(4U) |                   \
                                     PIN_OSPEED_HIGH(GPIOB_EN_DEVICE) |      \
                                     PIN_OSPEED_HIGH(GPIOB_I2C_SCL) |        \
                                     PIN_OSPEED_HIGH(7U) |                   \
                                     PIN_OSPEED_HIGH(8U) |                   \
                                     PIN_OSPEED_HIGH(GPIOB_I2C_SDA) |        \
                                     PIN_OSPEED_HIGH(10U) |                  \
                                     PIN_OSPEED_HIGH(11U) |                  \
                                     PIN_OSPEED_HIGH(12U) |                  \
                                     PIN_OSPEED_HIGH(13U) |                  \
                                     PIN_OSPEED_HIGH(14U) |                  \
                                     PIN_OSPEED_HIGH(GPIOB_HOST_FAULT_N))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_PULLUP(GPIOB_FOOTSW_BTN1) |    \
                                     PIN_PUPDR_PULLUP(GPIOB_FOOTSW_BTN2) |    \
                                     PIN_PUPDR_PULLUP(GPIOB_BOOT1) |          \
                                     PIN_PUPDR_PULLUP(3U) |                   \
                                     PIN_PUPDR_PULLUP(4U) |                   \
                                     PIN_PUPDR_FLOATING(GPIOB_EN_DEVICE) |      \
                                     PIN_PUPDR_PULLUP(GPIOB_I2C_SCL) |        \
                                     PIN_PUPDR_PULLUP(7U) |                   \
                                     PIN_PUPDR_PULLUP(8U) |                   \
                                     PIN_PUPDR_PULLUP(GPIOB_I2C_SDA) |        \
                                     PIN_PUPDR_PULLUP(10U) |                  \
                                     PIN_PUPDR_PULLUP(11U) |                  \
                                     PIN_PUPDR_PULLUP(12U) |                  \
                                     PIN_PUPDR_PULLUP(13U) |                  \
                                     PIN_PUPDR_PULLUP(14U) |                  \
                                     PIN_PUPDR_PULLUP(GPIOB_HOST_FAULT_N))
#define VAL_GPIOB_ODR               (PIN_ODR_HIGH(GPIOB_FOOTSW_BTN1) |    \
                                     PIN_ODR_HIGH(GPIOB_FOOTSW_BTN2) |    \
                                     PIN_ODR_HIGH(GPIOB_BOOT1) |          \
                                     PIN_ODR_HIGH(3U) |                   \
                                     PIN_ODR_HIGH(4U) |                   \
                                     PIN_ODR_LOW(GPIOB_EN_DEVICE) |      \
                                     PIN_ODR_HIGH(GPIOB_I2C_SCL) |        \
                                     PIN_ODR_HIGH(7U) |                   \
                                     PIN_ODR_HIGH(8U) |                   \
                                     PIN_ODR_HIGH(GPIOB_I2C_SDA) |        \
                                     PIN_ODR_HIGH(10U) |                  \
                                     PIN_ODR_HIGH(11U) |                  \
                                     PIN_ODR_HIGH(12U) |                  \
                                     PIN_ODR_HIGH(13U) |                  \
                                     PIN_ODR_HIGH(14U) |                  \
                                     PIN_ODR_HIGH(GPIOB_HOST_FAULT_N))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_I2C_SCL, 4U))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_I2C_SDA, 4U))

/*
 * GPIOC setup:
 *
 * PC0  - NC                        (input pullup).
 * PC1  - NC                        (input pullup).
 * PC2  - NC                        (input pullup).
 * PC3  - NC                        (input pullup).
 * PC4  - NC                        (input pullup).
 * PC5  - NC                        (input pullup).
 * PC6  - NC                        (input pullup).
 * PC7  - NC                        (input pullup).
 * PC8  - NC                        (input pullup).
 * PC9  - NC                        (input pullup).
 * PC10 - NC                        (input pullup).
 * PC11 - NC                        (input pullup).
 * PC12 - NC                        (input pullup).
 * PC13 - BTN                       (input pullup).
 * PC14 - MODE                      (input pullup).
 * PC15 - UNUSED                    (input pullup).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_INPUT(0U) |           \
                                     PIN_MODE_INPUT(1U) |           \
                                     PIN_MODE_INPUT(2U) |           \
                                     PIN_MODE_INPUT(3U) |           \
                                     PIN_MODE_INPUT(4U) |           \
                                     PIN_MODE_INPUT(5U) |           \
                                     PIN_MODE_INPUT(6U) |           \
                                     PIN_MODE_INPUT(7U) |           \
                                     PIN_MODE_INPUT(8U) |           \
                                     PIN_MODE_INPUT(9U) |           \
                                     PIN_MODE_INPUT(10U) |          \
                                     PIN_MODE_INPUT(11U) |          \
                                     PIN_MODE_INPUT(12U) |          \
                                     PIN_MODE_INPUT(GPIOC_BTN) |    \
                                     PIN_MODE_INPUT(GPIOC_MODE) |   \
                                     PIN_MODE_INPUT(15U))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(0U) |           \
                                     PIN_OTYPE_PUSHPULL(1U) |           \
                                     PIN_OTYPE_PUSHPULL(2U) |           \
                                     PIN_OTYPE_PUSHPULL(3U) |           \
                                     PIN_OTYPE_PUSHPULL(4U) |           \
                                     PIN_OTYPE_PUSHPULL(5U) |           \
                                     PIN_OTYPE_PUSHPULL(6U) |           \
                                     PIN_OTYPE_PUSHPULL(7U) |           \
                                     PIN_OTYPE_PUSHPULL(8U) |           \
                                     PIN_OTYPE_PUSHPULL(9U) |           \
                                     PIN_OTYPE_PUSHPULL(10U) |          \
                                     PIN_OTYPE_PUSHPULL(11U) |          \
                                     PIN_OTYPE_PUSHPULL(12U) |          \
                                     PIN_OTYPE_PUSHPULL(GPIOC_BTN) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOC_MODE) |   \
                                     PIN_OTYPE_PUSHPULL(15U))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_HIGH(0U) |           \
                                     PIN_OSPEED_HIGH(1U) |           \
                                     PIN_OSPEED_HIGH(2U) |           \
                                     PIN_OSPEED_HIGH(3U) |           \
                                     PIN_OSPEED_HIGH(4U) |           \
                                     PIN_OSPEED_HIGH(5U) |           \
                                     PIN_OSPEED_HIGH(6U) |           \
                                     PIN_OSPEED_HIGH(7U) |           \
                                     PIN_OSPEED_HIGH(8U) |           \
                                     PIN_OSPEED_HIGH(9U) |           \
                                     PIN_OSPEED_HIGH(10U) |          \
                                     PIN_OSPEED_HIGH(11U) |          \
                                     PIN_OSPEED_HIGH(12U) |          \
                                     PIN_OSPEED_HIGH(GPIOC_BTN) |    \
                                     PIN_OSPEED_HIGH(GPIOC_MODE) |   \
                                     PIN_OSPEED_HIGH(15U))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_PULLUP(0U) |       \
                                     PIN_PUPDR_PULLUP(1U) |       \
                                     PIN_PUPDR_PULLUP(2U) |         \
                                     PIN_PUPDR_PULLUP(3U) |         \
                                     PIN_PUPDR_PULLUP(4U) |         \
                                     PIN_PUPDR_PULLUP(5U) |         \
                                     PIN_PUPDR_PULLUP(6U) |         \
                                     PIN_PUPDR_PULLUP(7U) |       \
                                     PIN_PUPDR_PULLUP(8U) |         \
                                     PIN_PUPDR_PULLUP(9U) |         \
                                     PIN_PUPDR_PULLUP(10U) |        \
                                     PIN_PUPDR_PULLUP(11U) |        \
                                     PIN_PUPDR_PULLUP(12U) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_BTN) |     \
                                     PIN_PUPDR_PULLUP(GPIOC_MODE) |   \
                                     PIN_PUPDR_PULLUP(15U))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(0U) |           \
                                     PIN_ODR_HIGH(1U) |           \
                                     PIN_ODR_HIGH(2U) |             \
                                     PIN_ODR_HIGH(3U) |             \
                                     PIN_ODR_HIGH(4U) |             \
                                     PIN_ODR_HIGH(5U) |             \
                                     PIN_ODR_HIGH(6U) |             \
                                     PIN_ODR_HIGH(7U) |           \
                                     PIN_ODR_HIGH(8U) |             \
                                     PIN_ODR_HIGH(9U) |             \
                                     PIN_ODR_HIGH(10U) |            \
                                     PIN_ODR_HIGH(11U) |            \
                                     PIN_ODR_HIGH(12U) |            \
                                     PIN_ODR_HIGH(GPIOC_BTN) |           \
                                     PIN_ODR_HIGH(GPIOC_MODE) |         \
                                     PIN_ODR_HIGH(15U))
#define VAL_GPIOC_AFRL              (0)
#define VAL_GPIOC_AFRH              (0)

/*
 * GPIOD setup:
 *
 * PD0  - PIN0                      (input pullup).
 * PD1  - PIN1                      (input pullup).
 * PD2  - PIN2                      (input pullup).
 * PD3  - PIN3                      (input pullup).
 * PD4  - PIN4                      (input pullup).
 * PD5  - PIN5                      (input pullup).
 * PD6  - PIN6                      (input pullup).
 * PD7  - PIN7                      (input pullup).
 * PD8  - PIN8                      (input pullup).
 * PD9  - PIN9                      (input pullup).
 * PD10 - PIN10                     (input pullup).
 * PD11 - PIN11                     (input pullup).
 * PD12 - PIN12                     (input pullup).
 * PD13 - PIN13                     (input pullup).
 * PD14 - PIN14                     (input pullup).
 * PD15 - PIN15                     (input pullup).
 */
#define VAL_GPIOD_MODER             (PIN_MODE_INPUT(0) |           \
                                     PIN_MODE_INPUT(1) |           \
                                     PIN_MODE_INPUT(2) |           \
                                     PIN_MODE_INPUT(3) |           \
                                     PIN_MODE_INPUT(4) |           \
                                     PIN_MODE_INPUT(5) |           \
                                     PIN_MODE_INPUT(6) |           \
                                     PIN_MODE_INPUT(7) |           \
                                     PIN_MODE_INPUT(8) |           \
                                     PIN_MODE_INPUT(9) |           \
                                     PIN_MODE_INPUT(10) |          \
                                     PIN_MODE_INPUT(11) |          \
                                     PIN_MODE_INPUT(12) |          \
                                     PIN_MODE_INPUT(13) |          \
                                     PIN_MODE_INPUT(14) |          \
                                     PIN_MODE_INPUT(15))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(0) |       \
                                     PIN_OTYPE_PUSHPULL(1) |       \
                                     PIN_OTYPE_PUSHPULL(2) |       \
                                     PIN_OTYPE_PUSHPULL(3) |       \
                                     PIN_OTYPE_PUSHPULL(4) |       \
                                     PIN_OTYPE_PUSHPULL(5) |       \
                                     PIN_OTYPE_PUSHPULL(6) |       \
                                     PIN_OTYPE_PUSHPULL(7) |       \
                                     PIN_OTYPE_PUSHPULL(8) |       \
                                     PIN_OTYPE_PUSHPULL(9) |       \
                                     PIN_OTYPE_PUSHPULL(10) |      \
                                     PIN_OTYPE_PUSHPULL(11) |      \
                                     PIN_OTYPE_PUSHPULL(12) |      \
                                     PIN_OTYPE_PUSHPULL(13) |      \
                                     PIN_OTYPE_PUSHPULL(14) |      \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_HIGH(0) |          \
                                     PIN_OSPEED_HIGH(1) |          \
                                     PIN_OSPEED_HIGH(2) |          \
                                     PIN_OSPEED_HIGH(3) |          \
                                     PIN_OSPEED_HIGH(4) |          \
                                     PIN_OSPEED_HIGH(5) |          \
                                     PIN_OSPEED_HIGH(6) |          \
                                     PIN_OSPEED_HIGH(7) |          \
                                     PIN_OSPEED_HIGH(8) |          \
                                     PIN_OSPEED_HIGH(9) |          \
                                     PIN_OSPEED_HIGH(10) |         \
                                     PIN_OSPEED_HIGH(11) |         \
                                     PIN_OSPEED_HIGH(12) |         \
                                     PIN_OSPEED_HIGH(13) |         \
                                     PIN_OSPEED_HIGH(14) |         \
                                     PIN_OSPEED_HIGH(15))
#define VAL_GPIOD_PUPDR             (PIN_PUPDR_PULLUP(0) |         \
                                     PIN_PUPDR_PULLUP(1) |         \
                                     PIN_PUPDR_PULLUP(2) |         \
                                     PIN_PUPDR_PULLUP(3) |         \
                                     PIN_PUPDR_PULLUP(4) |         \
                                     PIN_PUPDR_PULLUP(5) |         \
                                     PIN_PUPDR_PULLUP(6) |         \
                                     PIN_PUPDR_PULLUP(7) |         \
                                     PIN_PUPDR_PULLUP(8) |         \
                                     PIN_PUPDR_PULLUP(9) |         \
                                     PIN_PUPDR_PULLUP(10) |        \
                                     PIN_PUPDR_PULLUP(11) |        \
                                     PIN_PUPDR_PULLUP(12) |        \
                                     PIN_PUPDR_PULLUP(13) |        \
                                     PIN_PUPDR_PULLUP(14) |        \
                                     PIN_PUPDR_PULLUP(15))
#define VAL_GPIOD_ODR               (PIN_ODR_HIGH(0) |             \
                                     PIN_ODR_HIGH(1) |             \
                                     PIN_ODR_HIGH(2) |             \
                                     PIN_ODR_HIGH(3) |             \
                                     PIN_ODR_HIGH(4) |             \
                                     PIN_ODR_HIGH(5) |             \
                                     PIN_ODR_HIGH(6) |             \
                                     PIN_ODR_HIGH(7) |             \
                                     PIN_ODR_HIGH(8) |             \
                                     PIN_ODR_HIGH(9) |             \
                                     PIN_ODR_HIGH(10) |            \
                                     PIN_ODR_HIGH(11) |            \
                                     PIN_ODR_HIGH(12) |            \
                                     PIN_ODR_HIGH(13) |            \
                                     PIN_ODR_HIGH(14) |            \
                                     PIN_ODR_HIGH(15))
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(0, 0U) |          \
                                     PIN_AFIO_AF(1, 0U) |          \
                                     PIN_AFIO_AF(2, 0U) |          \
                                     PIN_AFIO_AF(3, 0U) |          \
                                     PIN_AFIO_AF(4, 0U) |          \
                                     PIN_AFIO_AF(5, 0U) |          \
                                     PIN_AFIO_AF(6, 0U) |          \
                                     PIN_AFIO_AF(7, 0U))
#define VAL_GPIOD_AFRH              (PIN_AFIO_AF(8, 0U) |          \
                                     PIN_AFIO_AF(9, 0U) |          \
                                     PIN_AFIO_AF(10, 0U) |         \
                                     PIN_AFIO_AF(11, 0U) |         \
                                     PIN_AFIO_AF(12, 0U) |         \
                                     PIN_AFIO_AF(13, 0U) |         \
                                     PIN_AFIO_AF(14, 0U) |         \
                                     PIN_AFIO_AF(15, 0U))

/*
 * GPIOE setup:
 *
 * PE0  - PIN0                      (input pullup).
 * PE1  - PIN1                      (input pullup).
 * PE2  - PIN2                      (input pullup).
 * PE3  - PIN3                      (input pullup).
 * PE4  - PIN4                      (input pullup).
 * PE5  - PIN5                      (input pullup).
 * PE6  - PIN6                      (input pullup).
 * PE7  - PIN7                      (input pullup).
 * PE8  - PIN8                      (input pullup).
 * PE9  - PIN9                      (input pullup).
 * PE10 - PIN10                     (input pullup).
 * PE11 - PIN11                     (input pullup).
 * PE12 - PIN12                     (input pullup).
 * PE13 - PIN13                     (input pullup).
 * PE14 - PIN14                     (input pullup).
 * PE15 - PIN15                     (input pullup).
 */
#define VAL_GPIOE_MODER             (PIN_MODE_INPUT(0) |           \
                                     PIN_MODE_INPUT(1) |           \
                                     PIN_MODE_INPUT(2) |           \
                                     PIN_MODE_INPUT(3) |           \
                                     PIN_MODE_INPUT(4) |           \
                                     PIN_MODE_INPUT(5) |           \
                                     PIN_MODE_INPUT(6) |           \
                                     PIN_MODE_INPUT(7) |           \
                                     PIN_MODE_INPUT(8) |           \
                                     PIN_MODE_INPUT(9) |           \
                                     PIN_MODE_INPUT(10) |          \
                                     PIN_MODE_INPUT(11) |          \
                                     PIN_MODE_INPUT(12) |          \
                                     PIN_MODE_INPUT(13) |          \
                                     PIN_MODE_INPUT(14) |          \
                                     PIN_MODE_INPUT(15))
#define VAL_GPIOE_OTYPER            (PIN_OTYPE_PUSHPULL(0) |       \
                                     PIN_OTYPE_PUSHPULL(1) |       \
                                     PIN_OTYPE_PUSHPULL(2) |       \
                                     PIN_OTYPE_PUSHPULL(3) |       \
                                     PIN_OTYPE_PUSHPULL(4) |       \
                                     PIN_OTYPE_PUSHPULL(5) |       \
                                     PIN_OTYPE_PUSHPULL(6) |       \
                                     PIN_OTYPE_PUSHPULL(7) |       \
                                     PIN_OTYPE_PUSHPULL(8) |       \
                                     PIN_OTYPE_PUSHPULL(9) |       \
                                     PIN_OTYPE_PUSHPULL(10) |      \
                                     PIN_OTYPE_PUSHPULL(11) |      \
                                     PIN_OTYPE_PUSHPULL(12) |      \
                                     PIN_OTYPE_PUSHPULL(13) |      \
                                     PIN_OTYPE_PUSHPULL(14) |      \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOE_OSPEEDR           (PIN_OSPEED_HIGH(0) |          \
                                     PIN_OSPEED_HIGH(1) |          \
                                     PIN_OSPEED_HIGH(2) |          \
                                     PIN_OSPEED_HIGH(3) |          \
                                     PIN_OSPEED_HIGH(4) |          \
                                     PIN_OSPEED_HIGH(5) |          \
                                     PIN_OSPEED_HIGH(6) |          \
                                     PIN_OSPEED_HIGH(7) |          \
                                     PIN_OSPEED_HIGH(8) |          \
                                     PIN_OSPEED_HIGH(9) |          \
                                     PIN_OSPEED_HIGH(10) |         \
                                     PIN_OSPEED_HIGH(11) |         \
                                     PIN_OSPEED_HIGH(12) |         \
                                     PIN_OSPEED_HIGH(13) |         \
                                     PIN_OSPEED_HIGH(14) |         \
                                     PIN_OSPEED_HIGH(15))
#define VAL_GPIOE_PUPDR             (PIN_PUPDR_PULLUP(0) |         \
                                     PIN_PUPDR_PULLUP(1) |         \
                                     PIN_PUPDR_PULLUP(2) |         \
                                     PIN_PUPDR_PULLUP(3) |         \
                                     PIN_PUPDR_PULLUP(4) |         \
                                     PIN_PUPDR_PULLUP(5) |         \
                                     PIN_PUPDR_PULLUP(6) |         \
                                     PIN_PUPDR_PULLUP(7) |         \
                                     PIN_PUPDR_PULLUP(8) |         \
                                     PIN_PUPDR_PULLUP(9) |         \
                                     PIN_PUPDR_PULLUP(10) |        \
                                     PIN_PUPDR_PULLUP(11) |        \
                                     PIN_PUPDR_PULLUP(12) |        \
                                     PIN_PUPDR_PULLUP(13) |        \
                                     PIN_PUPDR_PULLUP(14) |        \
                                     PIN_PUPDR_PULLUP(15))
#define VAL_GPIOE_ODR               (PIN_ODR_HIGH(0) |             \
                                     PIN_ODR_HIGH(1) |             \
                                     PIN_ODR_HIGH(2) |             \
                                     PIN_ODR_HIGH(3) |             \
                                     PIN_ODR_HIGH(4) |             \
                                     PIN_ODR_HIGH(5) |             \
                                     PIN_ODR_HIGH(6) |             \
                                     PIN_ODR_HIGH(7) |             \
                                     PIN_ODR_HIGH(8) |             \
                                     PIN_ODR_HIGH(9) |             \
                                     PIN_ODR_HIGH(10) |            \
                                     PIN_ODR_HIGH(11) |            \
                                     PIN_ODR_HIGH(12) |            \
                                     PIN_ODR_HIGH(13) |            \
                                     PIN_ODR_HIGH(14) |            \
                                     PIN_ODR_HIGH(15))
#define VAL_GPIOE_AFRL              (PIN_AFIO_AF(0, 0U) |          \
                                     PIN_AFIO_AF(1, 0U) |          \
                                     PIN_AFIO_AF(2, 0U) |          \
                                     PIN_AFIO_AF(3, 0U) |          \
                                     PIN_AFIO_AF(4, 0U) |          \
                                     PIN_AFIO_AF(5, 0U) |          \
                                     PIN_AFIO_AF(6, 0U) |          \
                                     PIN_AFIO_AF(7, 0U))
#define VAL_GPIOE_AFRH              (PIN_AFIO_AF(8, 0U) |          \
                                     PIN_AFIO_AF(9, 0U) |          \
                                     PIN_AFIO_AF(10, 0U) |         \
                                     PIN_AFIO_AF(11, 0U) |         \
                                     PIN_AFIO_AF(12, 0U) |         \
                                     PIN_AFIO_AF(13, 0U) |         \
                                     PIN_AFIO_AF(14, 0U) |         \
                                     PIN_AFIO_AF(15, 0U))

/*
 * GPIOF setup:
 *
 * PF0  - PIN0                      (input pullup).
 * PF1  - PIN1                      (input pullup).
 * PF2  - PIN2                      (input pullup).
 * PF3  - PIN3                      (input pullup).
 * PF4  - PIN4                      (input pullup).
 * PF5  - PIN5                      (input pullup).
 * PF6  - PIN6                      (input pullup).
 * PF7  - PIN7                      (input pullup).
 * PF8  - PIN8                      (input pullup).
 * PF9  - PIN9                      (input pullup).
 * PF10 - PIN10                     (input pullup).
 * PF11 - PIN11                     (input pullup).
 * PF12 - PIN12                     (input pullup).
 * PF13 - PIN13                     (input pullup).
 * PF14 - PIN14                     (input pullup).
 * PF15 - PIN15                     (input pullup).
 */
#define VAL_GPIOF_MODER             (PIN_MODE_INPUT(0) |           \
                                     PIN_MODE_INPUT(1) |           \
                                     PIN_MODE_INPUT(2) |           \
                                     PIN_MODE_INPUT(3) |           \
                                     PIN_MODE_INPUT(4) |           \
                                     PIN_MODE_INPUT(5) |           \
                                     PIN_MODE_INPUT(6) |           \
                                     PIN_MODE_INPUT(7) |           \
                                     PIN_MODE_INPUT(8) |           \
                                     PIN_MODE_INPUT(9) |           \
                                     PIN_MODE_INPUT(10) |          \
                                     PIN_MODE_INPUT(11) |          \
                                     PIN_MODE_INPUT(12) |          \
                                     PIN_MODE_INPUT(13) |          \
                                     PIN_MODE_INPUT(14) |          \
                                     PIN_MODE_INPUT(15))
#define VAL_GPIOF_OTYPER            (PIN_OTYPE_PUSHPULL(0) |       \
                                     PIN_OTYPE_PUSHPULL(1) |       \
                                     PIN_OTYPE_PUSHPULL(2) |       \
                                     PIN_OTYPE_PUSHPULL(3) |       \
                                     PIN_OTYPE_PUSHPULL(4) |       \
                                     PIN_OTYPE_PUSHPULL(5) |       \
                                     PIN_OTYPE_PUSHPULL(6) |       \
                                     PIN_OTYPE_PUSHPULL(7) |       \
                                     PIN_OTYPE_PUSHPULL(8) |       \
                                     PIN_OTYPE_PUSHPULL(9) |       \
                                     PIN_OTYPE_PUSHPULL(10) |      \
                                     PIN_OTYPE_PUSHPULL(11) |      \
                                     PIN_OTYPE_PUSHPULL(12) |      \
                                     PIN_OTYPE_PUSHPULL(13) |      \
                                     PIN_OTYPE_PUSHPULL(14) |      \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOF_OSPEEDR           (PIN_OSPEED_HIGH(0) |          \
                                     PIN_OSPEED_HIGH(1) |          \
                                     PIN_OSPEED_HIGH(2) |          \
                                     PIN_OSPEED_HIGH(3) |          \
                                     PIN_OSPEED_HIGH(4) |          \
                                     PIN_OSPEED_HIGH(5) |          \
                                     PIN_OSPEED_HIGH(6) |          \
                                     PIN_OSPEED_HIGH(7) |          \
                                     PIN_OSPEED_HIGH(8) |          \
                                     PIN_OSPEED_HIGH(9) |          \
                                     PIN_OSPEED_HIGH(10) |         \
                                     PIN_OSPEED_HIGH(11) |         \
                                     PIN_OSPEED_HIGH(12) |         \
                                     PIN_OSPEED_HIGH(13) |         \
                                     PIN_OSPEED_HIGH(14) |         \
                                     PIN_OSPEED_HIGH(15))
#define VAL_GPIOF_PUPDR             (PIN_PUPDR_PULLUP(0) |         \
                                     PIN_PUPDR_PULLUP(1) |         \
                                     PIN_PUPDR_PULLUP(2) |         \
                                     PIN_PUPDR_PULLUP(3) |         \
                                     PIN_PUPDR_PULLUP(4) |         \
                                     PIN_PUPDR_PULLUP(5) |         \
                                     PIN_PUPDR_PULLUP(6) |         \
                                     PIN_PUPDR_PULLUP(7) |         \
                                     PIN_PUPDR_PULLUP(8) |         \
                                     PIN_PUPDR_PULLUP(9) |         \
                                     PIN_PUPDR_PULLUP(10) |        \
                                     PIN_PUPDR_PULLUP(11) |        \
                                     PIN_PUPDR_PULLUP(12) |        \
                                     PIN_PUPDR_PULLUP(13) |        \
                                     PIN_PUPDR_PULLUP(14) |        \
                                     PIN_PUPDR_PULLUP(15))
#define VAL_GPIOF_ODR               (PIN_ODR_HIGH(0) |             \
                                     PIN_ODR_HIGH(1) |             \
                                     PIN_ODR_HIGH(2) |             \
                                     PIN_ODR_HIGH(3) |             \
                                     PIN_ODR_HIGH(4) |             \
                                     PIN_ODR_HIGH(5) |             \
                                     PIN_ODR_HIGH(6) |             \
                                     PIN_ODR_HIGH(7) |             \
                                     PIN_ODR_HIGH(8) |             \
                                     PIN_ODR_HIGH(9) |             \
                                     PIN_ODR_HIGH(10) |            \
                                     PIN_ODR_HIGH(11) |            \
                                     PIN_ODR_HIGH(12) |            \
                                     PIN_ODR_HIGH(13) |            \
                                     PIN_ODR_HIGH(14) |            \
                                     PIN_ODR_HIGH(15))
#define VAL_GPIOF_AFRL              (PIN_AFIO_AF(0, 0U) |          \
                                     PIN_AFIO_AF(1, 0U) |          \
                                     PIN_AFIO_AF(2, 0U) |          \
                                     PIN_AFIO_AF(3, 0U) |          \
                                     PIN_AFIO_AF(4, 0U) |          \
                                     PIN_AFIO_AF(5, 0U) |          \
                                     PIN_AFIO_AF(6, 0U) |          \
                                     PIN_AFIO_AF(7, 0U))
#define VAL_GPIOF_AFRH              (PIN_AFIO_AF(8, 0U) |          \
                                     PIN_AFIO_AF(9, 0U) |          \
                                     PIN_AFIO_AF(10, 0U) |         \
                                     PIN_AFIO_AF(11, 0U) |         \
                                     PIN_AFIO_AF(12, 0U) |         \
                                     PIN_AFIO_AF(13, 0U) |         \
                                     PIN_AFIO_AF(14, 0U) |         \
                                     PIN_AFIO_AF(15, 0U))

/*
 * GPIOG setup:
 *
 * PG0  - PIN0                      (input pullup).
 * PG1  - PIN1                      (input pullup).
 * PG2  - PIN2                      (input pullup).
 * PG3  - PIN3                      (input pullup).
 * PG4  - PIN4                      (input pullup).
 * PG5  - PIN5                      (input pullup).
 * PG6  - PIN6                      (input pullup).
 * PG7  - PIN7                      (input pullup).
 * PG8  - PIN8                      (input pullup).
 * PG9  - PIN9                      (input pullup).
 * PG10 - PIN10                     (input pullup).
 * PG11 - PIN11                     (input pullup).
 * PG12 - PIN12                     (input pullup).
 * PG13 - PIN13                     (input pullup).
 * PG14 - PIN14                     (input pullup).
 * PG15 - PIN15                     (input pullup).
 */
#define VAL_GPIOG_MODER             (PIN_MODE_INPUT(0) |           \
                                     PIN_MODE_INPUT(1) |           \
                                     PIN_MODE_INPUT(2) |           \
                                     PIN_MODE_INPUT(3) |           \
                                     PIN_MODE_INPUT(4) |           \
                                     PIN_MODE_INPUT(5) |           \
                                     PIN_MODE_INPUT(6) |           \
                                     PIN_MODE_INPUT(7) |           \
                                     PIN_MODE_INPUT(8) |           \
                                     PIN_MODE_INPUT(9) |           \
                                     PIN_MODE_INPUT(10) |          \
                                     PIN_MODE_INPUT(11) |          \
                                     PIN_MODE_INPUT(12) |          \
                                     PIN_MODE_INPUT(13) |          \
                                     PIN_MODE_INPUT(14) |          \
                                     PIN_MODE_INPUT(15))
#define VAL_GPIOG_OTYPER            (PIN_OTYPE_PUSHPULL(0) |       \
                                     PIN_OTYPE_PUSHPULL(1) |       \
                                     PIN_OTYPE_PUSHPULL(2) |       \
                                     PIN_OTYPE_PUSHPULL(3) |       \
                                     PIN_OTYPE_PUSHPULL(4) |       \
                                     PIN_OTYPE_PUSHPULL(5) |       \
                                     PIN_OTYPE_PUSHPULL(6) |       \
                                     PIN_OTYPE_PUSHPULL(7) |       \
                                     PIN_OTYPE_PUSHPULL(8) |       \
                                     PIN_OTYPE_PUSHPULL(9) |       \
                                     PIN_OTYPE_PUSHPULL(10) |      \
                                     PIN_OTYPE_PUSHPULL(11) |      \
                                     PIN_OTYPE_PUSHPULL(12) |      \
                                     PIN_OTYPE_PUSHPULL(13) |      \
                                     PIN_OTYPE_PUSHPULL(14) |      \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOG_OSPEEDR           (PIN_OSPEED_HIGH(0) |          \
                                     PIN_OSPEED_HIGH(1) |          \
                                     PIN_OSPEED_HIGH(2) |          \
                                     PIN_OSPEED_HIGH(3) |          \
                                     PIN_OSPEED_HIGH(4) |          \
                                     PIN_OSPEED_HIGH(5) |          \
                                     PIN_OSPEED_HIGH(6) |          \
                                     PIN_OSPEED_HIGH(7) |          \
                                     PIN_OSPEED_HIGH(8) |          \
                                     PIN_OSPEED_HIGH(9) |          \
                                     PIN_OSPEED_HIGH(10) |         \
                                     PIN_OSPEED_HIGH(11) |         \
                                     PIN_OSPEED_HIGH(12) |         \
                                     PIN_OSPEED_HIGH(13) |         \
                                     PIN_OSPEED_HIGH(14) |         \
                                     PIN_OSPEED_HIGH(15))
#define VAL_GPIOG_PUPDR             (PIN_PUPDR_PULLUP(0) |         \
                                     PIN_PUPDR_PULLUP(1) |         \
                                     PIN_PUPDR_PULLUP(2) |         \
                                     PIN_PUPDR_PULLUP(3) |         \
                                     PIN_PUPDR_PULLUP(4) |         \
                                     PIN_PUPDR_PULLUP(5) |         \
                                     PIN_PUPDR_PULLUP(6) |         \
                                     PIN_PUPDR_PULLUP(7) |         \
                                     PIN_PUPDR_PULLUP(8) |         \
                                     PIN_PUPDR_PULLUP(9) |         \
                                     PIN_PUPDR_PULLUP(10) |        \
                                     PIN_PUPDR_PULLUP(11) |        \
                                     PIN_PUPDR_PULLUP(12) |        \
                                     PIN_PUPDR_PULLUP(13) |        \
                                     PIN_PUPDR_PULLUP(14) |        \
                                     PIN_PUPDR_PULLUP(15))
#define VAL_GPIOG_ODR               (PIN_ODR_HIGH(0) |             \
                                     PIN_ODR_HIGH(1) |             \
                                     PIN_ODR_HIGH(2) |             \
                                     PIN_ODR_HIGH(3) |             \
                                     PIN_ODR_HIGH(4) |             \
                                     PIN_ODR_HIGH(5) |             \
                                     PIN_ODR_HIGH(6) |             \
                                     PIN_ODR_HIGH(7) |             \
                                     PIN_ODR_HIGH(8) |             \
                                     PIN_ODR_HIGH(9) |             \
                                     PIN_ODR_HIGH(10) |            \
                                     PIN_ODR_HIGH(11) |            \
                                     PIN_ODR_HIGH(12) |            \
                                     PIN_ODR_HIGH(13) |            \
                                     PIN_ODR_HIGH(14) |            \
                                     PIN_ODR_HIGH(15))
#define VAL_GPIOG_AFRL              (PIN_AFIO_AF(0, 0U) |          \
                                     PIN_AFIO_AF(1, 0U) |          \
                                     PIN_AFIO_AF(2, 0U) |          \
                                     PIN_AFIO_AF(3, 0U) |          \
                                     PIN_AFIO_AF(4, 0U) |          \
                                     PIN_AFIO_AF(5, 0U) |          \
                                     PIN_AFIO_AF(6, 0U) |          \
                                     PIN_AFIO_AF(7, 0U))
#define VAL_GPIOG_AFRH              (PIN_AFIO_AF(8, 0U) |          \
                                     PIN_AFIO_AF(9, 0U) |          \
                                     PIN_AFIO_AF(10, 0U) |         \
                                     PIN_AFIO_AF(11, 0U) |         \
                                     PIN_AFIO_AF(12, 0U) |         \
                                     PIN_AFIO_AF(13, 0U) |         \
                                     PIN_AFIO_AF(14, 0U) |         \
                                     PIN_AFIO_AF(15, 0U))

/*
 * GPIOH setup:
 *
 * PH0  - OSC_IN                    (input floating).
 * PH1  - OSC_OUT                   (input floating).
 * PH2  - PIN2                      (input pullup).
 * PH3  - PIN3                      (input pullup).
 * PH4  - PIN4                      (input pullup).
 * PH5  - PIN5                      (input pullup).
 * PH6  - PIN6                      (input pullup).
 * PH7  - PIN7                      (input pullup).
 * PH8  - PIN8                      (input pullup).
 * PH9  - PIN9                      (input pullup).
 * PH10 - PIN10                     (input pullup).
 * PH11 - PIN11                     (input pullup).
 * PH12 - PIN12                     (input pullup).
 * PH13 - PIN13                     (input pullup).
 * PH14 - PIN14                     (input pullup).
 * PH15 - PIN15                     (input pullup).
 */
#define VAL_GPIOH_MODER             (PIN_MODE_INPUT(GPIOH_OSC_IN) |         \
                                     PIN_MODE_INPUT(GPIOH_OSC_OUT) |        \
                                     PIN_MODE_INPUT(2) |           \
                                     PIN_MODE_INPUT(3) |           \
                                     PIN_MODE_INPUT(4) |           \
                                     PIN_MODE_INPUT(5) |           \
                                     PIN_MODE_INPUT(6) |           \
                                     PIN_MODE_INPUT(7) |           \
                                     PIN_MODE_INPUT(8) |           \
                                     PIN_MODE_INPUT(9) |           \
                                     PIN_MODE_INPUT(10) |          \
                                     PIN_MODE_INPUT(11) |          \
                                     PIN_MODE_INPUT(12) |          \
                                     PIN_MODE_INPUT(13) |          \
                                     PIN_MODE_INPUT(14) |          \
                                     PIN_MODE_INPUT(15))
#define VAL_GPIOH_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOH_OSC_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOH_OSC_OUT) |    \
                                     PIN_OTYPE_PUSHPULL(2) |       \
                                     PIN_OTYPE_PUSHPULL(3) |       \
                                     PIN_OTYPE_PUSHPULL(4) |       \
                                     PIN_OTYPE_PUSHPULL(5) |       \
                                     PIN_OTYPE_PUSHPULL(6) |       \
                                     PIN_OTYPE_PUSHPULL(7) |       \
                                     PIN_OTYPE_PUSHPULL(8) |       \
                                     PIN_OTYPE_PUSHPULL(9) |       \
                                     PIN_OTYPE_PUSHPULL(10) |      \
                                     PIN_OTYPE_PUSHPULL(11) |      \
                                     PIN_OTYPE_PUSHPULL(12) |      \
                                     PIN_OTYPE_PUSHPULL(13) |      \
                                     PIN_OTYPE_PUSHPULL(14) |      \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOH_OSPEEDR           (PIN_OSPEED_HIGH(GPIOH_OSC_IN) |        \
                                     PIN_OSPEED_HIGH(GPIOH_OSC_OUT) |       \
                                     PIN_OSPEED_HIGH(2) |          \
                                     PIN_OSPEED_HIGH(3) |          \
                                     PIN_OSPEED_HIGH(4) |          \
                                     PIN_OSPEED_HIGH(5) |          \
                                     PIN_OSPEED_HIGH(6) |          \
                                     PIN_OSPEED_HIGH(7) |          \
                                     PIN_OSPEED_HIGH(8) |          \
                                     PIN_OSPEED_HIGH(9) |          \
                                     PIN_OSPEED_HIGH(10) |         \
                                     PIN_OSPEED_HIGH(11) |         \
                                     PIN_OSPEED_HIGH(12) |         \
                                     PIN_OSPEED_HIGH(13) |         \
                                     PIN_OSPEED_HIGH(14) |         \
                                     PIN_OSPEED_HIGH(15))
#define VAL_GPIOH_PUPDR             (PIN_PUPDR_FLOATING(GPIOH_OSC_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOH_OSC_OUT) |    \
                                     PIN_PUPDR_PULLUP(2) |         \
                                     PIN_PUPDR_PULLUP(3) |         \
                                     PIN_PUPDR_PULLUP(4) |         \
                                     PIN_PUPDR_PULLUP(5) |         \
                                     PIN_PUPDR_PULLUP(6) |         \
                                     PIN_PUPDR_PULLUP(7) |         \
                                     PIN_PUPDR_PULLUP(8) |         \
                                     PIN_PUPDR_PULLUP(9) |         \
                                     PIN_PUPDR_PULLUP(10) |        \
                                     PIN_PUPDR_PULLUP(11) |        \
                                     PIN_PUPDR_PULLUP(12) |        \
                                     PIN_PUPDR_PULLUP(13) |        \
                                     PIN_PUPDR_PULLUP(14) |        \
                                     PIN_PUPDR_PULLUP(15))
#define VAL_GPIOH_ODR               (PIN_ODR_HIGH(GPIOH_OSC_IN) |           \
                                     PIN_ODR_HIGH(GPIOH_OSC_OUT) |          \
                                     PIN_ODR_HIGH(2) |             \
                                     PIN_ODR_HIGH(3) |             \
                                     PIN_ODR_HIGH(4) |             \
                                     PIN_ODR_HIGH(5) |             \
                                     PIN_ODR_HIGH(6) |             \
                                     PIN_ODR_HIGH(7) |             \
                                     PIN_ODR_HIGH(8) |             \
                                     PIN_ODR_HIGH(9) |             \
                                     PIN_ODR_HIGH(10) |            \
                                     PIN_ODR_HIGH(11) |            \
                                     PIN_ODR_HIGH(12) |            \
                                     PIN_ODR_HIGH(13) |            \
                                     PIN_ODR_HIGH(14) |            \
                                     PIN_ODR_HIGH(15))
#define VAL_GPIOH_AFRL              (PIN_AFIO_AF(GPIOH_OSC_IN, 0U) |        \
                                     PIN_AFIO_AF(GPIOH_OSC_OUT, 0U) |       \
                                     PIN_AFIO_AF(2, 0U) |          \
                                     PIN_AFIO_AF(3, 0U) |          \
                                     PIN_AFIO_AF(4, 0U) |          \
                                     PIN_AFIO_AF(5, 0U) |          \
                                     PIN_AFIO_AF(6, 0U) |          \
                                     PIN_AFIO_AF(7, 0U))
#define VAL_GPIOH_AFRH              (PIN_AFIO_AF(8, 0U) |          \
                                     PIN_AFIO_AF(9, 0U) |          \
                                     PIN_AFIO_AF(10, 0U) |         \
                                     PIN_AFIO_AF(11, 0U) |         \
                                     PIN_AFIO_AF(12, 0U) |         \
                                     PIN_AFIO_AF(13, 0U) |         \
                                     PIN_AFIO_AF(14, 0U) |         \
                                     PIN_AFIO_AF(15, 0U))

/*
 * GPIOI setup:
 *
 * PI0  - PIN0                      (input pullup).
 * PI1  - PIN1                      (input pullup).
 * PI2  - PIN2                      (input pullup).
 * PI3  - PIN3                      (input pullup).
 * PI4  - PIN4                      (input pullup).
 * PI5  - PIN5                      (input pullup).
 * PI6  - PIN6                      (input pullup).
 * PI7  - PIN7                      (input pullup).
 * PI8  - PIN8                      (input pullup).
 * PI9  - PIN9                      (input pullup).
 * PI10 - PIN10                     (input pullup).
 * PI11 - PIN11                     (input pullup).
 * PI12 - PIN12                     (input pullup).
 * PI13 - PIN13                     (input pullup).
 * PI14 - PIN14                     (input pullup).
 * PI15 - PIN15                     (input pullup).
 */
#define VAL_GPIOI_MODER             (PIN_MODE_INPUT(0) |           \
                                     PIN_MODE_INPUT(1) |           \
                                     PIN_MODE_INPUT(2) |           \
                                     PIN_MODE_INPUT(3) |           \
                                     PIN_MODE_INPUT(4) |           \
                                     PIN_MODE_INPUT(5) |           \
                                     PIN_MODE_INPUT(6) |           \
                                     PIN_MODE_INPUT(7) |           \
                                     PIN_MODE_INPUT(8) |           \
                                     PIN_MODE_INPUT(9) |           \
                                     PIN_MODE_INPUT(10) |          \
                                     PIN_MODE_INPUT(11) |          \
                                     PIN_MODE_INPUT(12) |          \
                                     PIN_MODE_INPUT(13) |          \
                                     PIN_MODE_INPUT(14) |          \
                                     PIN_MODE_INPUT(15))
#define VAL_GPIOI_OTYPER            (PIN_OTYPE_PUSHPULL(0) |       \
                                     PIN_OTYPE_PUSHPULL(1) |       \
                                     PIN_OTYPE_PUSHPULL(2) |       \
                                     PIN_OTYPE_PUSHPULL(3) |       \
                                     PIN_OTYPE_PUSHPULL(4) |       \
                                     PIN_OTYPE_PUSHPULL(5) |       \
                                     PIN_OTYPE_PUSHPULL(6) |       \
                                     PIN_OTYPE_PUSHPULL(7) |       \
                                     PIN_OTYPE_PUSHPULL(8) |       \
                                     PIN_OTYPE_PUSHPULL(9) |       \
                                     PIN_OTYPE_PUSHPULL(10) |      \
                                     PIN_OTYPE_PUSHPULL(11) |      \
                                     PIN_OTYPE_PUSHPULL(12) |      \
                                     PIN_OTYPE_PUSHPULL(13) |      \
                                     PIN_OTYPE_PUSHPULL(14) |      \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOI_OSPEEDR           (PIN_OSPEED_HIGH(0) |          \
                                     PIN_OSPEED_HIGH(1) |          \
                                     PIN_OSPEED_HIGH(2) |          \
                                     PIN_OSPEED_HIGH(3) |          \
                                     PIN_OSPEED_HIGH(4) |          \
                                     PIN_OSPEED_HIGH(5) |          \
                                     PIN_OSPEED_HIGH(6) |          \
                                     PIN_OSPEED_HIGH(7) |          \
                                     PIN_OSPEED_HIGH(8) |          \
                                     PIN_OSPEED_HIGH(9) |          \
                                     PIN_OSPEED_HIGH(10) |         \
                                     PIN_OSPEED_HIGH(11) |         \
                                     PIN_OSPEED_HIGH(12) |         \
                                     PIN_OSPEED_HIGH(13) |         \
                                     PIN_OSPEED_HIGH(14) |         \
                                     PIN_OSPEED_HIGH(15))
#define VAL_GPIOI_PUPDR             (PIN_PUPDR_PULLUP(0) |         \
                                     PIN_PUPDR_PULLUP(1) |         \
                                     PIN_PUPDR_PULLUP(2) |         \
                                     PIN_PUPDR_PULLUP(3) |         \
                                     PIN_PUPDR_PULLUP(4) |         \
                                     PIN_PUPDR_PULLUP(5) |         \
                                     PIN_PUPDR_PULLUP(6) |         \
                                     PIN_PUPDR_PULLUP(7) |         \
                                     PIN_PUPDR_PULLUP(8) |         \
                                     PIN_PUPDR_PULLUP(9) |         \
                                     PIN_PUPDR_PULLUP(10) |        \
                                     PIN_PUPDR_PULLUP(11) |        \
                                     PIN_PUPDR_PULLUP(12) |        \
                                     PIN_PUPDR_PULLUP(13) |        \
                                     PIN_PUPDR_PULLUP(14) |        \
                                     PIN_PUPDR_PULLUP(15))
#define VAL_GPIOI_ODR               (PIN_ODR_HIGH(0) |             \
                                     PIN_ODR_HIGH(1) |             \
                                     PIN_ODR_HIGH(2) |             \
                                     PIN_ODR_HIGH(3) |             \
                                     PIN_ODR_HIGH(4) |             \
                                     PIN_ODR_HIGH(5) |             \
                                     PIN_ODR_HIGH(6) |             \
                                     PIN_ODR_HIGH(7) |             \
                                     PIN_ODR_HIGH(8) |             \
                                     PIN_ODR_HIGH(9) |             \
                                     PIN_ODR_HIGH(10) |            \
                                     PIN_ODR_HIGH(11) |            \
                                     PIN_ODR_HIGH(12) |            \
                                     PIN_ODR_HIGH(13) |            \
                                     PIN_ODR_HIGH(14) |            \
                                     PIN_ODR_HIGH(15))
#define VAL_GPIOI_AFRL              (PIN_AFIO_AF(0, 0U) |          \
                                     PIN_AFIO_AF(1, 0U) |          \
                                     PIN_AFIO_AF(2, 0U) |          \
                                     PIN_AFIO_AF(3, 0U) |          \
                                     PIN_AFIO_AF(4, 0U) |          \
                                     PIN_AFIO_AF(5, 0U) |          \
                                     PIN_AFIO_AF(6, 0U) |          \
                                     PIN_AFIO_AF(7, 0U))
#define VAL_GPIOI_AFRH              (PIN_AFIO_AF(8, 0U) |          \
                                     PIN_AFIO_AF(9, 0U) |          \
                                     PIN_AFIO_AF(10, 0U) |         \
                                     PIN_AFIO_AF(11, 0U) |         \
                                     PIN_AFIO_AF(12, 0U) |         \
                                     PIN_AFIO_AF(13, 0U) |         \
                                     PIN_AFIO_AF(14, 0U) |         \
                                     PIN_AFIO_AF(15, 0U))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
