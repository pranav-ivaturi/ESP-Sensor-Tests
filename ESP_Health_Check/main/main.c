#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"

void app_main(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    printf("\n=== ESP HEALTH CHECK ===\n");
    printf("Chip cores: %d\n", chip.cores);
    printf("Chip revision: %d\n", chip.revision);
    printf("Free heap: %lu bytes\n",
           (unsigned long)esp_get_free_heap_size());

    bool heap_ok = heap_caps_check_integrity_all(true);

    printf("Heap integrity: %s\n", heap_ok ? "PASS" : "FAIL");
    printf("Basic health: %s\n", heap_ok ? "PASS" : "FAIL");
    printf("========================\n");

    while (1) {
        printf("[HEARTBEAT] ESP running | Free heap: %lu bytes\n",
               (unsigned long)esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
