
// Name: Nick Erickson & Jack Churchill
// Desciption: This code controls a motor, servo, and LEDs through a keypad
// Date : 10/31/2018
// Class: EGR 226 − 902
// Prof: S.  Zuidema


#include "msp.h"
#include <stdio.h>
#include "time.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


#define EN      BIT4      //Pin  P4.4    on any PORT
#define RS      BIT5      //Pin  P4.5    on any PORT
#define DATA    0x0F      //pins P4.0-3  on any PORT
#define CLEAR   0x01

#define Lower_Nibble(x)   P4->OUT = (P4->OUT & 0xF0) + (x & 0x0F)        // This macro if PORT(x) uses the lower 4 pins


// Systick and LCD Initialization functions
void SysTick_initialization(void);
void delay_ms(volatile uint32_t ms_delay);
void delay_us(volatile uint32_t us_delay);
void lcdInit ();                                // Clear LCD
void lcdClear();                                // Initialize LCD
void lcdTriggerEN();                            // Trigger Enable
void lcdWriteData(unsigned char data);          // Send Data (Characters)
void lcdWriteCmd (unsigned char  cmd);          // Send Commands
void lcdSetText(char * text, int x, int y);     // Write string
void lcdSetInt (int val,     int x, int y);     // Write integer
void ports(void);


//Interrupt Function
void Alarm_Button_Init(void);
void PORT3_IRQHandler(void);

void configRTC(void);


// global struct variable called now
struct
{
uint8_t sec;
uint8_t min;
uint8_t hour;
} now;
struct
{
uint8_t asec;
uint8_t amin;
uint8_t ahour;
} alrm;
uint8_t RTC_flag = 0;

volatile uint32_t timeout ;
char alarmStat[30] = "Off";
int ALARM = 0;

void main(void)
{
    char time[30];
    char alarm[30];
    char AMPM = 'A';
    char aAMPM = 'A';
    alrm.asec = 0;
    alrm.amin = 0;
    alrm.ahour = 12;

    ports();    //Initialize ports
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    SysTick_initialization();
    lcdInit();  // Initialize the LCD
    __disable_irq();//disable all interrupts during set-up
    Alarm_Button_Init();//call button interrupt set-up
    configRTC();
    NVIC_EnableIRQ(RTC_C_IRQn);
    NVIC_EnableIRQ(PORT3_IRQn);//interrupts are on port 1
    __enable_irq();//enable interrupts after set up

     lcdClear();


     while (1)
      {

         if (RTC_flag)
              {
                  if(now.hour >= 13)
                  {
                      AMPM = 'P';
                  }
                 else
                  {
                      AMPM = 'A';
                  }
                  RTC_flag = 0;
              }

              sprintf(time, "%d:%02d:%02d %cM",now.hour, now.min, now.sec, AMPM);
              sprintf(alarm, "%d:%02d %cM",alrm.ahour, alrm.amin, aAMPM);
              lcdSetText(time, 3, 0);
              lcdSetText("Alarm", 2, 1);
              lcdSetText(alarmStat, 8, 1);
              lcdSetText(alarm, 4, 2);



      }

}

void configRTC(void)
{
 RTC_C->CTL0 = 0xA510; //Write Code, IE on RTC Ready
 RTC_C->CTL13 = 0x0000;
 RTC_C->TIM0 = 59<<8 | 00;
 RTC_C->TIM1 = 2<<8 | 12;

}

void RTC_C_IRQHandler(void)
{
 now.sec = RTC_C->TIM0>>0 & 0x00FF;
 now.min = RTC_C->TIM0>>8 & 0x00FF;
 now.hour = RTC_C->TIM1>>0 & 0x00FF;

RTC_flag = 1;
 RTC_C->CTL0 = 0xA510;
}


// This function initializes all of the ports required for the keypad
void ports(void)
{
      P4->DIR  |= (EN + RS + DATA);   // Make pins outputs.          just change P4 for any other port keeping lower pins for data
      P4->OUT &=~(BIT4 | BIT5);   // Clear the EN and RS pins    just change P4 for any other port keeping BIT4 and BIT5 for E and RS
}



void Alarm_Button_Init()//function to initialize motor button pin & interrupt
{
    P3 -> SEL0 &= ~(BIT0 | BIT5 | BIT6 | BIT7);//GPIO set-up
    P3 -> SEL1 &= ~(BIT0 | BIT5 | BIT6 | BIT7);
    P3 -> DIR |= (BIT0 | BIT5 | BIT6 | BIT7);//input
    P3 -> REN |= (BIT0 | BIT5 | BIT6 | BIT7);//enable resistor
    P3 -> OUT |= (BIT0 | BIT5 | BIT6 | BIT7);//input defaults to '1'
    P3 -> IES |= (BIT0 | BIT5 | BIT6 | BIT7);//P1.6 interrupt triggers when it goes from high to low
    P3 -> IE |= (BIT0 | BIT5 | BIT6 | BIT7);//set interrupt on P1.7
    P3 -> IFG &= ~(BIT0 | BIT5 | BIT6 | BIT7);//clear flag
}

void PORT3_IRQHandler(void)//interrupt function definition
{

    if(P3 -> IFG & BIT0)//conditional to check if button on P1.6 has been pressed
    {
        printf("SET Alarm\n");
        delay_ms(50);
        P3 -> IFG &= ~BIT0;//clear flag
    }
    if(P3 -> IFG & BIT5)//conditional to check if button on P1.6 has been pressed
       {
            printf("SET TIME\n");
           delay_ms(50);
           P3 -> IFG &= ~BIT5;//clear flag
       }
    if(P3 -> IFG & BIT6)//conditional to check if button on P1.6 has been pressed
    {
        printf("ON/OFF/UP\n");
        if(alarmStat[1] == 'f')
        {
            alarmStat[0] = 'O';
            alarmStat[1] = 'n';
            alarmStat[2] = '\0';
            lcdClear();
        }
        if(alarmStat[1] == 'n')
        {
            alarmStat[0] = 'O';
            alarmStat[0] = 'f';
            alarmStat[0] = 'f';
            lcdClear();
        }
        delay_ms(50);
        P3 -> IFG &= ~BIT6;//clear flag
    }
    if(P3 -> IFG & BIT7)//conditional to check if button on P1.6 has been pressed
       {
            printf("SNOOZE/DOWN\n");
            if(ALARM)
                    {
                        alarmStat[30] = "Snooze";
                    }
           delay_ms(50);
           P3 -> IFG &= ~BIT7;//clear flag
       }
}


void SysTick_initialization(void){
 SysTick->CTRL  &= ~BIT0;            //clears enable to stop the counter
 SysTick->LOAD  = 0x00FFFFFF;        //sets the period... note: (3006600/1000 - 1) = 1ms
 SysTick->VAL   = 0;                 //clears the value
 SysTick->CTRL  = 0x00000005;        //enable SysTick, no uint8_terrupts
}
void delay_ms(uint32_t ms_delay){
 SysTick->LOAD = ms_delay*3000 - 1;             //counts up to delay
 SysTick->VAL  = 0;                             //starts counting from 0
 while((SysTick->CTRL & 0x00010000) == 0);     //wait until flag is set (delay number is reached)
}
void delay_us(uint32_t us_delay){
 SysTick->LOAD = us_delay*3 - 1;                //counts up to delay
 SysTick->VAL  = 0;                             //starts counting from 0
 while((SysTick->CTRL & 0x00010000) == 0);     //wait until flag is set (delay number is reached)
}
void lcdInit() {
    P4->OUT &=~ BIT5;   // Set RS to zero to write commands
    delay_ms(1);
    P4->OUT = 0x03;       // Start LCD
    lcdTriggerEN();
    delay_ms(5);
    P4->OUT = 0x03;       // Set one.
    lcdTriggerEN();
    delay_ms(5);
    P4->OUT = 0x03;       // Set two.
    lcdTriggerEN();
    delay_ms(5);
    P4->OUT = 0x03;       // Set three.
    lcdTriggerEN();
    delay_ms(1);
    P4->OUT = 0x02;       // Set up four bit mode Finally.
    lcdTriggerEN();

    lcdWriteCmd(0x28);  //2 is for 4bit mode 8 is for 2 lines F has to be 0 for 5 x 8 dots.
    delay_ms(1);
    lcdWriteCmd(0x08);
    delay_ms(1);
    lcdWriteCmd(0x01);
    delay_ms(1);
    lcdWriteCmd(0x06);
    delay_ms(1);
    lcdWriteCmd(0x0F);  // Make this into 0x0E and the cursor will stay on the screen.
                        // 0x0C is no cursor.
    delay_ms(5);
}
void lcdTriggerEN() {
    P4->OUT |=  EN;
    P4->OUT &= ~EN;
}
void lcdWriteData(unsigned char data) {
    P4->OUT |= BIT5;              // Set RS to one to write Data
    Lower_Nibble(data >> 4);    // Upper nibble
    delay_ms(1);
    lcdTriggerEN();
    delay_ms(1);
    Lower_Nibble(data);         // Lower nibble
    delay_ms(1);
    lcdTriggerEN();
    delay_ms(2);                // Delay >>> 47 us
}
void lcdWriteCmd(unsigned char cmd) {
    P4->OUT &= ~BIT5;             // Set RS to zero to write Command
    Lower_Nibble(cmd >> 4);     // Upper nibble
    delay_ms(1);
    lcdTriggerEN();
    delay_ms(1);
    Lower_Nibble(cmd);          // Lower nibble
    delay_ms(1);
    lcdTriggerEN();
    delay_ms(2);                // Delay > 1.5ms
}
void lcdSetText(char* text, int x, int y) {
    int i;
    if (x < 16) {
        x |= 0x80;      // Set LCD for first line write
        switch (y){
        case 0:
            x |= 0x00;  // Set LCD for first line write
            break;
        case 1:
            x |= 0x40;  // Set LCD for Second line write
            break;
        case 2:
            x |= 0x10;  // Set LCD for Third line write
            break;
        case 3:
            x |= 0x50;  // Set LCD for Fourth line write
            break;
        case 5:
            x |= 0x20;  // Set LCD for second line write reverse
            break;
        }
        lcdWriteCmd(x);
    }
    i = 0;
    while (text[i] != '\0') {
        lcdWriteData(text[i]);
        i++;
    }
}
void lcdSetInt(int val, int x, int y){
    char number_string[16];
    sprintf(number_string, "%d", val); // Convert the integer to character string
    lcdSetText(number_string, x, y);
}
void lcdClear() {
    lcdWriteCmd(CLEAR);
    delay_ms(10);
}

