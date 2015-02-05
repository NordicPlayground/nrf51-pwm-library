#include "nrf_pwm.h"
#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#if(USE_WITH_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

static uint32_t pwm_max_value, pwm_next_value[PWM_MAX_CHANNELS], pwm_next_max_value, pwm_io_ch[PWM_MAX_CHANNELS], pwm_running[PWM_MAX_CHANNELS];
static bool pwm_modified[PWM_MAX_CHANNELS];
static uint8_t pwm_gpiote_channel[PWM_MAX_CHANNELS];
static uint32_t pwm_num_channels;
static uint32_t pwm_cc_update_margin_ticks = 10;
static const uint8_t pwm_cc_margin_by_prescaler[] = {80, 40, 20, 10, 5, 2, 1, 1, 1, 1};

#define PWM_TIMER_CURRENT  PWM_TIMER->CC[3]
#define PWM_TIMER2_CURRENT PWM_TIMER2->CC[3]

void PWM_IRQHandler(void);

static void apply_pan73_workaround(NRF_TIMER_Type *timer, bool enable)
{
    if(timer == NRF_TIMER0)
    {
        *(uint32_t *)0x40008C0C = (enable ? 1 : 0);
    }
    else if(timer == NRF_TIMER1)
    {
        *(uint32_t *)0x40009C0C = (enable ? 1 : 0);
    }
    else if(timer == NRF_TIMER2)
    {
        *(uint32_t *)0x4000AC0C = (enable ? 1 : 0);
    }
}

static __INLINE bool safe_margins_present(uint32_t timer_state, uint32_t compare_state)  // Approx runtime ~2us
{
    if(compare_state <= pwm_cc_update_margin_ticks)
    {
        if(timer_state >= compare_state && timer_state < (pwm_max_value + compare_state - pwm_cc_update_margin_ticks)) return true;
        else return false;
    }
    else
    {
        if(timer_state < (compare_state - pwm_cc_update_margin_ticks) || timer_state >= compare_state) return true;
        else return false;
    }
}

static void ppi_enable_channel(uint32_t ch_num, volatile uint32_t *event_ptr, volatile uint32_t *task_ptr)
{
    if(ch_num >= 16) return;
    else
    {
#if(USE_WITH_SOFTDEVICE == 1)
        sd_ppi_channel_assign(ch_num, event_ptr, task_ptr);
        sd_ppi_channel_enable_set(1 << ch_num);
#else
        // Otherwise we configure the channel and return the channel number
        NRF_PPI->CH[ch_num].EEP = (uint32_t)event_ptr;
        NRF_PPI->CH[ch_num].TEP = (uint32_t)task_ptr;    
        NRF_PPI->CHENSET = (1 << ch_num);   
#endif
    }
}

#if(USE_WITH_SOFTDEVICE == 1)
nrf_radio_signal_callback_return_param_t *nrf_radio_signal_callback(uint8_t signal_type)
{
    static nrf_radio_signal_callback_return_param_t return_params;
    return_params.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_END;
    switch(signal_type)
    {
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:            /**< This signal indicates the start of the radio timeslot. */
            PWM_IRQHandler();
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:            /**< This signal indicates the NRF_TIMER0 interrupt. */
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:             /**< This signal indicates the NRF_RADIO interrupt. */
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:     /**< This signal indicates extend action failed. */
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:   /**< This signal indicates extend action succeeded. */
            break;
    }
    return &return_params;
}  
#endif

uint32_t nrf_pwm_init(nrf_pwm_config_t *config)
{
    if(config->num_channels == 0 || config->num_channels > PWM_MAX_CHANNELS) return 0xFFFFFFFF;
    
    switch(config->mode)
    {
        case PWM_MODE_LED_100:   // 0-100 resolution, 321Hz PWM frequency, 32kHz timer frequency (prescaler 9)
            PWM_TIMER->PRESCALER = 9; 
            pwm_max_value = 100;
            break;
        case PWM_MODE_LED_255:   // 8-bit resolution, 122Hz PWM frequency, 32kHz timer frequency (prescaler 9)
            PWM_TIMER->PRESCALER = 9;
            pwm_max_value = 255;  
            break;
        case PWM_MODE_LED_1000:  // 0-1000 resolution, 250Hz PWM frequency, 250kHz timer frequency (prescaler 6)
            PWM_TIMER->PRESCALER = 6;
            pwm_max_value = 1000;
            break;
        case PWM_MODE_MTR_100:   // 0-100 resolution, 20kHz PWM frequency, 2MHz timer frequency (prescaler 3)
            PWM_TIMER->PRESCALER = 3;
            pwm_max_value = 100;
            break;
        case PWM_MODE_MTR_255:    // 8-bit resolution, 31kHz PWM frequency, 8MHz timer frequency (prescaler 1)	
            PWM_TIMER->PRESCALER = 1;
            pwm_max_value = 255;
            break;
        case PWM_MODE_BUZZER_255:  // 8-bit resolution, 62.5kHz PWM frequency, 16MHz timer frequency (prescaler 0)
            PWM_TIMER->PRESCALER = 0;
            pwm_max_value = 255;
            break;
        default:
            return 0xFFFFFFFF;
    }
    pwm_cc_update_margin_ticks = pwm_cc_margin_by_prescaler[PWM_TIMER->PRESCALER];
    pwm_num_channels = config->num_channels;
    for(int i = 0; i < pwm_num_channels; i++)
    {
        pwm_io_ch[i] = (uint32_t)config->gpio_num[i];
        nrf_gpio_cfg_output(pwm_io_ch[i]);
        pwm_running[i] = 0;       
        pwm_gpiote_channel[i] = config->gpiote_channel[i];        
    }
    PWM_TIMER->TASKS_CLEAR = 1;
    PWM_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
    PWM_TIMER->CC[2] = pwm_next_max_value = pwm_max_value;
	PWM_TIMER->MODE = TIMER_MODE_MODE_Timer;
    PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE2_CLEAR_Msk;
    PWM_TIMER->EVENTS_COMPARE[0] = PWM_TIMER->EVENTS_COMPARE[1] = PWM_TIMER->EVENTS_COMPARE[2] = PWM_TIMER->EVENTS_COMPARE[3] = 0;     
    
    if(pwm_num_channels > 2)
    {
        PWM_TIMER2->TASKS_CLEAR = 1;
        PWM_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
        PWM_TIMER2->CC[2] = pwm_next_max_value = pwm_max_value;
        PWM_TIMER2->MODE = TIMER_MODE_MODE_Timer;
        PWM_TIMER2->SHORTS = TIMER_SHORTS_COMPARE2_CLEAR_Msk;
        PWM_TIMER2->EVENTS_COMPARE[0] = PWM_TIMER2->EVENTS_COMPARE[1] = PWM_TIMER2->EVENTS_COMPARE[2] = PWM_TIMER2->EVENTS_COMPARE[3] = 0;             
        PWM_TIMER2->PRESCALER = PWM_TIMER->PRESCALER;
    }

    for(int i = 0; i < pwm_num_channels && i < 2; i++)
    {
        ppi_enable_channel(config->ppi_channel[i*2],  &PWM_TIMER->EVENTS_COMPARE[i], &NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]]);
        ppi_enable_channel(config->ppi_channel[i*2+1],&PWM_TIMER->EVENTS_COMPARE[2], &NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]]);  
        pwm_modified[i] = false;        
    }
    for(int i = 2; i < pwm_num_channels; i++)
    {
        ppi_enable_channel(config->ppi_channel[i*2],  &PWM_TIMER2->EVENTS_COMPARE[i-2], &NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]]);
        ppi_enable_channel(config->ppi_channel[i*2+1],&PWM_TIMER2->EVENTS_COMPARE[2], &NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]]);  
        pwm_modified[i] = false;        
    }
#if(USE_WITH_SOFTDEVICE == 1)
    sd_radio_session_open(nrf_radio_signal_callback);
#else
    NVIC_SetPriority(PWM_IRQn, 0);
    NVIC_EnableIRQ(PWM_IRQn);
#endif
    apply_pan73_workaround(PWM_TIMER, true);
    PWM_TIMER->TASKS_START = 1;
    if(pwm_num_channels > 2)
    {
        apply_pan73_workaround(PWM_TIMER2, true);
        PWM_TIMER2->TASKS_START = 1;
    }
    return 0;
}

void nrf_pwm_set_value(uint32_t pwm_channel, uint32_t pwm_value)
{
    pwm_next_value[pwm_channel] = pwm_value;
    pwm_modified[pwm_channel] = true;
#if(USE_WITH_SOFTDEVICE == 1)
    nrf_radio_request_t radio_request;
    radio_request.request_type = NRF_RADIO_REQ_TYPE_EARLIEST;
    radio_request.params.earliest.hfclk = NRF_RADIO_HFCLK_CFG_DEFAULT;
    radio_request.params.earliest.length_us = 250;
    radio_request.params.earliest.priority = NRF_RADIO_PRIORITY_HIGH;
    radio_request.params.earliest.timeout_us = 100000;
    sd_radio_request(&radio_request);
#else
    NVIC_SetPendingIRQ(PWM_IRQn);
#endif
}
 
void nrf_pwm_set_values(uint32_t pwm_channel_num, uint32_t *pwm_values)
{
    for(int i = 0; i < pwm_channel_num; i++)
    {
        pwm_next_value[i] = pwm_values[i];
        pwm_modified[i] = true;
    }
#if(USE_WITH_SOFTDEVICE == 1)
    nrf_radio_request_t radio_request;
    radio_request.request_type = NRF_RADIO_REQ_TYPE_EARLIEST;
    radio_request.params.earliest.hfclk = NRF_RADIO_HFCLK_CFG_DEFAULT;
    radio_request.params.earliest.length_us = 250;
    radio_request.params.earliest.priority = NRF_RADIO_PRIORITY_HIGH;
    radio_request.params.earliest.timeout_us = 100000;
    sd_radio_request(&radio_request);
#else
    NVIC_SetPendingIRQ(PWM_IRQn);
#endif
}

void nrf_pwm_set_max_value(uint32_t max_value)
{
    pwm_next_max_value = max_value;
}

void nrf_pwm_set_enabled(bool enabled)
{
    if(enabled)
    {
        PWM_TIMER->TASKS_START = 1;
        if(pwm_num_channels > 2) PWM_TIMER2->TASKS_START = 1;
    }
    else
    {
        PWM_TIMER->TASKS_STOP = 1;
        if(pwm_num_channels > 2) PWM_TIMER2->TASKS_STOP = 1;
        for(uint32_t i = 0; i < pwm_num_channels; i++)
        {
            nrf_gpiote_unconfig(pwm_gpiote_channel[i]);
            nrf_gpio_pin_write(pwm_io_ch[i], 0); 
            pwm_running[i] = 0;
        }       
    }
}

void PWM_IRQHandler(void)
{
    static uint32_t i, new_capture, old_capture;
    PWM_TIMER->CC[2] = pwm_max_value = pwm_next_max_value;
    if(pwm_num_channels > 2) PWM_TIMER2->CC[2] = pwm_max_value;
    for(i = 0; i < pwm_num_channels; i++)
    {
        if(pwm_modified[i])
        {
            pwm_modified[i] = false;
            if(pwm_next_value[i] == 0)
            {
                nrf_gpiote_unconfig(pwm_gpiote_channel[i]);
                nrf_gpio_pin_write(pwm_io_ch[i], 0);
                pwm_running[i] = 0;
            }
            else if (pwm_next_value[i] >= pwm_max_value)
            {
                nrf_gpiote_unconfig(pwm_gpiote_channel[i]);
                nrf_gpio_pin_write(pwm_io_ch[i], 1); 
                pwm_running[i] = 0;
            }
            else
            {
                if(i < 2)
                {
                    new_capture = pwm_next_value[i];
                    old_capture = PWM_TIMER->CC[i];
                    if(!pwm_running[i])
                    {
                        nrf_gpiote_task_config(pwm_gpiote_channel[i], pwm_io_ch[i], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);  
                        pwm_running[i] = 1;
                        PWM_TIMER->TASKS_CAPTURE[3] = 1;
                        if(PWM_TIMER->CC[3] > new_capture) NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]] = 1;
                        PWM_TIMER->CC[i] = new_capture;
                    }
                    else
                    {
                        while(1)
                        {
                            PWM_TIMER->TASKS_CAPTURE[3] = 1;
                            if(safe_margins_present(PWM_TIMER_CURRENT, old_capture) && safe_margins_present(PWM_TIMER_CURRENT, new_capture)) break;
                        }
                        if((PWM_TIMER_CURRENT >= old_capture && PWM_TIMER_CURRENT < new_capture) || (PWM_TIMER_CURRENT < old_capture && PWM_TIMER_CURRENT >= new_capture))
                        {
                            NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]] = 1;
                        }
                        PWM_TIMER->CC[i] = new_capture;
                    }
                }
                else
                {
                    new_capture = pwm_next_value[i];
                    old_capture = PWM_TIMER2->CC[i-2];
                    if(!pwm_running[i])
                    {
                        nrf_gpiote_task_config(pwm_gpiote_channel[i], pwm_io_ch[i], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);  
                        pwm_running[i] = 1;
                        PWM_TIMER2->TASKS_CAPTURE[3] = 1;
                        if(PWM_TIMER2->CC[3] > new_capture) NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]] = 1;
                        PWM_TIMER2->CC[i-2] = new_capture;
                    }
                    else
                    {
                        while(1)
                        {
                            PWM_TIMER2->TASKS_CAPTURE[3] = 1;
                            if(safe_margins_present(PWM_TIMER2_CURRENT, old_capture) && safe_margins_present(PWM_TIMER2_CURRENT, new_capture)) break;
                        }
                        if((PWM_TIMER2_CURRENT >= old_capture && PWM_TIMER2_CURRENT < new_capture) || (PWM_TIMER2_CURRENT < old_capture && PWM_TIMER2_CURRENT >= new_capture))
                        {
                            NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]] = 1;
                        }
                        PWM_TIMER2->CC[i-2] = new_capture;
                    }                    
                }
            }
        }
    }
}
