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
    OLED_SPACESHIP,
    OLED_MEDIA_VER,
    OLED_MEDIA_HOR,
    OLED_DISABLED,
} oled_mode_t;

oled_mode_t get_oled_mode_on_half(bool on_master) {
    if (on_master) return vial_config.oled_master;

    // first two modes swapped for slave
    if (vial_config.oled_slave == OLED_STATUS_CLASSIC) return OLED_SPLASH;
    if (vial_config.oled_slave == OLED_SPLASH) return OLED_STATUS_CLASSIC;

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
        case OLED_MEDIA_HOR:
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
    if (split_get_mac()) {
        oled_write_P(PSTR("Mac"), false);
    } else {
        oled_write_P(PSTR("Win"), false);
    }

    // Print current layer
    oled_set_cursor(0, 10);
    oled_write_P(PSTR("LAYER"), false);
    oled_set_cursor(0, 12);
    oled_write_P(PSTR(layer_name(get_current_layer())), false);

    oled_set_cursor(0, 15);
    bool caps = host_keyboard_led_state().caps_lock || split_get_caps_word();
    oled_write_P(PSTR("CPSLK"), caps);
}

void render_status_modern(void) {
    oled_clear();
    oled_write_ln(layer_upper_name(get_current_layer()), false);
    oled_set_cursor(0, 1);
    if (split_get_mac())
        oled_write_P(PSTR("   \01\02   \03\04"), false);
    else
        oled_write_P(PSTR("          "), false);

    oled_set_cursor(0, 2);
    oled_write(split_get_lang() == LANG_EN ? "EN" : "RU", false);

    oled_set_cursor(0, 4);
    led_t led_usb_state = host_keyboard_led_state();
    bool  caps          = led_usb_state.caps_lock || split_get_caps_word();
    oled_write_P(led_usb_state.num_lock ? PSTR("NUM\07\10") : PSTR("NUM\05\06"), false);
    oled_write_P(caps ? PSTR("CPS\07\10") : PSTR("CPS\05\06"), false);
    oled_write_P(led_usb_state.scroll_lock ? PSTR("SCR\07\10") : PSTR("SCR\05\06"), false);

    oled_set_cursor(0, 8);
    uint8_t mods = get_mods() | get_oneshot_mods();
    oled_write_P(mods & MOD_MASK_SHIFT ? PSTR("SFT\07\10") : PSTR("SFT\05\06"), false);
    oled_write_P(mods & MOD_MASK_CTRL ? PSTR("CTL\07\10") : PSTR("CTL\05\06"), false);
    oled_write_P(mods & MOD_MASK_ALT ? PSTR("ALT\07\10") : PSTR("ALT\05\06"), false);
    oled_write_P(mods & MOD_MASK_GUI ? PSTR("GUI\07\10") : PSTR("GUI\05\06"), false);

    char buf[16];
    int  wpm = get_current_wpm();
    if (wpm < 10)
        sprintf(buf, "WPM %d", wpm);
    else if (wpm < 100)
        sprintf(buf, "W  %d", wpm);
    else
        sprintf(buf, "W %d", wpm);
    oled_set_cursor(0, 13);
    oled_write_ln(buf, false);
}

void render_big_num(int num, char* c0, char* c1, char* c2, char* c3) {
    *c0 = 0x80 + num * 2;
    *c1 = 0x81 + num * 2;
    *c2 = 0xa0 + num * 2;
    *c3 = 0xa1 + num * 2;
}

const char* render_clock_ver(uint8_t hours, uint8_t minutes) {
    static char buf[26] = "                         ";
    render_big_num(hours / 10, buf + 0, buf + 1, buf + 5, buf + 6);
    render_big_num(hours % 10, buf + 2, buf + 3, buf + 7, buf + 8);
    render_big_num(minutes / 10, buf + 16, buf + 17, buf + 21, buf + 22);
    render_big_num(minutes % 10, buf + 18, buf + 19, buf + 23, buf + 24);
    return buf;
}

void render_volume_ver(int volume) {
    // clang-format off
    const char* vol_str[] = {
        "\xCC\xC0\xC0\xC0\xCD\0",
        "\xCC\xC1\xC1\xC1\xCD\0",
        "\xCC\xC2\xC2\xC2\xCD\0",
        "\xCC\xC3\xC3\xC3\xCD\0",
        "\xCC\xC4\xC4\xC4\xCD\0",
        "\xCC\xC5\xC5\xC5\xCD\0",
        "\xCC\xC6\xC6\xC6\xCD\0",
        "\xCC\xC7\xC7\xC7\xCD\0",
        "\xCC\xC8\xC8\xC8\xCD\0",
    };
    // clang-format on

    char buf[6];
    sprintf(buf, " %2d%%", volume);
    oled_write(buf, false);

    oled_set_cursor(0, 1);
    oled_write("\xC9\xCA\xCA\xCA\xCB", false);
    for (int i = 0; i < 10; i++) {
        int t1 = volume - (9 - i) * 10;
        int t2 = MIN(MAX(t1, 0), 10);
        int t3 = (t2 * 8 + 5) / 10;
        oled_write(vol_str[t3], false);
    }
    oled_write("\xCE\xCF\xCF\xCF\xD0", false);
    oled_write(" VOL ", false);
}

void render_media_ver(void) {
    static uint32_t volume_changed_stamp = 0;
    static uint32_t time_changed_stamp   = 0;

    hid_data_t* hid_data = get_hid_data();
    if (hid_data->volume_changed) {
        volume_changed_stamp     = timer_read32();
        hid_data->volume_changed = false;
    }

    if (hid_data->time_changed) {
        time_changed_stamp     = timer_read32();
        hid_data->time_changed = false;
    }

    oled_clear();
    if (timer_elapsed32(volume_changed_stamp) < 2 * 1000) {
        render_volume_ver(hid_data->volume);
    } else if (timer_elapsed32(time_changed_stamp) < 61 * 1000) {
        oled_set_cursor(0, 5);
        oled_write(render_clock_ver(hid_data->hours, hid_data->minutes), false);
    }
}

void render_media_hor(void) {
    const int LINE_LEN = 21;

    hid_data_t* hid_data = get_hid_data();
    if (hid_data->media_artist_changed || hid_data->media_title_changed) {
        char title_buf[LINE_LEN + 1];
        int  title_len   = strlen(hid_data->media_title);
        int  title_shift = (LINE_LEN - MIN(title_len, LINE_LEN)) / 2;
        for (int i = 0; i < LINE_LEN; i++) {
            if (i < title_shift) {
                title_buf[i] = ' ';
                continue;
            }
            char c = hid_data->media_title[i - title_shift];
            if (c == '\0') {
                title_buf[i] = '\0';
                break;
            }
            title_buf[i] = toupper(c);
        }
        title_buf[LINE_LEN] = '\0';

        hid_data->media_title_changed = false;

        char artist_buf[LINE_LEN + 1];
        int  artist_len   = strlen(hid_data->media_artist);
        int  artist_shift = (LINE_LEN - MIN(artist_len, LINE_LEN)) / 2;
        for (int i = 0; i < LINE_LEN; i++) {
            if (i < artist_shift) {
                artist_buf[i] = ' ';
                continue;
            }
            char c = hid_data->media_artist[i - artist_shift];
            if (c == '\0') {
                artist_buf[i] = '\0';
                break;
            }
            artist_buf[i] = c;
        }
        artist_buf[LINE_LEN] = '\0';

        hid_data->media_artist_changed = false;

        oled_clear();
        oled_set_cursor(0, 0);
        oled_write(title_buf, false);
        oled_set_cursor(0, 2);
        oled_write(artist_buf, false);
    }
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

       case OLED_MEDIA_HOR:
            render_media_hor();
            break;

        case OLED_MEDIA_VER:
            render_media_ver();
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
