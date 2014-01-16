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

bool    new_adc_sample_ready = false;
uint8_t adc_result;

int main(void)
{     
    nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;
    
    pwm_config.mode             = PWM_MODE_LED_255;
    pwm_config.num_channels     = 2;
    pwm_config.gpio_num[0]      = 8;
    pwm_config.gpio_num[1]      = 9;  

    // Initialize the PWM library
    nrf_pwm_init(&pwm_config);

    // Configure the ADC
    // P0.1 is used for ADC input, apply a varying voltage between 0 and VDD to change the PWM values
    NRF_ADC->CONFIG = ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos | ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos |
                          ADC_CONFIG_REFSEL_SupplyOneThirdPrescaling << ADC_CONFIG_REFSEL_Pos | ADC_CONFIG_PSEL_AnalogInput2 << ADC_CONFIG_PSEL_Pos;
    NRF_ADC->ENABLE = 1;  
    NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;
    
    // Enable the ADC interrupt, and set the priority to 1
    NVIC_SetPriority(ADC_IRQn, 1);
    NVIC_EnableIRQ(ADC_IRQn);   

    // Start the ADC
    NRF_ADC->TASKS_START = 1;
    
    while (true)
    {
        // Update the PWM when this flag is set by the ADC interrupt
        if(new_adc_sample_ready)
        {
            new_adc_sample_ready = 0;
            
            // Update the PWM output based on the previous ADC sample
            nrf_pwm_set_value(0, adc_result);
            nrf_pwm_set_value(1, 255 - adc_result);
        }
    }
}

void ADC_IRQHandler(void)
{
    // Clear the END event
    NRF_ADC->EVENTS_END = 0;
    
    // Read the ADC result
    adc_result = NRF_ADC->RESULT;
    
    // Set a flag to have the PWM updated in main
    new_adc_sample_ready = 1;

    // Trigger a new ADC sampling
    NRF_ADC->TASKS_START = 1;
}


