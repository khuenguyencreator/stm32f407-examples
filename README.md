# STM32F407 Examples

Code ví dụ các ngoại vi nâng cao trên **STM32F407VET6** (DAC, SDIO, USB Device/Host, I2S) bằng thư viện **HAL**, cấu hình với **STM32CubeMX** và build bằng **Keil MDK-ARM**. Mỗi thư mục tương ứng một bài trên blog [khuenguyencreator.com](https://khuenguyencreator.com).

- MCU: STM32F407VET6
- Cấu hình: STM32CubeMX (file `.ioc` trong mỗi project)
- IDE / Compiler: Keil µVision 5 (MDK-ARM)
- Nạp / debug: ST-Link

## Danh sách bài

| Bài | Nội dung |
|---|---|
| [Bai1a_STM32F407VE_DAC_Voltage_change](Bai1a_STM32F407VE_DAC_Voltage_change) | DAC – xuất điện áp tương tự |
| [Bai1b_STM32F407VE_DAC_DMA_Sinwave](Bai1b_STM32F407VE_DAC_DMA_Sinwave) | DAC + DMA – tạo sóng sin |
| [Bai2_STM32F407VE_SDIO_1_BIT](Bai2_STM32F407VE_SDIO_1_BIT) | Đọc / ghi thẻ SD qua SDIO 1-bit |
| [Bai3_STM32F407VE_USB_DEVICE_MSC](Bai3_STM32F407VE_USB_DEVICE_MSC) | USB Device MSC (thiết bị lưu trữ) |
| [Bai4_STM32F407VE_USB_HOST_HID](Bai4_STM32F407VE_USB_HOST_HID) | USB Host HID (đọc chuột / bàn phím) |
| [Bai5_STM32F407VE_USB_HOST_MSC](Bai5_STM32F407VE_USB_HOST_MSC) | USB Host MSC (đọc USB flash) |
| [Bai6_STM32F407VE_USB_HOST_CDC](Bai6_STM32F407VE_USB_HOST_CDC) | USB Host CDC |
| [Bai7_STM32F407VE_I2S_WAV_PLAYER](Bai7_STM32F407VE_I2S_WAV_PLAYER) | Phát file WAV qua I2S |
| [Bai8_STM32F407VE_USB_MSC_Device](Bai8_STM32F407VE_USB_MSC_Device) | USB MSC Device dùng thẻ SD (SDIO) |

## Cách sử dụng

1. Tải repo:
   ```bash
   git clone https://github.com/khuenguyencreator/stm32f407-examples.git
   ```
2. Mở file `MDK-ARM/*.uvprojx` trong thư mục bài muốn chạy bằng Keil µVision.
3. Build (F7), cắm ST-Link rồi nạp (F8).
4. Riêng `Bai8` chưa có sẵn project Keil: mở file `.ioc` bằng STM32CubeMX, chọn Toolchain / IDE là MDK-ARM rồi Generate Code để tạo project.

## Liên kết

- 📖 Bài viết hướng dẫn chi tiết: [khuenguyencreator.com](https://khuenguyencreator.com)
- 📚 Các repo khác: [github.com/khuenguyencreator](https://github.com/khuenguyencreator)

Nếu thấy hữu ích, hãy bấm ⭐ **Star** để ủng hộ nhé! Có lỗi hoặc thắc mắc, bạn tạo **Issue** trong repo này hoặc để lại bình luận trên blog.
