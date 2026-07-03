# LP Core SSD1312 OLED Driver for ESP32-C6 & ESP32-C5

A lightweight, hardware-optimized graphics and text library written from scratch for the **RISC-V LP Core (ULP)** coprocessor inside ESP32-C6 and ESP32-C5 SoC.

This library enables the Low-Power (LP) core to completely offload monochrome OLED display rendering via the hardware `LP_I2C` peripheral. The High-Performance (HP) main core can remain in Deep Sleep or execute independent high-load tasks (such as Wi-Fi 6 or BLE communication) without interrupting the display output.

---

## ⚡ Key Specifications

* **RAM Footprint:** The library standalone footprint is only **2.5 KB to 4 KB** of LP SRAM (including the framebuffer). With full system drivers, ULP bootloader components, and stack allocation, real runtime consumption scales to **~6 KB to 8 KB** out of the 16 KB total available. Completely static allocation.
* **Performance:** Capable of rendering complex scenes (tested with 150 bounding/colliding objects at 100% LP Core load.
* **Auto-Platform Pin Mapping:** Target I2C pins are handled automatically at compile-time based on the selected target framework (GPIO 6/7 for ESP32-C6; GPIO 2/3 for ESP32-C5). No manual pin configuration is required in the codebase.
* **Hardware Time-Tracking:** Implements a precise microsecond timer (`oled_lp_get_time_us`) with hardware clock-tick calibration and overflow protection, compensating for the lack of `esp_timer` on the LP Core.

---

## 🛠 Features & Graphics API

* **Core Control:** Framerate configuration (1–30 FPS), power state toggling (sleep/wake), contrast adjustment, 180° display rotation, and inversion.
* **Geometry Engine:** Standard primitives including pixels, lines (1 to 4 pixels thick), rectangles, frames, circles, and capsules with adjustable corner radius.
* **Advanced Rendering:** Built-in **chess-pattern fill mode** for efficient semi-transparent overlays and shading on monochrome matrices.
* **Text Engine:** Fast 5x7 ASCII font rendering with fast scaling (1x to 4x) and 180° string mirroring.

---

## 📅 Development Context

* **Framework Version:** Developed and fully tested on the official **ESP-IDF `master` branch** (v6.0.DEV state as of **July 1, 2026**). 

---

## 📂 Repository Structure

```text
lp_core_ssd1312/
├── CMakeLists.txt           # Top-level project CMake
├── README.md                # Documentation
├── main/
│   ├── CMakeLists.txt       # Component build script with ULP integration
│   ├── lp_core_ssd1312.c    # Main HP Core initialization application
│   └── ulp/
│       ├── main.c           # Entry point for the LP Core application
│       ├── ssd1312_lp.c     # Library implementation
│       └── ssd1312_lp.h     # Library header
```

---

## 🚀 Integration Guide

To integrate this library into any existing ESP-IDF v6.0+ project, follow these steps:

### 1. Copy Files
Place `ssd1312_lp.c` and `ssd1312_lp.h` directly into your component's `ulp/` directory.

### 2. Update Component CMakeLists.txt
Modify your `main/CMakeLists.txt` to include the `ulp` and `esp_driver_i2c` requirements, and register the source files inside the `ulp_embed_binary` function as shown below:

```cmake
# Set usual component variables
set(app_sources "lp_core_ssd1312.c")

idf_component_register(SRCS \${app_sources}
                       REQUIRES ulp esp_driver_i2c
                       WHOLE_ARCHIVE)

# ULP source registration
set(ulp_app_name ulp_\${COMPONENT_NAME})
set(ulp_sources 
    "ulp/main.c"
    "ulp/ssd1312_lp.c" # <-- Add library source file here
)

set(ulp_exp_dep_srcs \${app_sources})
ulp_embed_binary(\${ulp_app_name} "\${ulp_sources}" "\${ulp_exp_dep_srcs}")
```

### 3. Configure via Menuconfig
Before building, you must explicitly enable the LP Core coprocessor and allocate enough memory for the combined driver and application footprint:

1. Run `idf.py menuconfig`
2. Navigate to: `Component config` ➡️ `Ultra Low Power (ULP) Co-processor`
3. Enable `Enable Ultra Low Power (ULP) Coprocessor`
4. Set `ULP Co-processor type` to **`LP Core RISC-V`**
5. Change `ULP memory size (bytes)` to a **minimum of 8192** (10240 or 12288 is recommended if you plan to add more custom user logic).

### 4. Usage
Include the header file inside your LP Core source code (`ulp/main.c`):
```c
#include "ssd1312_lp.h"
```

---

## 📋 API Reference

###// --- Initialization and System Time ---
*void oled_lp_init(void);             // Initialize display, calibrate timer, start at 24 FPS by default
*void oled_lp_deinit(void);           // Deinitialize: turn off matrix, clear buffer, disable charge pump
*void oled_lp_time_init(void);        // Hardware measurement of LP Core frequency (ticks per 1 µs)
*uint64_t oled_lp_get_time_us(void);  // Time in µs with overflow protection and auto-recalibration
*void oled_lp_set_fps(uint8_t fps);   // Set target frame rate (1..30 FPS)

###// --- Power Management and Matrix Parameters ---
*void oled_lp_power_on(void);         // Quick wake from sleep (turn on matrix)
*void oled_lp_power_off(void);        // Quick sleep mode (turn off matrix for power saving)
*void oled_lp_set_contrast(uint8_t contrast); // Matrix brightness (0x00 .. 0xFF)
*void oled_lp_set_inversion(bool invert);     // Screen color inversion (true = inverted)
*void oled_lp_set_rotation_180(bool rotate);  // Rotate display 180 degrees
*void oled_lp_set_pump(oled_pump_volt_t v);   // Set charge pump voltage (OLED_PUMP_7_5V .. OLED_PUMP_10_0V)

###// --- Frame Buffer and Synchronization ---
*void oled_lp_clear_buffer(void);     // Clear local RAM buffer (1024 bytes) to zero
*bool oled_lp_is_dirty(void);         // Check if the buffer has changed since last transmission
*void oled_lp_refresh_once(void);     // Force transmit buffer to I2C bus regardless of FPS
*bool oled_lp_refresh_sync(void);     // Send frame based on FPS schedule (if buffer is "dirty"). Returns true on send

###// --- Graphics and Text (Drawing to buffer) ---
*void oled_lp_draw_pixel(uint8_t x, uint8_t y, bool set); // Pixel (set: true = on, false = off)
*void oled_lp_draw_frame(void);       // Draw 1-pixel outline border around the screen
*void oled_lp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness); // Line (thickness 1..4)
*void oled_lp_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t t, bool solid, bool chess); 
*// ^ Rectangle (w-width, h-height, r-corner radius, t-border thickness, solid-fill, chess-checkerboard fill)
*void oled_lp_draw_string(uint8_t x, uint8_t y, const char* str, uint8_t scale, bool flip_180); 
*// ^ Text (5x7 ASCII font). scale-multiplier (1..4x), flip_180-mirror string horizontally/vertically

---

## 🛑 Hardware Limitation Notice

This library relies strictly on the hardware `LP_I2C` peripheral architecture implemented in the ESP32-C6 and ESP32-C5 series. Due to architectural differences and specific hardware behavior of the RTC I2C controller on the ESP32-S3 SoC, this codebase is fundamentally incompatible with the ESP32-S3 ULP RISC-V coprocessor and cannot be run on it.

---

## 🔒 License

This project is licensed under the MIT License. You are free to use, modify, and distribute this software in both commercial and open-source applications without restriction. Thanks

---

