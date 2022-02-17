#ifndef AUDIO_PWM_H
#define AUDIO_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "hardware/pwm.h"

#include "hardware/dma.h"

#include "hardware/regs/intctrl.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include "audio_samples.h"

#define AUDIO_PWM_L 27
#define AUDIO_PWM_R 28

#define AUDIO_PWM pwm6

bool is_playing_music();

float get_clock_div_rate(uint32_t sampling_rate);

void init_dma(int *channel, dma_channel_config *config);

void play_music(const uint8_t* music_data, uint32_t len);
void play_slice();
void stop_music();

#ifdef __cplusplus
}
#endif

#endif