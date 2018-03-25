#include "timing.h"

#include <stdbool.h>



static bool _master_interrupt_was_enabled;

static void _timing_enter_critical_section(void)
{
	_master_interrupt_was_enabled = !MAP_Interrupt_disableMaster();
}

static void _timing_exit_critical_section(void)
{
	if(_master_interrupt_was_enabled)
	{
		MAP_Interrupt_enableMaster();
	}
}



void init_clocks(void)
{
	_timing_enter_critical_section();

	// Set up pins for high-frequency oscillator
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(
		GPIO_PORT_PJ,
		GPIO_PIN3 | GPIO_PIN2,
		GPIO_PRIMARY_MODULE_FUNCTION);

	// enables getMCLK, getSMCLK to know externally set frequencies
	CS_setExternalClockSourceFrequency(32000,48000000);

	/* Starting HFXT in non-bypass mode without a timeout. Before we start
	 * we have to change VCORE to 1 to support the 48MHz frequency */
	MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
	MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
	MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
	// false means that there are no timeouts set, will return when stable
	CS_startHFXT(false);

	/* Initializing MCLK to HFXT (effectively 48MHz) */
	MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

	// Initializing SMCLK to its maximum, which is 24MHz
	MAP_CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_2);

	_timing_exit_critical_section();
}



ClockFrequency get_frequency(ClockSignalID signal)
{
	if(signal == CS_ACLK) return CS_getACLK();
	else if(signal == CS_MCLK) return CS_getMCLK();
	else if(signal == CS_SMCLK) return CS_getSMCLK();
	else if(signal == CS_HSMCLK) return CS_getHSMCLK();
	else if(signal == CS_BCLK) return CS_getBCLK();
	else return 0xFFFFFFFF;
}



/*
 * parrotdelay and delay_spin_ms taken from ST7735.c
 *
 */

// delay function for testing
// which delays about 8.1*ulCount cycles
#ifdef __TI_COMPILER_VERSION__
  //Code Composer Studio Code
  void parrotdelay(unsigned long ulCount){
  __asm (  "pdloop:  subs    r0, #1\n"
      "    bne    pdloop\n");
}

#else
  //Keil uVision Code
  __asm void
  parrotdelay(unsigned long ulCount)
  {
    subs    r0, #1
    bne     parrotdelay
    bx      lr
  }
#endif

// Subroutine to wait 1 msec
// Inputs: n  number of 1 msec to wait
// Outputs: None
// Notes: ...
void delay_spin_ms(uint32_t n)
{
	while(n)
	{
		parrotdelay(5901);                  // 1 msec, tuned at 48 MHz
		n--;
	}
}






