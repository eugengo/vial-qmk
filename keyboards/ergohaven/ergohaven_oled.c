#include "ergohaven.h"
#include "info_config.h"
#include "ergohaven_ruen.h"
#include "hid.h"
#include "transactions.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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
    // Print current mode
    oled_clear();
    oled_write_P(EH_SHORT_PRODUCT_NAME, false);

    oled_set_cursor(0, 2);
    oled_write_P(PSTR(EH_VERSION_STR), false);

    oled_set_cursor(0, 5);
    oled_write_P("MODE:", false);
    oled_set_cursor(0, 7);


    // Print current layer
    oled_set_cursor(0, 10);
    oled_write_P(PSTR("LAYER"), false);
    oled_set_cursor(0, 12);
    oled_write_P(PSTR(layer_name(get_current_layer())), false);

    oled_set_cursor(0, 15);
    bool caps = host_keyboard_led_state().caps_lock || split_get_caps_word();
    oled_write_P(PSTR("CPSLK"), caps);
}

void render_space(void) {
    oled_write_P(PSTR("     "), false);
}

//
void render_name(void) {
     oled_write_P(EH_SHORT_PRODUCT_NAME, false);
}

void render_version(void) {
    oled_write_P(PSTR(EH_VERSION_STR), false);
}

// Renders the logo and optionally WPM
void render_apple_logo(void) {
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

// Renders modifier status (GUI/ALT or CTRL/SHIFT)
void render_mod_status(uint8_t modifiers, bool gui_alt) {
    //oled_set_cursor(0, 5);
    // Icon sets for GUI/ALT and CTRL/SHIFT
    static const char PROGMEM icons[][2][3] = {
        // [0] = off, [1] = on
        // GUI/CTRL
        {{0x80, 0x81, 0}, {0x88, 0x89, 0}}, // GUI
        {{0x82, 0x83, 0}, {0x8a, 0x8b, 0}}, // ALT
        {{0x84, 0x85, 0}, {0x8c, 0x8d, 0}}, // CTRL
        {{0x86, 0x87, 0}, {0x8e, 0x8f, 0}}, // SHIFT
        // Second row
        {{0xa0, 0xa1, 0}, {0xa8, 0xa9, 0}}, // GUI
        {{0xa2, 0xa3, 0}, {0xaa, 0xab, 0}}, // ALT
        {{0xa4, 0xa5, 0}, {0xac, 0xad, 0}}, // CTRL
        {{0xa6, 0xa7, 0}, {0xae, 0xaf, 0}}, // SHIFT
    };
    // Fillers between icons
    static const char PROGMEM fillers[4][2][2] = {
        {{0xc0, 0}, {0xc6, 0}}, // off_off, on_on
        {{0xc2, 0}, {0xc4, 0}}, // on_off, off_on
        {{0xc1, 0}, {0xc7, 0}}, // off_off_2, on_on_2
        {{0xc3, 0}, {0xc5, 0}}, // on_off_2, off_on_2
    };

    if (gui_alt) {
        // GUI/ALT row 1
        oled_write_P(icons[0][(modifiers & MOD_MASK_GUI) ? 1 : 0], false);
        if ((modifiers & MOD_MASK_GUI) && (modifiers & MOD_MASK_ALT)) {
            oled_write_P(fillers[0][1], false);
        } else if (modifiers & MOD_MASK_GUI) {
            oled_write_P(fillers[1][0], false);
        } else if (modifiers & MOD_MASK_ALT) {
            oled_write_P(fillers[1][1], false);
        } else {
            oled_write_P(fillers[0][0], false);
        }
        oled_write_P(icons[1][(modifiers & MOD_MASK_ALT) ? 1 : 0], false);
        // GUI/ALT row 2
        oled_write_P(icons[4][(modifiers & MOD_MASK_GUI) ? 1 : 0], false);
        if ((modifiers & MOD_MASK_GUI) && (modifiers & MOD_MASK_ALT)) {
            oled_write_P(fillers[2][1], false);
        } else if (modifiers & MOD_MASK_GUI) {
            oled_write_P(fillers[3][0], false);
        } else if (modifiers & MOD_MASK_ALT) {
            oled_write_P(fillers[3][1], false);
        } else {
            oled_write_P(fillers[2][0], false);
        }
        oled_write_P(icons[5][(modifiers & MOD_MASK_ALT) ? 1 : 0], false);
    } else {
        // CTRL/SHIFT row 1
        oled_write_P(icons[2][(modifiers & MOD_MASK_CTRL) ? 1 : 0], false);
        if ((modifiers & MOD_MASK_CTRL) && (modifiers & MOD_MASK_SHIFT)) {
            oled_write_P(fillers[0][1], false);
        } else if (modifiers & MOD_MASK_CTRL) {
            oled_write_P(fillers[1][0], false);
        } else if (modifiers & MOD_MASK_SHIFT) {
            oled_write_P(fillers[1][1], false);
        } else {
            oled_write_P(fillers[0][0], false);
        }
        oled_write_P(icons[3][(modifiers & MOD_MASK_SHIFT) ? 1 : 0], false);
        // CTRL/SHIFT row 2
        oled_write_P(icons[6][(modifiers & MOD_MASK_CTRL) ? 1 : 0], false);
        if ((modifiers & MOD_MASK_CTRL) && (modifiers & MOD_MASK_SHIFT)) {
            oled_write_P(fillers[2][1], false);
        } else if (modifiers & MOD_MASK_CTRL) {
            oled_write_P(fillers[3][0], false);
        } else if (modifiers & MOD_MASK_SHIFT) {
            oled_write_P(fillers[3][1], false);
        } else {
            oled_write_P(fillers[2][0], false);
        }
        oled_write_P(icons[7][(modifiers & MOD_MASK_SHIFT) ? 1 : 0], false);
    }
}

// Main unified status render function
void render_status_darkside(bool is_master) {
    // Render logo (with or without WPM)
    render_name();
    render_space();
    //oled_clear();
    render_apple_logo();

    // Render current layer
    //render_layer_state();
    //render_space();

    // Render modifier status
    uint8_t mods = get_mods() | get_oneshot_mods();
    render_mod_status(mods, true);  // GUI/ALT
    render_mod_status(mods, false); // CTRL/SHIFT
}

void render_status_modern(void) {
    oled_clear();
    render_version();
    render_space();
    render_apple_logo();
    render_space();
    oled_write_ln(layer_upper_name(get_current_layer()), false);
    oled_set_cursor(0, 1);

        oled_write_P(PSTR("   \00\01   \02\03"), false);

        oled_write_P(PSTR("          "), false);

    oled_set_cursor(0, 4);
    led_t led_usb_state = host_keyboard_led_state();
    bool  caps          = led_usb_state.caps_lock || split_get_caps_word();
    oled_write_P(led_usb_state.num_lock ? PSTR("NUM\07\10") : PSTR("NUM\05\06"), false);
    oled_write_P(caps ? PSTR("CPS\07\10") : PSTR("CPS\05\06"), false);
    oled_write_P(led_usb_state.scroll_lock ? PSTR("SCR\07\10") : PSTR("SCR\05\06"), false);
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
