#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

#include "timer.h"

#define PRESCALER   (84)
#define ARR_VALUE   (1000)

#define UPPER_BOUND_CHECK(num)      ((num) > 100.0f)
#define LOWER_BOUND_CHECK(num)      ((num) < 0.0f)

// 84_000_000
// freq = system_freq / ((prescaler - 1) * (arr - 1) )

void timer_setup(void)
{
    rcc_periph_clock_enable(RCC_TIM3);

    // High level config
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_DOWN);

    // Setup PWM mode
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM3, TIM_OC2, TIM_OCM_PWM1);

    // Enable PWM output
    timer_enable_counter(TIM3);
    timer_enable_oc_output(TIM3, TIM_OC1);
    timer_enable_oc_output(TIM3, TIM_OC2);

    // Setup frequency and resolution
    timer_set_prescaler(TIM3, PRESCALER - 1);
    timer_set_period(TIM3, ARR_VALUE - 1);
}

bool timer_pwm_set_duty_cycle(float duty_cycle)
{
    // duty cycle = (ccr / arr) * 100
    // ccr = (duty_cycle/100) * arr
    
    if (UPPER_BOUND_CHECK(duty_cycle) || LOWER_BOUND_CHECK(duty_cycle))
        return false;

    const float raw_value_ccr1 = (float)ARR_VALUE * (duty_cycle / 100.0f);
    const float raw_value_ccr2 = (float)ARR_VALUE * ((100.0f - duty_cycle) / 100.0f);

    timer_set_oc_value(TIM3, TIM_OC1, (uint32_t)raw_value_ccr1);
    timer_set_oc_value(TIM3, TIM_OC2, (uint32_t)raw_value_ccr2);

    return true;
} 