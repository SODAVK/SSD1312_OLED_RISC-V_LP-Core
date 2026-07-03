//LP RAM Usage: 6216 bytes (6-8kbytes) 8.350байт в примере с 150 обьектами
#include "ssd1312_lp.h"
#include "ulp_lp_core_i2c.h"
#include "ulp_lp_core_utils.h"

#define OLED_I2C_ADDRESS            0x3C //Can be change on 0x3D if display cant ON
#define LP_I2C_TRANS_TIMEOUT_CYCLES 200000 //10ms

static uint8_t oled_buffer[1024];
static bool screen_is_dirty = false;

static uint32_t target_frame_interval_us = 41666; //24fps by default
static uint64_t last_frame_time_us = 0;

static uint32_t cycles_per_us = 20;
static bool time_is_initialized = false;
static uint32_t last_hardware_cycles = 0;
static uint64_t extended_us_offset = 0;
static uint16_t calibration_countdown = 0;

//Gliphs Caps Latinian
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 'Space' (0)
    {0x7E, 0x90, 0x90, 0x90, 0x7E}, // 'A' (1)
    {0xFE, 0x92, 0x92, 0x92, 0x6C}, // 'B' (2)  
    {0x7C, 0x82, 0x82, 0x82, 0x44}, // 'C' (3)  
    {0xFE, 0x82, 0x82, 0x82, 0x7C}, // 'D' (4)
    {0xFE, 0x92, 0x92, 0x92, 0x82}, // 'E' (5)  
    {0xFE, 0x90, 0x90, 0x90, 0x80}, // 'F' (6)  
    {0x7C, 0x82, 0x8A, 0x8A, 0x4E}, // 'G' (7)
    {0xFE, 0x10, 0x10, 0x10, 0xFE}, // 'H' (8)  
    {0x82, 0x82, 0xFE, 0x82, 0x82}, // 'I' (9)  
    {0x04, 0x02, 0x82, 0xFC, 0x80}, // 'J' (10)
    {0xFE, 0x10, 0x28, 0x44, 0x82}, // 'K' (11) 
    {0xFE, 0x02, 0x02, 0x02, 0x02}, // 'L' (12) 
    {0xFE, 0x40, 0x20, 0x40, 0xFE}, // 'M' (13) 
    {0xFE, 0x20, 0x10, 0x08, 0xFE}, // 'N' (14)
    {0x7C, 0x82, 0x82, 0x82, 0x7C}, // 'O' (15) 
    {0xFE, 0x90, 0x90, 0x90, 0x60}, // 'P' (16) 
    {0x7C, 0x82, 0x8A, 0x84, 0x7A}, // 'Q' (17)
    {0xFE, 0x90, 0x98, 0x94, 0x62}, // 'R' (18) 
    {0x62, 0x92, 0x92, 0x92, 0x8C}, // 'S' (19) 
    {0x80, 0x80, 0xFE, 0x80, 0x80}, // 'T' (20) 
    {0xFC, 0x02, 0x02, 0x02, 0xFC}, // 'U' (21) 
    {0xF8, 0x04, 0x02, 0x04, 0xF8}, // 'V' (22) 
    {0xFC, 0x02, 0x3C, 0x02, 0xFC}, // 'W' (23)
    {0xC6, 0x28, 0x10, 0x28, 0xC6}, // 'X' (24)
    {0xE0, 0x10, 0x0E, 0x10, 0xE0}, // 'Y' (25)
    {0x86, 0x8A, 0x92, 0xA2, 0xC2}, // 'Z' (26)
    {0x7C, 0x8A, 0x92, 0xA2, 0x7C}, // '0' (27) 
    {0x00, 0x42, 0xFE, 0x02, 0x00}, // '1' (28)
    {0x46, 0x8A, 0x92, 0xA2, 0x62}, // '2' (29) 
    {0x44, 0x92, 0x92, 0x92, 0x6C}, // '3' (30)
    {0x18, 0x28, 0x48, 0xFE, 0x08}, // '4' (31) 
    {0xE4, 0xA2, 0xA2, 0xA2, 0x9C}, // '5' (32)
    {0x3C, 0x52, 0x92, 0x92, 0x0C}, // '6' (33)
    {0x80, 0x8E, 0x90, 0xA0, 0xC0}, // '7' (34)
    {0x6C, 0x92, 0x92, 0x92, 0x6C}, // '8' (35)
    {0x60, 0x92, 0x92, 0x94, 0x78}, // '9' (36)
    
    {0x1C, 0x22, 0x22, 0x14, 0x3E}, // 'a' (37)
    {0xFE, 0x12, 0x22, 0x22, 0x1C}, // 'b' (38)
    {0x1C, 0x22, 0x22, 0x22, 0x22}, // 'c' (39)
    {0x1C, 0x22, 0x22, 0x12, 0xFE}, // 'd' (40)
    {0x1C, 0x2A, 0x2A, 0x2A, 0x18}, // 'e' (41)
    {0x10, 0x7E, 0x90, 0x90, 0x40}, // 'f' (42)
    {0x30, 0x4A, 0x4A, 0x4A, 0x7C}, // 'g' (43)
    {0xFE, 0x10, 0x20, 0x20, 0x1E}, // 'h' (44)
    {0x00, 0x22, 0xBE, 0x02, 0x00}, // 'i' (45)
    {0x02, 0x01, 0x21, 0xBE, 0x00}, // 'j' (46)
    {0xFE, 0x08, 0x14, 0x22, 0x00}, // 'k' (47)
    {0x00, 0x82, 0xFE, 0x02, 0x00}, // 'l' (48)
    {0x3E, 0x20, 0x18, 0x20, 0x1E}, // 'm' (49)
    {0x3E, 0x10, 0x20, 0x20, 0x1E}, // 'n' (50)
    {0x1C, 0x22, 0x22, 0x22, 0x1C}, // 'o' (51)
    {0x7C, 0x12, 0x22, 0x22, 0x1C}, // 'p' (52)
    {0x1C, 0x22, 0x22, 0x12, 0x7C}, // 'q' (53)
    {0x3E, 0x10, 0x20, 0x20, 0x10}, // 'r' (54)
    {0x12, 0x2A, 0x2A, 0x2A, 0x24}, // 's' (55)
    {0x20, 0x7C, 0x22, 0x22, 0x04}, // 't' (56)
    {0x3C, 0x02, 0x02, 0x04, 0x3E}, // 'u' (57)
    {0x38, 0x04, 0x02, 0x04, 0x38}, // 'v' (58)
    {0x3C, 0x02, 0x0C, 0x02, 0x3C}, // 'w' (59)
    {0x22, 0x14, 0x08, 0x14, 0x22}, // 'x' (60)
    {0x38, 0x05, 0x05, 0x05, 0x3E}, // 'y' (61)
    {0x22, 0x26, 0x2A, 0x32, 0x22}, // 'z' (62)

    {0x02, 0x00, 0x00, 0x00, 0x00}, // '.' (63)
    {0x00, 0x01, 0x03, 0x00, 0x00}, // ',' (64)
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '/' (65)
    {0x00, 0x24, 0x00, 0x00, 0x00}, // ':' (66)
    {0x3C, 0x42, 0x81, 0x00, 0x00}, // '(' (67)
    {0x00, 0x00, 0x81, 0x42, 0x3C}, // ')' (68)
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*' (69)
    {0x08, 0x10, 0x20, 0x10, 0x08}, // '^' (70)
    {0x46, 0x26, 0x10, 0x64, 0x62}, // '%' (71)
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#' (72)
    {0x00, 0x00, 0xFA, 0x00, 0x00}, // '!' (73)
    {0x40, 0x80, 0x8A, 0x90, 0x60}, // '?' (74)
    {0x00, 0xFF, 0x81, 0x81, 0x00}, // '[' (75)
    {0x00, 0x81, 0x81, 0xFF, 0x00}, // ']' (76)
    {0x10, 0x28, 0x44, 0x82, 0x00}, // '<' (77)
    {0x00, 0x82, 0x44, 0x28, 0x10}, // '>' (78)
    {0x10, 0x10, 0x10, 0x10, 0x10}, // '-' (79)
    {0x01, 0x01, 0x01, 0x01, 0x01}, // '_' (80)
    {0x10, 0x10, 0x7C, 0x10, 0x10}, // '+' (81)
    {0x28, 0x28, 0x28, 0x28, 0x28}  // '=' (82)
};

static uint8_t get_font_index(char c) {
    if (c == ' ')  return 0;
    if (c >= 'A' && c <= 'Z') return (c - 'A' + 1);
    if (c >= '0' && c <= '9') return (c - '0' + 27);
    if (c >= 'a' && c <= 'z') return (c - 'a' + 37);
    
    switch(c) {
        case '.': return 63; case ',': return 64; case '/': return 65;
        case ':': return 66; case '(': return 67; case ')': return 68; 
        case '*': return 69; case '^': return 70; case '%': return 71; 
        case '#': return 72; case '!': return 73; case '?': return 74; 
        case '[': return 75; case ']': return 76; case '<': return 77; 
        case '>': return 78; case '-': return 79; case '_': return 80; 
        case '+': return 81; case '=': return 82;
        default:  return 74; 
    }
}

static void oled_send_command(uint8_t command) {
    uint8_t buf[2] = {0x00, command}; 
    lp_core_i2c_master_write_to_device(LP_I2C_NUM_0, OLED_I2C_ADDRESS, buf, 2, LP_I2C_TRANS_TIMEOUT_CYCLES);
}

void oled_lp_time_init(void) {
    uint64_t start = ulp_lp_core_get_cpu_cycles();
    ulp_lp_core_delay_us(1000); 
    uint64_t end = ulp_lp_core_get_cpu_cycles();
    
    uint32_t diff = (uint32_t)(end - start);
    cycles_per_us = diff / 1000;
    
    if (cycles_per_us < 1) cycles_per_us = 20; 
    time_is_initialized = true;
    
    last_hardware_cycles = (uint32_t)ulp_lp_core_get_cpu_cycles();
}

uint64_t oled_lp_get_time_us(void) {
    if (!time_is_initialized) {
        oled_lp_time_init();
    }

    uint32_t current_hardware_cycles = (uint32_t)ulp_lp_core_get_cpu_cycles();
    
    if (current_hardware_cycles < last_hardware_cycles) {
        uint32_t ticks_passed = (0xFFFFFFFF - last_hardware_cycles) + current_hardware_cycles + 1;
        extended_us_offset += (ticks_passed / cycles_per_us);
    } else {
        uint32_t ticks_passed = current_hardware_cycles - last_hardware_cycles;
        extended_us_offset += (ticks_passed / cycles_per_us);
    }
    last_hardware_cycles = current_hardware_cycles;


    calibration_countdown++;
    if (calibration_countdown >= 5000) {
        calibration_countdown = 0;
        uint64_t start_cal = ulp_lp_core_get_cpu_cycles();
        ulp_lp_core_delay_us(500);
        uint64_t end_cal = ulp_lp_core_get_cpu_cycles();
        uint32_t cal_diff = (uint32_t)(end_cal - start_cal);
        if (cal_diff > 0) {
            cycles_per_us = cal_diff / 500;
            if (cycles_per_us < 1) cycles_per_us = 20;
        }
        last_hardware_cycles = (uint32_t)ulp_lp_core_get_cpu_cycles();
    }
    return extended_us_offset;
}

void oled_lp_clear_buffer(void) {
    for (uint16_t i = 0; i < 1024; i++) oled_buffer[i] = 0x00;
    screen_is_dirty = true;
}

void oled_lp_draw_frame(void) {
    for (uint8_t col = 0; col < 128; col++) {
        oled_buffer[0 * 128 + col] |= 0x01; 
        oled_buffer[7 * 128 + col] |= 0x80; 
    }
    for (uint8_t page = 0; page < 8; page++) {
        oled_buffer[page * 128 + 0] = 0xFF;   
        oled_buffer[page * 128 + 127] = 0xFF; 
    }
    screen_is_dirty = true;
}

void oled_lp_draw_pixel(uint8_t x, uint8_t y, bool set) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    uint16_t index = (page * DISPLAY_WIDTH) + x;
    
    uint8_t old_val = oled_buffer[index];
    if (set) oled_buffer[index] |= (1 << bit);
    else oled_buffer[index] &= ~(1 << bit);
    
    if (old_val != oled_buffer[index]) {
        screen_is_dirty = true;
    }
}

void oled_lp_draw_string(uint8_t x, uint8_t y, const char* str, uint8_t scale, bool flip_180) {
    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;

    uint8_t char_width = 6 * scale; 
    int len = 0;
    while (str[len]) len++;
    
    if (flip_180) {
        x = x + (len * char_width) - char_width;
    }

    for (int i = 0; i < len; i++) {
        char current_char = str[i]; 
        uint8_t idx = get_font_index(current_char);
        
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t target_col = flip_180 ? (4 - col) : col;
            uint8_t line = font_5x7[idx][target_col];
            
            for (uint8_t row = 0; row < 8; row++) {
                uint8_t target_row = flip_180 ? (7 - row) : row;
                
                if (line & (1 << target_row)) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        for (uint8_t sy = 0; sy < scale; sy++) {
                            int16_t pixel_x, pixel_y;
                            
                            if (flip_180) {
                                pixel_x = x + (col * scale) + (scale - 1 - sx);
                                pixel_y = y + (row * scale) + (scale - 1 - sy);
                            } else {
                                pixel_x = x + (col * scale) + sx;
                                pixel_y = y + (row * scale) + sy;
                            }
                            
                            oled_lp_draw_pixel((uint8_t)pixel_x, (uint8_t)pixel_y, true);
                        }
                    }
                }
            }
        }
        if (flip_180) x -= char_width;
        else x += char_width;
    }
}

void oled_lp_set_inversion(bool invert) { oled_send_command(invert ? 0xA7 : 0xA6); }
void oled_lp_set_contrast(uint8_t contrast) { oled_send_command(0x81); oled_send_command(contrast); }
void oled_lp_set_pump(oled_pump_volt_t voltage) { oled_send_command(0x8D); oled_send_command((uint8_t)voltage); }

void oled_lp_set_rotation_180(bool rotate) {
    if (rotate) {
        oled_send_command(0xA0); 
        oled_send_command(0xC0); 
    } else {
        oled_send_command(0xA1); 
        oled_send_command(0xC8); 
    }
}

void oled_lp_set_fps(uint8_t fps) {
    if (fps < 1) fps = 1; 
    if (fps > 30) fps = 30;
    target_frame_interval_us = 1000000 / fps; 
}

void oled_lp_power_off(void) {
    oled_send_command(0xAE); oled_send_command(0x8D); oled_send_command(0x02);
    ulp_lp_core_delay_us(20000); 
}

void oled_lp_power_on(void) {
    oled_send_command(0x8D); oled_send_command(0x14);
    ulp_lp_core_delay_us(20000); oled_send_command(0xAF); 
}

void oled_lp_init(void) {
    oled_lp_time_init();
    oled_send_command(0xE3); ulp_lp_core_delay_us(20000); 
    oled_send_command(0xAE); 
    oled_send_command(0xD5); oled_send_command(0x80); 
    oled_send_command(0xA8); oled_send_command(0x3F); 
    oled_send_command(0x40); 
    oled_send_command(0x8D); oled_send_command(0x14); 
    oled_send_command(0x20); oled_send_command(0x02); 
    oled_send_command(0xA1); 
    oled_send_command(0xC8); 
    oled_send_command(0xDA); oled_send_command(0x12); 
    oled_send_command(0xD3); oled_send_command(0x00); 
    oled_send_command(0x81); oled_send_command(0xFF); 
    oled_send_command(0xD9); oled_send_command(0xF1); 
    oled_send_command(0xDB); oled_send_command(0x40); 
    oled_send_command(0x2E); oled_send_command(0xA4); 
    oled_send_command(0xA6); oled_send_command(0xAC); 
    ulp_lp_core_delay_us(50000); oled_send_command(0xAF); 
    last_frame_time_us = oled_lp_get_time_us();
}

void oled_lp_deinit(void) { oled_lp_power_off(); oled_lp_clear_buffer(); }

bool oled_lp_is_dirty(void) {
    return screen_is_dirty;
}

void oled_lp_refresh_once(void) {
    if (!screen_is_dirty) return;

    uint8_t tx_buf[129]; tx_buf[0] = 0x40; 
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_command(0xB0 + page); oled_send_command(0x00); oled_send_command(0x10); 
        uint16_t page_offset = page * DISPLAY_WIDTH;
        for (uint16_t i = 0; i < 128; i++) tx_buf[i + 1] = oled_buffer[page_offset + i];
        lp_core_i2c_master_write_to_device(LP_I2C_NUM_0, OLED_I2C_ADDRESS, tx_buf, 129, LP_I2C_TRANS_TIMEOUT_CYCLES);
    }  
    screen_is_dirty = false; 
}

bool oled_lp_refresh_sync(void) {
    uint64_t now = oled_lp_get_time_us();

    if (now - last_frame_time_us >= target_frame_interval_us) {
        if (screen_is_dirty) {
            oled_lp_refresh_once();
            last_frame_time_us = now;
            return true; //I2C send, LP core work
        }
    }
    return false; // I2C free, LP core free
}

static void oled_lp_draw_fat_pixel(int16_t x, int16_t y, uint8_t thickness) {
    if (thickness <= 1) {
        oled_lp_draw_pixel((uint8_t)x, (uint8_t)y, true);
        return;
    }
    int16_t r = thickness / 2;
    for (int16_t dx = -r; dx <= r; dx++) {
        for (int16_t dy = -r; dy <= r; dy++) {
            oled_lp_draw_pixel((uint8_t)(x + dx), (uint8_t)(y + dy), true);
        }
    }
}

void oled_lp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness) {
    if (thickness < 1) thickness = 1;
    if (thickness > 4) thickness = 4;

    if (y0 == y1) {
        int16_t start_x = (x0 < x1) ? x0 : x1;
        int16_t end_x = (x0 < x1) ? x1 : x0;
        for (int16_t x = start_x; x <= end_x; x++) oled_lp_draw_fat_pixel(x, y0, thickness);
        return;
    }
    if (x0 == x1) {
        int16_t start_y = (y0 < y1) ? y0 : y1;
        int16_t end_y = (y0 < y1) ? y1 : y0;
        for (int16_t y = start_y; y <= end_y; y++) oled_lp_draw_fat_pixel(x0, y, thickness);
        return;
    }

    int16_t dx = (x1 - x0 >= 0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 - y0 >= 0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    while (1) {
        oled_lp_draw_fat_pixel(x0, y0, thickness);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

static uint32_t ulp_sqrt(uint32_t value) {
    uint32_t res = 0;
    uint32_t bit = 1 << 14; // start 2^16
    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (value >= res + bit) {
            value -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

void oled_lp_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t corner_radius, uint8_t thickness, bool fill_solid, bool fill_chess) {
    if (width == 0 || height == 0) return;
    if (thickness < 1) thickness = 1;

    uint8_t max_r = (width < height) ? (width / 2) : (height / 2);
    if (corner_radius > max_r) {
        corner_radius = max_r;
    }

    uint8_t x1 = x + width - 1;
    uint8_t y1 = y + height - 1;
    uint32_t r_sq = (uint32_t)corner_radius * corner_radius;
    for (uint8_t curr_y = y; curr_y <= y1; curr_y++) {
        uint8_t x_offset = 0;

        if (corner_radius > 0) {
            int16_t dy = 0;
            if (curr_y < y + corner_radius) {
                dy = (y + corner_radius) - curr_y;
            } else if (curr_y > y1 - corner_radius) {
                dy = curr_y - (y1 - corner_radius);
            }

            if (dy > 0) {
            
                uint32_t corrected_dy = ((uint32_t)dy * 11) / 10; 
                uint32_t dy_sq = corrected_dy * corrected_dy;
    
                if (r_sq >= dy_sq) {
                    x_offset = corner_radius - (uint8_t)ulp_sqrt(r_sq - dy_sq + 1);
                } else {
                    x_offset = corner_radius;
                }
            }   
        }

        uint8_t line_x_start = x + x_offset;
        uint8_t line_x_end   = x1 - x_offset;

        if (line_x_start > line_x_end && x_offset > corner_radius) continue;
        if (line_x_start > x1 || line_x_end < x) continue; 
        if (line_x_start > line_x_end) {
            line_x_start = x + corner_radius;
            line_x_end = line_x_start;
        }

        bool is_horizontal_edge = (curr_y < y + thickness) || (curr_y > y1 - thickness);
        for (uint8_t curr_x = line_x_start; curr_x <= line_x_end; curr_x++) {
            
            bool is_in_thickness = is_horizontal_edge || 
                                   (curr_x < line_x_start + thickness) || 
                                   (curr_x > line_x_end - thickness);

            if (is_in_thickness) {
                oled_lp_draw_pixel(curr_x, curr_y, true);
            } else {
                if (fill_solid) {
                    oled_lp_draw_pixel(curr_x, curr_y, true);
                } else if (fill_chess) {
                    if ((curr_x + curr_y) % 2 == 0) {
                        oled_lp_draw_pixel(curr_x, curr_y, true);
                    }
                }
            }
        }
    }
}
