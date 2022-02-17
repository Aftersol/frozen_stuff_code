/*
    https://github.com/rgrosset/pico-pwm-audio
*/
#include <stdbool.h>
#include <limits.h>

#include "audio_pwm.h"

#define SLICE_BUFFER_SIZE 8192

static uint32_t slice_position = 0;

/* Make audio buffer small to reduce memory consumption */
static uint8_t audio_buffers[2][SLICE_BUFFER_SIZE];
static uint32_t seek_track = 0;
static char seek_buffer = 0;

static uint8_t *current_music_data = NULL;
static uint16_t slice_length = 0;
static uint32_t music_data_length = 0;
static bool currently_playing = false;

static dma_channel_config* dma_config_addr = NULL;
static int* dma_channel_addr = NULL;

bool is_playing_music() { return currently_playing; }

float get_clock_div_rate(uint32_t sampling_rate)
{
    assert(sampling_rate != 0);
    
    /*
        The value for 88000 came from this equation 88000/x based on data from rgrosset's pico-pwm-audio.c assuming 176MHz
    */
    return (float)(88000.0f/sampling_rate);
}

void init_dma(int *channel, dma_channel_config *config)
{
    *channel = dma_claim_unused_channel(true);
    if (*channel == -1)
    {
        panic("Cannot claim DMA Channel\n");
    }

    *config = dma_channel_get_default_config(*channel);
    channel_config_set_read_increment(config, true);
    channel_config_set_write_increment(config, true);

    dma_channel_addr = channel;
    dma_config_addr = config;
}


void play_music(const uint8_t* music_data, uint32_t len)
{
    if (music_data == NULL || len < 1)
        return;

    /* Reset position to the start so we don't read beyond the music data */
    slice_position = 0;
    seek_track = 0;
    seek_buffer = 1;

    if (len < SLICE_BUFFER_SIZE)
    {

        uint32_t first_round_transfers = len / 4; 
        uint32_t second_round_transfers = (len % 4) / 2;
        uint32_t third_round_transfers = (len % 4) % 2;

        slice_length = len;
        
        channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_32);
        dma_channel_configure(
            *dma_channel_addr,
            dma_config_addr,
            &audio_buffers[0][0],
            &current_music_data[seek_track],
            first_round_transfers,
            true
        );

        seek_track += first_round_transfers * 4;

        channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_16);
        dma_channel_configure(
            *dma_channel_addr,
            dma_config_addr,
            &audio_buffers[0][0],
            &current_music_data[seek_track],
            second_round_transfers,
            true
        );

        seek_track += second_round_transfers * 2;

        channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_8);
        dma_channel_configure(
            *dma_channel_addr,
            dma_config_addr,
            &audio_buffers[0][0],
            &current_music_data[seek_track],
            third_round_transfers,
            true
        );
        
        seek_track += third_round_transfers;

    }
    else
    {
        channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_32);
        dma_channel_configure(
            *dma_channel_addr,
            dma_config_addr,
            &audio_buffers[0][0],
            &current_music_data[seek_track],
            2048,
            true
        );
        seek_track += 8192;
        slice_length = SLICE_BUFFER_SIZE;
    }

    seek_buffer ^= 1;
    current_music_data = music_data;
    music_data_length = len;
    currently_playing = true;

    /* Enable interrupt so we can play music */
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

void play_slice()
{
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PWM_L));
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PWM_R));

    if (slice_position < (slice_length << 3) - 1)
    {
        pwm_set_gpio_level(AUDIO_PWM_L, audio_buffers[seek_buffer ^ 1][slice_position >> 3]);
        pwm_set_gpio_level(AUDIO_PWM_R, audio_buffers[seek_buffer ^ 1][slice_position >> 3]);

        slice_position++;
    }
    else
    {
        slice_position = 0;
        uint32_t difference = music_data_length - seek_track;
        if (difference >= 8192)
        {
            channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_32);
            dma_channel_configure(
                *dma_channel_addr,
                dma_config_addr,
                &audio_buffers[seek_buffer][0],
                &current_music_data[seek_track],
                2048,
                true
            );
            seek_buffer ^= 1;
            seek_track += 8192;
            slice_length = SLICE_BUFFER_SIZE;
        }
        else if (difference > 1)
        {
            irq_set_enabled(PWM_IRQ_WRAP, false);

            uint32_t first_round_transfers = difference / 4; 
            uint32_t second_round_transfers = (difference % 4) / 2;
            uint32_t third_round_transfers = (difference % 4) % 2;

            slice_length = difference;
            
            channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_32);
            dma_channel_configure(
                *dma_channel_addr,
                dma_config_addr,
                &audio_buffers[seek_buffer][0],
                &current_music_data[seek_track],
                first_round_transfers,
                true
            );

            seek_track += first_round_transfers * 4;

            channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_16);
            dma_channel_configure(
                *dma_channel_addr,
                dma_config_addr,
                &audio_buffers[seek_buffer][0],
                &current_music_data[seek_track],
                second_round_transfers,
                true
            );

            seek_track += second_round_transfers * 2;

            channel_config_set_transfer_data_size(dma_config_addr, DMA_SIZE_8);
            dma_channel_configure(
                *dma_channel_addr,
                dma_config_addr,
                &audio_buffers[seek_buffer][0],
                &current_music_data[seek_track],
                third_round_transfers,
                true
            );
            
            seek_track += third_round_transfers;
            seek_buffer ^= 1;
            irq_set_enabled(PWM_IRQ_WRAP, true);

        }
        else 
        {   
            stop_music();
        }
    }

}

void stop_music()
{
    irq_set_enabled(PWM_IRQ_WRAP, false);
    currently_playing = false;
}