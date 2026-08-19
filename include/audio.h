#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void audio_init(void);
void audio_volume(uint8_t v);
void audio_beep(uint32_t ms);

#endif
