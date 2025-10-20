#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>

// --- Configuration ---
// Define which Pico pins are connected to the WINC module
#define WINC_CHIP_EN_PIN 7
#define WINC_RESET_N_PIN 6
// --- End Configuration ---

int main() {
    // Initialize stdio for printf (optional, but helpful for debugging)
    // You can view this output using a serial monitor connected to the Pico's USB
    stdio_init_all();
    sleep_ms(2000); // Wait 2s for user to connect serial monitor
    
    printf("--- WINC Bootloader Initializer ---\n");

    // Initialize the GPIO pins
    gpio_init(WINC_CHIP_EN_PIN);
    gpio_init(WINC_RESET_N_PIN);

    // Set them as outputs
    gpio_set_dir(WINC_CHIP_EN_PIN, GPIO_OUT);
    gpio_set_dir(WINC_RESET_N_PIN, GPIO_OUT);

    // --- Start the Boot Sequence ---
    
    // 1. Start with both pins LOW
    printf("Step 1: Setting CHIP_EN and RESET_N to LOW.\n");
    gpio_put(WINC_CHIP_EN_PIN, 0);
    gpio_put(WINC_RESET_N_PIN, 0);

    // 2. Wait for power to be stable. 
    //    (The WINC is powered by the Pico's 3.3V, so we just wait)
    sleep_ms(500); 

    // 3. Pull CHIP_EN HIGH
    printf("Step 2: Setting CHIP_EN to HIGH.\n");
    gpio_put(WINC_CHIP_EN_PIN, 1);

    // 4. Wait a moment (100ms is a safe value)
    sleep_ms(100);

    // 5. Pull RESET_N HIGH
    printf("Step 3: Setting RESET_N to HIGH.\n");
    gpio_put(WINC_RESET_N_PIN, 1);

    printf("\nDone! WINC module should be in bootloader mode.\n");
    printf("You can now flash over its UART pins from your PC.\n");

    // Keep the program running indefinitely, holding the pins in this state.
    while (true) {
        tight_loop_contents();
    }

    return 0; // Unreachable
}
