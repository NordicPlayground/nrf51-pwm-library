#ifndef __NRF_PWM_H__
#define __NRF_PWM_H__

#include <stdint.h>
#include <stdbool.h>

// The maximum number of channels supported by the library. Should NOT be changed! 
#define PWM_MAX_CHANNELS        4

// Set this to 1 if the application uses a SoftDevice, 0 otherwise
#define USE_WITH_SOFTDEVICE     0

// To change the timer used for the PWM library replace the three defines below
#define PWM_TIMER               NRF_TIMER2
#define PWM_IRQHandler          TIMER2_IRQHandler
#define PWM_IRQn                TIMER2_IRQn
#define PWM_IRQ_PRIORITY        3

// For 3-4 PWM channels a second timer is necessary
#define PWM_TIMER2              NRF_TIMER1

#define PWM_DEFAULT_CONFIG  {.num_channels   = 2,                \
                             .gpio_num       = {8,9,10,11},         \
                             .ppi_channel    = {0,1,2,3,4,5,6,7},    \
                             .gpiote_channel = {2,3,0,1},          \
                             .mode           = PWM_MODE_LED_100};

typedef enum
{
    PWM_MODE_LED_100,   // 0-100 resolution, 156Hz PWM frequency, 32kHz timer frequency (prescaler 9)
    PWM_MODE_LED_255,   // 8-bit resolution, 122Hz PWM frequency, 32kHz timer frequency (prescaler 9)
    PWM_MODE_LED_1000,  // 0-1000 resolution, 125Hz PWM frequency, 250kHz timer frequency (prescaler 6)
    
    PWM_MODE_MTR_100,   // 0-100 resolution, 20kHz PWM frequency, 2MHz timer frequency (prescaler 3)
    PWM_MODE_MTR_255,   // 8-bit resolution, 31kHz PWM frequency, 8MHz timer frequency (prescaler 1)
    
    PWM_MODE_BUZZER_255  // 8-bit resolution, 62.5kHz PWM frequency, 16MHz timer frequency (prescaler 0)
} nrf_pwm_mode_t;

typedef struct
{
    uint8_t         num_channels;
    uint8_t         gpio_num[4];
    uint8_t         ppi_channel[8];
    uint8_t         gpiote_channel[4];
    uint8_t         mode;
} nrf_pwm_config_t; 

uint32_t nrf_pwm_init(nrf_pwm_config_t *config);

void nrf_pwm_set_value(uint32_t pwm_channel, uint32_t pwm_value);

void nrf_pwm_set_values(uint32_t pwm_channel_num, uint32_t *pwm_values);

void nrf_pwm_set_max_value(uint32_t max_value);

void nrf_pwm_set_enabled(bool enabled);

#endif
