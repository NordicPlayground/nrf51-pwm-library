/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
* @brief Example template project.
* @defgroup nrf_templates_example Example template
* @{
* @ingroup nrf_examples_nrf6310
*
* @brief Example template.
*
*/

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_pwm.h"

#define FREQ_HALF_NOTE_FACTOR 1.059463f

void set_frequency_and_duty_cycle(uint32_t frequency, uint32_t duty_cycle_percent)
{
    nrf_pwm_set_max_value((16000000 + (frequency / 2)) / frequency);
    nrf_pwm_set_value(0, (16000000 / frequency) * duty_cycle_percent / 100);
}

void pwm_init()
{
    nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;
    
    pwm_config.mode             = PWM_MODE_BUZZER_64;
    pwm_config.num_channels     = 1;
    pwm_config.gpio_num[0]      = 14;
    
    // Initialize the PWM library
    nrf_pwm_init(&pwm_config);    
}
    
int main(void)
{
    float frequency = 440.0f;
    
    // Start the external 16 MHz clock for a more accurate PWM frequency
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    
    pwm_init();
    
    while (true)
    {
        set_frequency_and_duty_cycle((uint32_t)(frequency + 0.5f), 50);
        
        frequency *= FREQ_HALF_NOTE_FACTOR;
        if(frequency > (440.0f * 4.0f)) frequency = 440.0f;

        nrf_delay_us(500000);
    }
}
