
// Name: Nick Erickson & Jack Churchill
// Description: This code displays the temperature every half second in both Celsius and Fahrenheit
// Date : 10/31/2018
// Class: EGR 226 − 902
// Prof: S.  Zuidema



#include "msp.h"
#include <stdio.h>
#include "time.h"
#include <stdint.h>
void delaysetup(void);
void delayms(uint16_t delay);
void ADC14_init (void);
void SysTick_Init_interrupt(void);
void SysTick_Handler(void);

volatile uint32_t timeout;

main(void)
{
static volatile uint16_t result;
float CTemp;
float FTemp;
float Vref;

 ADC14_init ( );
 WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // stop watchdog timer

 __disable_irq();
 SysTick_Init_interrupt();
 __enable_irq();

while(1)
 {
ADC14->CTL0 |= 1; //start conversation
while(!ADC14->IFGR0); //wait for conversation to complete
result = ADC14->MEM[0]; // get the value from the ADC
Vref = result*(3.3/16383);
Vref = Vref*1000;
CTemp = (Vref-500)/100;
FTemp = ((CTemp*9)/5)+32;
if(timeout)
{
printf("Value is:\n\t%f Degrees Fahrenheit\n\t%f Degrees Celsius\n",FTemp, CTemp);
timeout = 0;
}
}
}
void SysTick_Init_interrupt(void)
{
SysTick-> CTRL = 0;
SysTick-> LOAD = 1500000;
SysTick-> VAL = 0;
SysTick-> CTRL = 0x00000007;
}
void SysTick_Handler(void)
{
    timeout = 1;
}
void ADC14_init (void)
{
P5SEL0 |= 0X20; // configure pin 5.5 for A0 input
P5SEL1 |= 0X20;
 ADC14->CTL0 &=~ 0x00000002; // disable ADC14ENC during configuration
ADC14->CTL0 |= 0x04400110; // S/H pulse mode, SMCLK, 16 sample clocks
ADC14->CTL1 = 0x00000030; // 14 bit resolution
ADC14->CTL1 |= 0x00000000; // Selecting ADC14CSTARTADDx mem0 REGISTER
ADC14->MCTL[0] = 0x00000000; // ADC14INCHx = 0 for mem[0]
// ADC14->MCTL[0] = ADC14_MCTLN_INCH_0;
ADC14->CTL0 |= 0x00000002; // enable ADC14ENC, starts the ADC after configuration
}
