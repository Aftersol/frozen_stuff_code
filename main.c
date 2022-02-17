/* 

    Based on code from PWM Project https://github.com/rgrosset/pico-pwm-audio
    MCP3002 Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/21294E.pdf
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"

#include "hardware/regs/intctrl.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/dma.h"

#include "adc.h"
#include "audio_pwm.h"

void pwm_interrupt_handler() {
    play_slice();
}

int main() {

    /* ADC Channels from MCP3002 */
    short adc_channels[2] = {0x00,0x00};
    bool hall_effect_low = false, hall_effect_high = false;
    bool lock_state[2] = {false, false};

    stdio_init_all();
    set_sys_clock_khz(176000, true); 


    /* Set up SPI for MCP3002 ADC */
    init_adc_spi_pins();
    spi_init(spi0, 9600);

    /* Set up PWM pins */
    gpio_set_function(AUDIO_PWM_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PWM_R, GPIO_FUNC_PWM);

    int left_pwm_slice = pwm_gpio_to_slice_num(AUDIO_PWM_L);
    int right_pwm_slice = pwm_gpio_to_slice_num(AUDIO_PWM_R);

    pwm_clear_irq(left_pwm_slice);
    pwm_clear_irq(right_pwm_slice);

    /* Set up interrupts for PWM */
    irq_set_exclusive_handler(PWM_IRQ_WRAP, play_slice);
    irq_set_enabled(PWM_IRQ_WRAP, false);

    pwm_config pwm_config = pwm_get_default_config();

    /* Set up interrupt frequency so we can play a slice of sound */
    pwm_config_set_clkdiv(&pwm_config, get_clock_div_rate(8000)); 
    pwm_config_set_wrap(&pwm_config, 250); 
    pwm_init(left_pwm_slice, &pwm_config, true);
    pwm_init(right_pwm_slice, &pwm_config, true);

    pwm_set_gpio_level(AUDIO_PWM_L, 0);
    pwm_set_gpio_level(AUDIO_PWM_R, 0);

    int dma_channel = dma_claim_unused_channel(true);
    dma_channel_config dma_config;

    init_dma(&dma_channel, &dma_config);
    
    while(1)
    {
        adc_channels[0] = read_ADC(0);
        adc_channels[1] = read_ADC(1);

        /* 
            Get the signed difference between two ADC channels
            Signess of values does not matter because 10 bit values are between 0-1023
            and the max for 16 bit for signed short integer is 32767 
        */
        short adc_difference = adc_channels[0] - adc_channels[1]; 

        if (adc_difference < -100)
        {
            hall_effect_low = true;
            hall_effect_high = false;
            lock_state[1] = false;
        }
        else if (adc_difference > 100)
        {
            lock_state[0] = false;
            hall_effect_low = false;
            hall_effect_high = true;
        }
        else
        {
            hall_effect_low = false;
            hall_effect_high = false;
            lock_state[0] = false;
            lock_state[1] = false;
        }

        if ((adc_difference > 100 || adc_difference < 100) && is_playing_music() == false)
        {
            if (hall_effect_low == true && hall_effect_high == false && lock_state[0] == false)
            {
                lock_state[0] = true;
                play_music(my_music, my_music_length);
            }
            else if (hall_effect_low == false && hall_effect_high == true && lock_state[1] == false)
            {
                lock_state[1] = true;
                play_music(mcdonalds_chaos_king, mcdonalds_chaos_king_length);
            }
        }
    }

    return 0;
}