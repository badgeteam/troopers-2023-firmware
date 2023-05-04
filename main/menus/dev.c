#include "dev.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>

#include "appfs.h"
#include "button_test.h"
#include "file_browser.h"
#include "hardware.h"
#include "menu.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "sao.h"
#include "settings.h"

extern const uint8_t dev_png_start[] asm("_binary_dev_png_start");
extern const uint8_t dev_png_end[] asm("_binary_dev_png_end");

typedef enum action {
    ACTION_NONE,
    ACTION_BACK,
    ACTION_FILE_BROWSER,
    ACTION_FILE_BROWSER_INT,
    ACTION_BUTTON_TEST,
    ACTION_SAO
} menu_dev_action_t;

static void render_help(pax_buf_t* pax_buffer) {
    const pax_font_t* font = pax_font_saira_regular;
    pax_background(pax_buffer, 0xFFFFFF);
    pax_noclip(pax_buffer);
    pax_draw_text(pax_buffer, 0xFF491d88, font, 18, 5, 240 - 18, "🅰 accept  🅱 back");
}

void menu_dev(xQueueHandle button_queue) {
    pax_buf_t* pax_buffer = get_pax_buffer();
    menu_t*    menu       = menu_alloc("Tools", 34, 18);

    menu->fgColor           = 0xFF000000;
    menu->bgColor           = 0xFFFFFFFF;
    menu->bgTextColor       = 0xFF000000;
    menu->selectedItemColor = 0xFFfec859;
    menu->borderColor       = 0xFFfa448c;
    menu->titleColor        = 0xFFfec859;
    menu->titleBgColor      = 0xFFfa448c;
    menu->scrollbarBgColor  = 0xFFCCCCCC;
    menu->scrollbarFgColor  = 0xFF555555;

    pax_buf_t icon_dev;
    pax_decode_png_buf(&icon_dev, (void*) dev_png_start, dev_png_end - dev_png_start, PAX_BUF_32_8888ARGB, 0);

    menu_set_icon(menu, &icon_dev);

    menu_insert_item(menu, "File browser (SD card)", NULL, (void*) ACTION_FILE_BROWSER, -1);
    menu_insert_item(menu, "File browser (internal)", NULL, (void*) ACTION_FILE_BROWSER_INT, -1);
    menu_insert_item(menu, "Button test", NULL, (void*) ACTION_BUTTON_TEST, -1);
    menu_insert_item(menu, "SAO EEPROM tool", NULL, (void*) ACTION_SAO, -1);

    bool              render = true;
    menu_dev_action_t action = ACTION_NONE;

    render_help(pax_buffer);

    while (1) {
        keyboard_input_message_t buttonMessage = {0};
        if (xQueueReceive(button_queue, &buttonMessage, 16 / portTICK_PERIOD_MS) == pdTRUE) {
            uint8_t pin   = buttonMessage.input;
            bool    value = buttonMessage.state;
            switch (pin) {
                case JOYSTICK_DOWN:
                    if (value) {
                        menu_navigate_next(menu);
                        render = true;
                    }
                    break;
                case JOYSTICK_UP:
                    if (value) {
                        menu_navigate_previous(menu);
                        render = true;
                    }
                    break;
                case BUTTON_BACK:
                    if (value) {
                        action = ACTION_BACK;
                    }
                    break;
                case BUTTON_ACCEPT:
                case BUTTON_SELECT:
                case BUTTON_START:
                    if (value) {
                        action = (menu_dev_action_t) menu_get_callback_args(menu, menu_get_position(menu));
                    }
                    break;
                default:
                    break;
            }
        }

        if (render) {
            menu_render(pax_buffer, menu, 0, 0, 320, 220);
            display_flush();
            render = false;
        }

        if (action != ACTION_NONE) {
            if (action == ACTION_FILE_BROWSER) {
                file_browser(button_queue, "/sd");
            } else if (action == ACTION_FILE_BROWSER_INT) {
                file_browser(button_queue, "/internal");
            } else if (action == ACTION_BUTTON_TEST) {
                test_buttons(button_queue);
            } else if (action == ACTION_SAO) {
                menu_sao(button_queue);
            } else if (action == ACTION_BACK) {
                break;
            }
            action = ACTION_NONE;
            render = true;
            render_help(pax_buffer);
        }
    }

    menu_free(menu);

    pax_buf_destroy(&icon_dev);
}
