/**
 * @file    main.c
 * @brief   Formal implementation of a breathing LED using eFlexPWM on i.MX RT1040.
 * Target Pin: GPIO_AD_B0_10 (PWM1, Submodule 3, Channel A)
 */

#include "fsl_pwm.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_PWM_BASE          PWM1
#define DEMO_PWM_SUBMODULE     kPWM_Module_3
#define DEMO_PWM_CHANNEL       kPWM_PwmA
#define DEMO_PWM_FREQUENCY     1000U
#define ANIMATION_DELAY_MS     25U

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * @brief Performs manual hardware overrides to bypass safety interlocks.
 * @note  As per RM Section 53.2.3, PWM outputs may be inactive in certain modes.
 */
static void PWM_ApplyHardwareFixes(void)
{
    /* Clear all fault flags by writing 1s to the Fault Status Register */
    DEMO_PWM_BASE->FSTS |= 0x000FU;

    /* Disable fault mapping: Ensures signal isn't blocked by floating fault pins */
    DEMO_PWM_BASE->SM[DEMO_PWM_SUBMODULE].DISMAP[0] = 0x0000U;

    /* Manually enable output gate for Submodule 3, Channel A (Bit 6) */
    DEMO_PWM_BASE->OUTEN |= (1U << (DEMO_PWM_SUBMODULE * 2 + 0));
}

int main(void)
{
    pwm_config_t pwmConfig;
    pwm_signal_param_t pwmSignal;
    uint32_t pwmClockHz;
    uint8_t dutyCycle = 0U;
    int8_t step = 1;

    /* Initialize system hardware */
    BOARD_ConfigMPU();
    BOARD_InitBootPins();   /* Ensure GPIO_AD_B0_10 is muxed to ALT2 */
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    /* Enable peripheral clock and get frequency */
    CLOCK_EnableClock(kCLOCK_Pwm1);
    pwmClockHz = CLOCK_GetFreq(kCLOCK_IpgClk);

    /* Configure PWM Submodule */
    PWM_GetDefaultConfig(&pwmConfig);
    pwmConfig.reloadLogic = kPWM_ReloadPwmFullCycle;
    pwmConfig.enableDebugMode = true; /* Prevents PWM freeze during debugging */
    pwmConfig.enableWait      = true; /* Prevents PWM freeze in low-power wait */

    if (PWM_Init(DEMO_PWM_BASE, DEMO_PWM_SUBMODULE, &pwmConfig) != kStatus_Success)
    {
        PRINTF("PWM Initialization Failed\r\n");
        return -1;
    }

    /* Define PWM Signal properties */
    pwmSignal.pwmChannel       = DEMO_PWM_CHANNEL;
    pwmSignal.level            = kPWM_HighTrue;
    pwmSignal.dutyCyclePercent = 0U;
    pwmSignal.pwmchannelenable = true;

    /* Initialize PWM signal on Submodule 3 */
    PWM_SetupPwm(DEMO_PWM_BASE, DEMO_PWM_SUBMODULE, &pwmSignal, 1U, kPWM_EdgeAligned, DEMO_PWM_FREQUENCY, pwmClockHz);

    /* Apply safety bypasses and fault clearing */
    PWM_ApplyHardwareFixes();

    /* Load initial values and start the timer */
    PWM_SetPwmLdok(DEMO_PWM_BASE, (1U << DEMO_PWM_SUBMODULE), true);
    PWM_StartTimer(DEMO_PWM_BASE, (1U << DEMO_PWM_SUBMODULE));

    PRINTF("PWM Breathing Animation Started...\r\n");

    while (1)
    {
        /* Update the duty cycle value */
        PWM_UpdatePwmDutycycle(DEMO_PWM_BASE, DEMO_PWM_SUBMODULE, DEMO_PWM_CHANNEL, kPWM_EdgeAligned, dutyCycle);

        /* Set Load OK bit to transfer buffer values to active registers */
        PWM_SetPwmLdok(DEMO_PWM_BASE, (1U << DEMO_PWM_SUBMODULE), true);

        /* Animation Logic */
        if (dutyCycle >= 100U) step = -1;
        if (dutyCycle <= 0U)   step = 1;
        dutyCycle += step;

        /* Blocking delay for visible transition */
        SDK_DelayAtLeastUs(ANIMATION_DELAY_MS * 1000U, CLOCK_GetFreq(kCLOCK_CpuClk));
    }
}
