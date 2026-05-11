/*
 * Board configuration — WaveShare ESP32-S3-Touch-LCD-4.3B
 *
 * Pin assignments and peripheral capabilities for the v0.2 firmware.
 * Will be expanded into per-variant capability flags (HAS_*) by
 * Workstream 0 prereqs #47 (board config + canvas) and #49 (PRODUCT_VARIANT).
 *
 * Reference: WaveShare wiki + waveshareteam/ESP32-S3-Touch-LCD-4.3B/examples/ESP-IDF
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Board identification ---- */
#define BOARD_NAME              "WaveShare ESP32-S3-Touch-LCD-4.3B"
#define BOARD_MCU               "ESP32-S3-WROOM-1-N16R8"

/* ---- Display (ST7262 16-bit RGB parallel) — wiring only; init in display driver ---- */
#define DISPLAY_WIDTH_PX        800
#define DISPLAY_HEIGHT_PX       480
#define DISPLAY_PIXEL_CLK_HZ    (16 * 1000 * 1000)

#define LCD_PIN_VSYNC           3
#define LCD_PIN_HSYNC           46
#define LCD_PIN_DE              5
#define LCD_PIN_PCLK            7

/* ---- I2C0 (shared bus) ---- */
#define I2C0_SDA_GPIO           8
#define I2C0_SCL_GPIO           9
#define I2C0_FREQ_HZ            400000

/* I2C device addresses on the shared bus. */
#define I2C_ADDR_CH422G_MODE    0x24    /* CH422G mode register */
#define I2C_ADDR_CH422G_OD      0x23    /* CH422G open-drain output */
#define I2C_ADDR_CH422G_IO      0x38    /* CH422G push-pull output AND read-back */
#define I2C_ADDR_GT911_TOUCH    0x5D    /* GT911 capacitive touch */
#define I2C_ADDR_PCF85063A_RTC  0x51    /* PCF85063A RTC */
/* I2C_ADDR_BNO085 = 0x4A or 0x4B   (set when IMU/GPS variant integrates) */

/* ---- Touch (GT911) ---- */
#define TOUCH_INT_GPIO          4       /* GT911 IRQ */
/* Touch reset is via CH422G EXIO1 — see CH422G section below */

/* ---- SD card (SPI mode) ---- */
#define SD_PIN_MOSI             11
#define SD_PIN_SCK              12
#define SD_PIN_MISO             13
/* SD card CS is via CH422G EXIO4 — see CH422G section below */

/* ---- TWAI / NMEA 2000 (CAN bus) ---- */
#define TWAI_TX_GPIO            15
#define TWAI_RX_GPIO            16
#define TWAI_BITRATE_BPS        250000  /* NMEA 2000 standard */

/* ---- RS485 (HWT901B IMU on IMU/GPS variant; serial GPS option always) ---- */
#define RS485_RX_GPIO           43
#define RS485_TX_GPIO           44

/* ---- CH422G I/O expander pin functions ----
 *
 * The CH422G is a multi-function I/O expander. On the WaveShare 4.3B,
 * pins are wired to specific functions and the firmware accesses them
 * by named role rather than raw pin number.
 *
 *   EXIO0  Reserved (do not drive)
 *   EXIO1  Touch reset (active LOW — pulse low to reset GT911)
 *   EXIO2  LCD backlight enable (active HIGH — high = on)
 *   EXIO3  LCD reset (active HIGH — high = released, low = held in reset)
 *   EXIO4  SD card chip select (active LOW — low = selected)
 *   EXIO5  USB/CAN multiplexer (HIGH = CAN routed to GPIO 15/16, LOW = USB)
 *
 * Pin numbers used by the CH422G driver are the bit positions in the
 * IO output register at I2C address 0x38.
 */
#define CH422G_EXIO_TOUCH_RESET     1
#define CH422G_EXIO_LCD_BACKLIGHT   2
#define CH422G_EXIO_LCD_RESET       3
#define CH422G_EXIO_SD_CS           4
#define CH422G_EXIO_USB_CAN_SEL     5

#define CH422G_EXIO_TOUCH_RESET_BIT     (1U << CH422G_EXIO_TOUCH_RESET)
#define CH422G_EXIO_LCD_BACKLIGHT_BIT   (1U << CH422G_EXIO_LCD_BACKLIGHT)
#define CH422G_EXIO_LCD_RESET_BIT       (1U << CH422G_EXIO_LCD_RESET)
#define CH422G_EXIO_SD_CS_BIT           (1U << CH422G_EXIO_SD_CS)
#define CH422G_EXIO_USB_CAN_SEL_BIT     (1U << CH422G_EXIO_USB_CAN_SEL)

/*
 * Initial CH422G state at boot (per WaveShare reference 0x0A pattern, with
 * variations for our anchor alarm needs):
 *   - Touch reset RELEASED (high)
 *   - LCD backlight OFF (off until display driver brings it up)
 *   - LCD reset RELEASED (high)
 *   - SD CS DEASSERTED (high — driver pulls low when selecting)
 *   - USB/CAN: USB mode at boot (low) — switch to CAN later when N2K initializes
 *
 * Bits set:    EXIO1 + EXIO3 + EXIO4 = 0b00011010 = 0x1A
 * Bits clear:  EXIO2 (backlight off), EXIO5 (USB mode)
 */
#define CH422G_EXIO_INIT_STATE \
    (CH422G_EXIO_TOUCH_RESET_BIT | CH422G_EXIO_LCD_RESET_BIT | CH422G_EXIO_SD_CS_BIT)

#ifdef __cplusplus
}
#endif
