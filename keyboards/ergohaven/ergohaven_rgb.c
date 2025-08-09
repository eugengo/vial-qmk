#include "quantum.h"
#include "ergohaven.h"
#include "ergohaven_rgb.h"

// Custom HSV color definitions
#define HSV_MY_RED  0, 255, 255
#define HSV_MY_BLUE    160, 255, 200
#define HSV_MY_YELLOW     40, 255, 120

// RGB segments for each layer
const rgblight_segment_t PROGMEM layer0_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_BLACK});
const rgblight_segment_t PROGMEM layer1_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_MY_RED});
const rgblight_segment_t PROGMEM layer2_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_MY_BLUE});
const rgblight_segment_t PROGMEM layer3_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_MY_YELLOW});
const rgblight_segment_t PROGMEM layer4_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_GREEN});
const rgblight_segment_t PROGMEM layer5_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_PURPLE});
const rgblight_segment_t PROGMEM layer6_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_BLUE});
const rgblight_segment_t PROGMEM layer7_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_PINK});
const rgblight_segment_t PROGMEM layer8_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_SPRINGGREEN});
const rgblight_segment_t PROGMEM layer9_rgb[]  = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_YELLOW});
const rgblight_segment_t PROGMEM layer10_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_TEAL});
const rgblight_segment_t PROGMEM layer11_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_ORANGE});
const rgblight_segment_t PROGMEM layer12_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_AZURE});
const rgblight_segment_t PROGMEM layer13_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_CHARTREUSE});
const rgblight_segment_t PROGMEM layer14_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_CORAL});
const rgblight_segment_t PROGMEM layer15_rgb[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_GOLD});

// Array of all RGB layers; later layers take precedence
// clang-format off
const rgblight_segment_t* const PROGMEM my_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    layer0_rgb, layer1_rgb, layer2_rgb, layer3_rgb, layer4_rgb, layer5_rgb, layer6_rgb, layer7_rgb,
    layer8_rgb, layer9_rgb, layer10_rgb, layer11_rgb, layer12_rgb, layer13_rgb, layer14_rgb, layer15_rgb
);
// clang-format on

// Called after keyboard initialization to set up RGB layers
void keyboard_post_init_rgb(void) {
    rgblight_layers = my_rgb_layers;
}

// This function sets the RGB effect depending on the active layer
void layer_state_set_rgb(layer_state_t state) {
    // If only layer 0 is active, set static light mode
        if (state & (1 << 0)) {
            rgblight_mode(RGBLIGHT_MODE_STATIC_LIGHT);
        } else {
            // For any other active layer, set breathing effect
            rgblight_mode(RGBLIGHT_MODE_BREATHING 1);
        }
        // Enable RGB for all active layers except 0
    for (int layer = 1; layer <= _FIFTEEN; ++layer)
        rgblight_set_layer_state(layer, layer_state_cmp(state, layer));
}

static bool is_rgb_on = false;

void rgb_on(void) {
    if (!is_rgb_on) {
        rgblight_wakeup();
        is_rgb_on = true;
    }
}

void rgb_off(void) {
    if (is_rgb_on) {
        rgblight_suspend();
        is_rgb_on = false;
    }
}
