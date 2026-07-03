#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ulp_lp_core.h"
#include "lp_core_i2c.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void lp_i2c_init(void) {
    lp_core_i2c_cfg_t i2c_cfg = LP_CORE_I2C_DEFAULT_CONFIG();
    i2c_cfg.i2c_timing_cfg.clk_speed_hz = 400000; 

    if (lp_core_i2c_master_init(LP_I2C_NUM_0, &i2c_cfg) != ESP_OK) abort();
}

static void lp_core_init(void) {
    ulp_lp_core_cfg_t cfg = { .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU };

    if (ulp_lp_core_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start)) != ESP_OK) abort();

    ulp_ulp_lp_perf_counter = 0;
    ulp_ulp_lp_done_flag = 0;

    if (ulp_lp_core_run(&cfg) != ESP_OK) abort();
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500));
    printf("HP CPU: Starting subsystems...\n");
    
    lp_i2c_init();
    lp_core_init();

    printf("HP CPU: Boot OK. Monitoring LP Core loop...\n");

    while (1) {
        printf("LP RAM Usage: %lu bytes\n", ulp_ulp_lp_memory_usage);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
