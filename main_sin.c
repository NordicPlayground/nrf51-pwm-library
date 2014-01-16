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

const uint8_t sin_table[] = {0, 0,1,2,4,6,9,12,16,20,24,29,35,40,	46,	53,	59,	66,	74,	81,	88,	96,	104,112,120,128,136,144,152,160,168,175,182,190,197,203,210,216,221,227,
               232,236,240,244,247,250,252,254,255,255,255,255,255,254,252,250,247,244,240,236,232,227,221,216,210,203,197,190,182,175,168,160,152,144,136,128,120,112,104,
               96,88,81,74,66,59,	53,	46,	40,	35,	29,24,	20,	16,	12,	9,	6,	4,	2,1,0};
        
int main(void)
{
    uint32_t counter = 0;
    
    nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;
    
    pwm_config.mode             = PWM_MODE_LED_100;
    pwm_config.num_channels     = 3;
    pwm_config.gpio_num[0]      = 8;
    pwm_config.gpio_num[1]      = 9;
    pwm_config.gpio_num[2]      = 10;    
    
    // Initialize the PWM library
    nrf_pwm_init(&pwm_config);

    while (true)
    {
        // Update the 3 outputs with out of phase sine waves
        nrf_pwm_set_value(0, sin_table[counter]);
        nrf_pwm_set_value(1, sin_table[(counter + 33) % 100]);
        nrf_pwm_set_value(2, sin_table[(counter + 66) % 100]);
        counter = (counter + 1) % 100;
        
        // Add a delay to control the speed of the sine wave
        nrf_delay_us(8000);
    }
}


