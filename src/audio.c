#include "audio.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"

static uint8_t volume_level = 128;

void audio_init(void) {
    gpio_set_function(AUDIO_L_PIN, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_R_PIN, GPIO_FUNC_PWM);

    pwm_config c = pwm_get_default_config();
    pwm_config_set_wrap(&c, AUDIO_PWM_WRAP);
    pwm_init(pwm_gpio_to_slice_num(AUDIO_L_PIN), &c, true);
    pwm_init(pwm_gpio_to_slice_num(AUDIO_R_PIN), &c, true);

    pwm_set_gpio_level(AUDIO_L_PIN, AUDIO_PWM_WRAP / 2);
    pwm_set_gpio_level(AUDIO_R_PIN, AUDIO_PWM_WRAP / 2);
}

void audio_volume(uint8_t v) {
    volume_level = v;
}

/* TODO: заменить на программный микшер + DMA в IRQ таймера (эмуляция YM2612/SN76489). */
void audio_beep(uint32_t ms) {
    uint32_t end = time_us_32() + ms * 1000;
    while (time_us_32() < end) {
        pwm_set_gpio_level(AUDIO_L_PIN, AUDIO_PWM_WRAP);
        pwm_set_gpio_level(AUDIO_R_PIN, AUDIO_PWM_WRAP);
        busy_wait_us_32(500);
        pwm_set_gpio_level(AUDIO_L_PIN, 0);
        pwm_set_gpio_level(AUDIO_R_PIN, 0);
        busy_wait_us_32(500);
    }
    pwm_set_gpio_level(AUDIO_L_PIN, AUDIO_PWM_WRAP / 2);
    pwm_set_gpio_level(AUDIO_R_PIN, AUDIO_PWM_WRAP / 2);
}
