/**
 * @file    ili9341.h
 * @brief   TFT LCD ILI9341 240x320, giao tiep song song 16 bit qua FSMC.
 *
 * Ket noi tren kit STM32F407VET6: FSMC_NE1 (PD7) -> CS, FSMC_A18 (PD13) -> RS (DC),
 * FSMC_NOE (PD4) -> RD, FSMC_NWE (PD5) -> WR, FSMC_D0..D15 -> DB0..DB15.
 *
 * @author  Khue Nguyen
 * @website khuenguyencreator.com
 */
#ifndef __ILI9341_H
#define __ILI9341_H

#include "main.h"

/* Bank 1, NE1 bat dau tu 0x60000000.
 * RS noi voi A18. Bus 16 bit nen A18 ung voi bit 19 cua dia chi: 1 << 19 = 0x80000.
 * Ghi vao LCD_REG: RS = 0 (lenh). Ghi vao LCD_RAM: RS = 1 (du lieu). */
#define LCD_REG   (*((volatile uint16_t *)0x60000000))
#define LCD_RAM   (*((volatile uint16_t *)0x60080000))

#define ILI9341_WIDTH   240
#define ILI9341_HEIGHT  320

/* Mau RGB565 */
#define ILI9341_BLACK   0x0000
#define ILI9341_BLUE    0x001F
#define ILI9341_RED     0xF800
#define ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_WHITE   0xFFFF

uint16_t ILI9341_ReadID(void);
void ILI9341_Init(void);
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

#endif
