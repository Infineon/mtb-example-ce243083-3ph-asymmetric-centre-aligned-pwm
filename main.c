/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the three phase asymmetric center
*              aligned PWM Template for ModusToolbox.
*
* Related Document: See README.md
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#include "cybsp.h"
#include "cy_utils.h"
#include "cy_retarget_io.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define PWM_COMPARE_MAX (2400u)
#define PWM_COMPARE_25PCT ((PWM_COMPARE_MAX * 25u) / 100u)
#define PWM_COMPARE_75PCT ((PWM_COMPARE_MAX * 75u) / 100u)
#define PWM_PERIOD_US (50u)
#define DUTY_UPDATE_US (2500u)
#define DUTY_UPDATE_TICKS (DUTY_UPDATE_US / PWM_PERIOD_US)

/*******************************************************************************
* Global Variables
*******************************************************************************/
bool duty_is_25pct = true;
uint16_t irq_count = 0u;

/*******************************************************************************
* Function Name: NVIC_Config
********************************************************************************
* Summary:
* Configures the NVIC for the CCU8 period match interrupt (IRQ25). Sets the
* interrupt priority and enables the IRQ.
*
* Parameters:
*  none
*
* Return:
*  none
*
*******************************************************************************/
void NVIC_Config(void)
{
    /*Set Priority for IRQ*/
    NVIC_SetPriority(IRQ25_IRQn, 1u);
    /*Enable the Interrupt*/
    NVIC_EnableIRQ(IRQ25_IRQn);
}

/*******************************************************************************
* Function Name: IRQ25_Handler
********************************************************************************
* Summary:
* Interrupt handler for the CCU8 period match event (IRQ25). Updates the PWM
* compare values for all three phases (U, V, W) and toggles the duty cycle
* between 25% and 75% every DUTY_UPDATE_TICKS interrupts. Triggers a shadow
* transfer to apply the new compare values.
*
* Parameters:
*  none
*
* Return:
*  none
*
*******************************************************************************/
void IRQ25_Handler(void)
{
    Cy_GPIO_SetOutputLow(Debug_pin_PORT, Debug_pin_PIN);

    uint16_t compare_value = duty_is_25pct ? PWM_COMPARE_25PCT : PWM_COMPARE_75PCT;

    /*
     * Alternate duty cycle between 25% and 75% on each interrupt:
     * - CH1 compare controls the up-count edge.
     * - CH2 compare controls the down-count edge.
     */
    Cy_CCU8_SLICE_SetTimerCompareMatchChannel1(PWM_U_HW, compare_value);
    Cy_CCU8_SLICE_SetTimerCompareMatchChannel1(PWM_V_HW, compare_value);
    Cy_CCU8_SLICE_SetTimerCompareMatchChannel1(PWM_W_HW, compare_value);

    Cy_CCU8_SLICE_SetTimerCompareMatchChannel2(PWM_U_HW, compare_value);
    Cy_CCU8_SLICE_SetTimerCompareMatchChannel2(PWM_V_HW, compare_value);
    Cy_CCU8_SLICE_SetTimerCompareMatchChannel2(PWM_W_HW, compare_value);

    irq_count++;
    if (irq_count >= DUTY_UPDATE_TICKS)
    {
        irq_count = 0u;
        duty_is_25pct = !duty_is_25pct;
    }

    /* Enable shadow transfer for slice 0,1,2 for CCU80 Kernel. */
    CCU80->GCSS |= (uint32_t)(CCU8_GCSS_S0SE_Msk | CCU8_GCSS_S1SE_Msk | CCU8_GCSS_S2SE_Msk);

    Cy_GPIO_SetOutputLow(Debug_pin_PORT, Debug_pin_PIN);

}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* Entry point of the application. Initializes the board peripherals and
* retarget-IO UART, prints the startup banner with PWM pin assignments,
* configures the NVIC, and starts all three CCU8 PWM slices synchronously
* via the global CCU80 start trigger.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    cy_retarget_io_init(CYBSP_DEBUG_UART_HW);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("** PSOC C1 : 3-Phase Asymmetric Centre-Aligned PWM **\r\n\n");
    printf("Phase U : High-side = P0.0 | Low-side = P0.1\r\n");
    printf("Phase V : High-side = P0.6 | Low-side = P0.7\r\n");
    printf("Phase W : High-side = P0.8 | Low-side = P0.9\r\n");

    NVIC_Config();

    /* Enable Global Start Control CCU80 */
    Cy_SCU_SetCcuTriggerHigh(SCU_GENERAL_CCUCON_GSC80_Msk);

    for (;;)
    {
    }
}

/* [] END OF FILE */
