#ifndef __NRF_PWM_NOGLITCH_H__
#define __NRF_PWM_NOGLITCH_H__

#include <stdint.h>

// The maximum number of channels supported by the library. Should NOT be changed! 
#define PWM_MAX_CHANNELS        2

// Set this to 1 if the application uses a SoftDevice, 0 otherwise
#define USE_WITH_SOFTDEVICE     1

// To change the timer used for the PWM library replace the three defines below
#define PWM_TIMER               NRF_TIMER2
#define PWM_IRQHandler          TIMER2_IRQHandler
#define PWM_IRQn                TIMER2_IRQn
#define PWM_IRQ_PRIORITY        3

#define PWM_DEFAULT_CONFIG  {.num_channels   = 2,                \
                             .gpio_num       = {8,9},            \
                             .ppi_channel    = {0,1,2,3,4,5,6},    \
                             .ppi_group      = {0},              \
                             .gpiote_channel = {0,1},            \
                             .mode           = PWM_MODE_LED_100};

/**@brief PWM frequency */
typedef enum
{
    PWM_MODE_LED_100,   // 0-100 resolution, 156 Hz PWM frequency, 32 kHz timer frequency (prescaler 9)
    PWM_MODE_LED_255,   // 8-bit resolution, 122 Hz PWM frequency, 65 kHz timer frequency (prescaler 8)
    PWM_MODE_LED_1000,  // 0-1000 resolution, 125 Hz PWM frequency, 500 kHz timer frequency (prescaler 5)
    
    PWM_MODE_MTR_100,   // 0-100 resolution, 20 kHz PWM frequency, 4MHz timer frequency (prescaler 2)
    PWM_MODE_MTR_255,   // 8-bit resolution, 31 kHz PWM frequency, 16MHz timer frequency (prescaler 0)
    
    PWM_MODE_BUZZER_64  // 0-64  resolution, 125 kHz PWM frequency, 16 MHz timer frequency (prescaler 0)
} nrf_pwm_mode_t;

/**@brief PWM configuration */
typedef struct
{
    uint8_t         num_channels;
    uint8_t         gpio_num[2];
    uint8_t         ppi_channel[7];
    uint8_t         ppi_group[1];
    uint8_t         gpiote_channel[2];
    uint8_t         mode;
} nrf_pwm_config_t; 

/**@brief Initialize PWM library
 *
 * @note Run after sd_softdevice_enable() when using SoftDevice.
 *
 * @params[in] config PWM configuration
 * @return 0 when successful
 */
uint32_t nrf_pwm_init(nrf_pwm_config_t *config);

/**@brief Update PWM duty cycle
 * TODO: Let pwm_value argument be percentage instead of magic compare value
 *
 * @params[in] pwm_channel Channel to update [0, PWM_MAX_CHANNELS]
 * @params[in] pwm_value   Duty cycle (Use @ref nrf_pwm_get_max_value() to get 100% duty cycle value)
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_BUSY Previous update has not yet finished
 */
uint32_t nrf_pwm_set_value(uint32_t pwm_channel, uint32_t pwm_value);

uint32_t nrf_pwm_get_max_value(void);

#endif /* __NRF_PWM_NOGLITCH_H__ */
