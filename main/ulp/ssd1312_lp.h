#ifndef SSD1312_LP_H
#define SSD1312_LP_H

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_WIDTH               128
#define DISPLAY_HEIGHT              64

typedef enum {
    OLED_PUMP_7_5V = 0x12,
    OLED_PUMP_8_0V = 0x52,
    OLED_PUMP_9_0V = 0x72,
    OLED_PUMP_10_0V = 0x92
} oled_pump_volt_t;

void oled_lp_init(void);
void oled_lp_deinit(void);
void oled_lp_power_on(void);
void oled_lp_power_off(void);

void oled_lp_set_inversion(bool invert);
void oled_lp_set_contrast(uint8_t contrast);
void oled_lp_set_pump(oled_pump_volt_t voltage);
void oled_lp_set_rotation_180(bool rotate);
void oled_lp_set_fps(uint8_t fps);

void oled_lp_clear_buffer(void);
void oled_lp_draw_frame(void);
void oled_lp_draw_pixel(uint8_t x, uint8_t y, bool set);

void oled_lp_draw_string(uint8_t x, uint8_t y, const char* str, uint8_t scale, bool flip_180);
void oled_lp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness);
void oled_lp_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t corner_radius, uint8_t thickness, bool fill_solid, bool fill_chess);

void oled_lp_refresh_once(void);
bool oled_lp_refresh_sync(void); //status activity of send i2c on display
uint64_t oled_lp_get_time_us(void);
void oled_lp_time_init(void);

#endif
