#include "ergohaven.h"
#include "info_config.h"
#include "ergohaven_ruen.h"
#include "hid.h"
#include "transactions.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <timer.h>

#include "spaceship.c"

typedef union {
    uint32_t raw;
    struct {
        uint8_t oled_slave : 3;
        uint8_t oled_master : 3;
        bool    right_encoder : 1;
        bool    left_encoder : 1;
        uint8_t lang : 1;
        bool    mac : 1;
        bool    caps_word : 1;
    };
} vial_config_t;

vial_config_t vial_config;

typedef enum {
    OLED_STATUS_CLASSIC = 0,
    OLED_STATUS_MODERN,
    OLED_STATUS_DARKSIDE,
    OLED_SPACESHIP,
    OLED_DISABLED,
} oled_mode_t;

oled_mode_t get_oled_mode_on_half(bool on_master) {
    if (on_master) return vial_config.oled_master;

    // first two modes swapped for slave
    if (vial_config.oled_slave == OLED_STATUS_DARKSIDE) return OLED_SPACESHIP;
    if (vial_config.oled_slave == OLED_SPACESHIP) return OLED_STATUS_DARKSIDE;

    return vial_config.oled_slave;
}

oled_mode_t get_oled_mode(void) {
    return get_oled_mode_on_half(is_keyboard_master());
}

uint8_t split_get_lang(void) {
    return is_keyboard_master() ? get_cur_lang() : vial_config.lang;
}

bool split_get_mac(void) {
    return is_keyboard_master() ? keymap_config.swap_lctl_lgui : vial_config.mac;
}

bool split_get_caps_word(void) {
    return is_keyboard_master() ? is_caps_word_on() : vial_config.caps_word;
}

oled_rotation_t get_desired_oled_rotation(void) {
    int mode = get_oled_mode();
    switch (mode) {
        case OLED_SPACESHIP:
            return is_keyboard_left() ? OLED_ROTATION_0 : OLED_ROTATION_180;
            break;
        default:
            return OLED_ROTATION_270;
    }
}

static oled_rotation_t current_oled_rotation;

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    current_oled_rotation = get_desired_oled_rotation();
    return current_oled_rotation;
}

void render_status_classic(void) {

    // Print current layer
    oled_set_cursor(0, 12);
    oled_write_P(PSTR(layer_name(get_current_layer())), false);

    oled_set_cursor(0, 15);
    bool caps = host_keyboard_led_state().caps_lock || split_get_caps_word();
    oled_write_P(PSTR("CPSLK"), caps);
}

//void render_space(void) {
    //oled_write_P(PSTR("     "), false);
//}

//
void render_name(void) {
     oled_write_P(EH_SHORT_PRODUCT_NAME, false);
}

void render_version(void) {
    oled_write_P(PSTR(EH_VERSION_STR), false);
}

// Renders the logo and optionally WPM
void render_diamond_logo(void) {
    static const uint8_t apple_logo[] = {0x20, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    oled_set_cursor(0, 2);
    oled_write_char(apple_logo[0], false);
    oled_write_char(apple_logo[1], false);
    oled_write_char(apple_logo[2], false);
    oled_write_char(apple_logo[3], false);
    oled_write_char(apple_logo[0], false);
    oled_set_cursor(0, 3);
    oled_write_char(apple_logo[0], false);
    oled_write_char(apple_logo[4], false);
    oled_write_char(apple_logo[5], false);
    oled_write_char(apple_logo[6], false);
    oled_write_char(apple_logo[0], false);

}

// ergohaven_oled.c
//#include "quantum.h"
//#include <stdio.h>

// --- Bitmaps ---
static const char PROGMEM icons[][2][3] = {
    // [0] = off, [1] = on
    // GUI/ALT/CTRL/SHIFT first row
    {{0x80, 0x81, 0}, {0x88, 0x89, 0}}, // GUI
    {{0x82, 0x83, 0}, {0x8a, 0x8b, 0}}, // ALT
    {{0x84, 0x85, 0}, {0x8c, 0x8d, 0}}, // CTRL
    {{0x86, 0x87, 0}, {0x8e, 0x8f, 0}}, // SHIFT
    // GUI/ALT/CTRL/SHIFT second row
    {{0xa0, 0xa1, 0}, {0xa8, 0xa9, 0}}, // GUI
    {{0xa2, 0xa3, 0}, {0xaa, 0xab, 0}}, // ALT
    {{0xa4, 0xa5, 0}, {0xac, 0xad, 0}}, // CTRL
    {{0xa6, 0xa7, 0}, {0xae, 0xaf, 0}}, // SHIFT
};

static const char PROGMEM icons_layer[][2][3] = {
    // [0] = off, [1] = on
    // GUI/ALT/CTRL/SHIFT first row
    {{0x90, 0x91, 0}, {0x98, 0x99, 0}}, // GUI
    {{0x92, 0x93, 0}, {0x9a, 0x9b, 0}}, // ALT
    {{0x94, 0x95, 0}, {0x9c, 0x9d, 0}}, // CTRL
    {{0x96, 0x97, 0}, {0x9e, 0x9f, 0}}, // SHIFT
    // GUI/ALT/CTRL/SHIFT second row
    {{0xb0, 0xb1, 0}, {0xb8, 0xb9, 0}}, // GUI
    {{0xb2, 0xb3, 0}, {0xba, 0xbb, 0}}, // ALT
    {{0xb4, 0xb5, 0}, {0xbc, 0xbd, 0}}, // CTRL
    {{0xb6, 0xb7, 0}, {0xbe, 0xbf, 0}}, // SHIFT
};

static const char PROGMEM fillers[][2] = {
    {0xc0, 0}, {0xc2, 0}, {0xc4, 0}, {0xc6, 0}, // first row: off_off, on_off, off_on, on_on
    {0xc1, 0}, {0xc3, 0}, {0xc5, 0}, {0xc7, 0}, // second row: off_off_2, on_off_2, off_on_2, on_on_2
};

// --- Helpers ---
static inline void render_space(void) {
    oled_write_P(PSTR("     "), false);
}

// Renders a pair of modifier icons with a filler between them
static void render_mod_pair(uint8_t mods, uint8_t left_mask, uint8_t right_mask, uint8_t icon_idx, uint8_t filler_base) {
    bool left = mods & left_mask;
    bool right = mods & right_mask;
    oled_write_P(icons[icon_idx][left], false);
    // Filler index: 0=off_off, 1=on_off, 2=off_on, 3=on_on
    uint8_t filler_idx = filler_base + (left ? (right ? 3 : 1) : (right ? 2 : 0));
    oled_write_P(&fillers[filler_idx][0], false);
    oled_write_P(icons[icon_idx + 1][right], false);
}

// --- Main status render ---
void render_status_darkside(bool is_master) {
    render_name();
    render_space();
    render_diamond_logo();
    render_space();

    uint8_t mods = get_mods() | get_oneshot_mods();

    // GUI/ALT first row
    render_mod_pair(mods, MOD_MASK_GUI, MOD_MASK_ALT, 0, 0);
    // GUI/ALT second row
    render_mod_pair(mods, MOD_MASK_GUI, MOD_MASK_ALT, 4, 4);
    // CTRL/SHIFT first row
    render_mod_pair(mods, MOD_MASK_CTRL, MOD_MASK_SHIFT, 2, 0);
    // CTRL/SHIFT second row
    render_mod_pair(mods, MOD_MASK_CTRL, MOD_MASK_SHIFT, 6, 4);

    oled_set_cursor(0, 11);
    oled_write_P(PSTR("\x06\x07\x08\x09\x0f"), false);
    oled_write_P(PSTR("\x0b\x0c\x0d\x0e\x0f"), false);
    oled_write_P(PSTR("\x20\x20\x20\x20\x10"), false);

    oled_set_cursor(0, 14);
    static uint32_t start_time = 0;
    if (start_time == 0) start_time = timer_read32();
    uint32_t elapsed = (timer_read32() - start_time) / 1000;
    char buf[6];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", elapsed / 3600, (elapsed / 60) % 60);
    oled_write_ln(buf, false);
}

void render_status_modern(void) {
    oled_clear();
    render_version();
    render_space();
    render_diamond_logo();

    oled_set_cursor(0, 5);
    //led_t led_usb_state = host_keyboard_led_state();
    //bool  caps          = led_usb_state.caps_lock || split_get_caps_word();

    // oled_write_P(
    //     caps
    //         ? PSTR("\x9c\x90\x91\x92\x93")
    //         : PSTR("\x9c\x94\x95\x96\x97"), false);

        int layer = get_current_layer();
        // Определяем, какая пиктограмма активна
        uint8_t active = 0xFF;
        if (layer == 0)      active = 0; // GUI
        else if (layer == 1) active = 1; // ALT
        else if (layer == 2) active = 2; // CTRL
        else if (layer == 4) active = 3; // SHIFT

        // First row: GUI and ALT (icons 0,1; fillers 0–3)
        // Set cursor to start (0,0) for first row
        oled_set_cursor(0, 5);
            for (uint8_t i = 0; i < 2; ++i) {
                bool is_on = (active == i);
                oled_write_P(icons_layer[i][is_on], false);
                if (i == 0) {
                    static const char PROGMEM filler_top[] = {0xc0, 0};
                    oled_write_P(filler_top, false);
                }
            }

            // Second row: GUI and ALT (icons 4,5; fillers 4–7)
            for (uint8_t i = 4; i < 6; ++i) {
                bool is_on = (active == (i - 4));
                oled_write_P(icons_layer[i][is_on], false);
                if (i == 4) {
                    static const char PROGMEM filler_top[] = {0xc1, 0};
                    oled_write_P(filler_top, false);
                }
            }

            // First row: CTRL and SHIFT (icons 2,3; fillers 0–3)
            for (uint8_t i = 2; i < 4; ++i) {
                bool is_on = (active == i);
                oled_write_P(icons_layer[i][is_on], false);
                if (i == 2) {
                    static const char PROGMEM filler_top[] = {0xc0, 0};
                    oled_write_P(filler_top, false);
                }
            }

            // Second row: CTRL and SHIFT (icons 6,7; fillers 4–7)
            for (uint8_t i = 6; i < 8; ++i) {
                bool is_on = (active == (i - 4));
                oled_write_P(icons_layer[i][is_on], false);
                if (i == 6) {
                    static const char PROGMEM filler_top[] = {0xc1, 0};
                    oled_write_P(filler_top, false);
                }
            }

            // Third line: nan or layer number
            char buf[8];
            if (layer > 4) {
                snprintf(buf, sizeof(buf), "%d", layer);
                oled_write_ln(buf, false);
            } else {
                oled_write_ln(" ", false);
            }
            oled_write_P(PSTR("\xc8\xc9\xca\xcb\xcc"), false);
            oled_write_P(PSTR("     "), false);
            oled_write_P(PSTR("\xd2\xd3\xd4\xd5\xd6"), false);
}

void render_big_num(int num, char* c0, char* c1, char* c2, char* c3) {
    *c0 = 0x80 + num * 2;
    *c1 = 0x81 + num * 2;
    *c2 = 0xa0 + num * 2;
    *c3 = 0xa1 + num * 2;
}

static uint32_t last_layout_options_time = 0;

void via_set_layout_options_kb(uint32_t value) {
    if (vial_config.raw == value) return;
    vial_config.raw          = value;
    last_layout_options_time = sync_timer_read32();
}

bool oled_task_kb(void) {
    // Defer to the keymap if they want to override
    if (!oled_task_user()) {
        return false;
    }

    uint32_t activity_elapsed = MIN(last_input_activity_elapsed(), //
                                    sync_timer_elapsed32(last_layout_options_time));

    if (activity_elapsed > EH_TIMEOUT || get_oled_mode() == OLED_DISABLED) {
        oled_off();
        return false;
    } else {
        oled_on();
    }

    if (get_desired_oled_rotation() != current_oled_rotation) oled_init(get_desired_oled_rotation());

    uint8_t mode = get_oled_mode();
    switch (mode) {
        case OLED_STATUS_CLASSIC:
            render_status_classic();
            break;

        case OLED_STATUS_MODERN:
            render_status_modern();
            break;

        case OLED_STATUS_DARKSIDE:
                  render_status_darkside(is_keyboard_master());
                  break;

        case OLED_SPACESHIP:
            render_spaceship();
            break;

        case OLED_DISABLED:
        default:
            oled_clear();
            break;
    }

    return false;
}

void sync_config(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
    uint32_t value;
    memcpy(&value, in_data, sizeof(uint32_t));
    via_set_layout_options_kb(value);
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(RPC_SYNC_CONFIG, sync_config);
}

void housekeeping_task_split_oled(void) {
    if (is_keyboard_master()) {
        // Interact with slave every 500ms
        static uint32_t last_sync = 0;
        if (timer_elapsed32(last_sync) > 500) {
            vial_config.lang      = split_get_lang();
            vial_config.mac       = split_get_mac();
            vial_config.caps_word = split_get_caps_word();
            if (transaction_rpc_send(RPC_SYNC_CONFIG, sizeof(vial_config_t), &vial_config)) {
                last_sync = timer_read32();
            }
        }
    }
}
