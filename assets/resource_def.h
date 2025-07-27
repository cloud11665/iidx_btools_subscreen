#pragma once

// fonts
#define IDR_iidx_font	101

// textures
#define IDR_icon_home		201
#define IDR_icon_keypad		202
#define IDR_keypad			203
#define IDR_keypad_close	204
#define IDR_icon_card       208
#define IDR_icon_settings   209
#define IDR_icon_concentration_on 214
#define IDR_icon_concentration_off 215
#define IDR_icon_coin 216

#define IDR_effector_bg     210
#define IDR_effector_0      211
#define IDR_effector_1      212
#define IDR_effector_head   213

#define IDR_touch_effect01  205
#define IDR_touch_effect_cross	206
#define IDR_touch_effect_cross2 207


// X-MARCORS
#define RESOURCE_IS_FONT(x) (x >= 100 && x < 200)
#define RESOURCE_IS_TEXTURE(x) (x >= 200 && x < 300)

#define RESOURCE_FONT_TABLE(_)       \
    _(iidx_font)

#define RESOURCE_TEXTURE_TABLE(_)    \
    _(icon_home)                     \
    _(icon_keypad)                   \
    _(keypad)                        \
    _(keypad_close)                  \
    _(icon_card)                     \
    _(icon_settings)                 \
    _(effector_bg)                   \
    _(effector_0)                    \
    _(effector_1)                    \
    _(effector_head)                 \
    _(touch_effect01)                \
    _(touch_effect_cross)            \
    _(touch_effect_cross2)           \
    _(icon_concentration_on)         \
    _(icon_concentration_off)        \
    _(icon_coin)
