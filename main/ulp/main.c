#include "ulp_lp_core_utils.h"
#include "ssd1312_lp.h"

// For linker ESP-IDF (do not remove)
uint32_t ulp_lp_perf_counter = 0; 
uint32_t ulp_lp_done_flag = 0;
//This for track RAM usage
uint32_t ulp_lp_memory_usage = 0;
extern uint32_t _bss_end;    

int main(void) {
    oled_lp_init(); //init display
    oled_lp_set_fps(30);  //set FPS 1-30

    while (ulp_lp_done_flag == 0) {
        ulp_lp_memory_usage = (uint32_t)&_bss_end - 0x50000000; //Track RAM
        ulp_lp_perf_counter++; 
        
        oled_lp_clear_buffer(); //clear display

        oled_lp_draw_frame(); //frame 1px                                     
        oled_lp_draw_string(10, 10, "HELLO ULP!", 1, false); //text       
        oled_lp_draw_rect(10, 24, 108, 16, 4, 1, false, true); //rect checkered fill

        oled_lp_refresh_sync(); //start update display
    }
    return 0;
}
