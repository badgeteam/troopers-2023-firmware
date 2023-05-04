#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hardware.h"
#include "pax_gfx.h"

typedef bool (*test_fn)(uint32_t *rc);

bool test_wait_for_response(uint32_t *rc) {
    printf("Waiting for button press...\r\n");

    keyboard_input_message_t button_message = {0};
    Keyboard* keyboard = get_keyboard();

    if (rc != NULL) *rc = 0;
    while (1) {
        if (xQueueReceive(keyboard->queue, &button_message, portMAX_DELAY) == pdTRUE) {
            if (button_message.state) {
                switch (button_message.input) {
                    case BUTTON_START:
                    case BUTTON_SELECT:
                    case BUTTON_BACK:
                        return false;
                    case BUTTON_ACCEPT:
                        if (rc != NULL) *rc = 1;
                        return true;
                    default:
                        break;
                }
            }
        }
    }
}

bool run_test(const pax_font_t *font, int line, const char *test_name, test_fn fn) {
    pax_buf_t *pax_buffer = get_pax_buffer();
    bool       test_result;
    uint32_t   rc;

    printf("Starting test %s...\r\n", test_name);

    /* Test name */
    pax_draw_text(pax_buffer, 0xffffffff, font, 18, 0, 20 * line, test_name);
    display_flush();

    /* Run the test */
    test_result = fn(&rc);

    /* Display result */
    if (!test_result) {
        /* Error */
        char buf[10];
        snprintf(buf, sizeof(buf), "%08x", rc);
        pax_draw_text(pax_buffer, 0xffff0000, font, 18, 200, 20 * line, buf);
    } else {
        /* OK ! */
        pax_draw_text(pax_buffer, 0xff00ff00, font, 18, 200, 20 * line, "      OK");
    }

    display_flush();

    printf("    Test %s result: %s\r\n", test_name, test_result ? "OK" : "FAIL");

    /* Pass through the 'OK' status */
    return test_result;
}
