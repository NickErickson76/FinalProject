
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
#define OFF     1
#define ON      2
#define SNOOZE  3
#define Lower_Nibble(x)   P4->OUT = (P4->OUT & 0xF0) + (x & 0x0F)        // This macro if PORT(x) uses the lower 4 pins

#define BUFFER_SIZE 100
char INPUT_BUFFER[BUFFER_SIZE];

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
void writeOutput(char *string); // write output charactrs to the serial port
void readInput(char* string); // read input characters from INPUT_BUFFER that are valid
void setupSerial(void);
//Interrupt Function
void Alarm_Button_Init(void);
void PORT3_IRQHandler(void);
void LEDPWM(int threesec);
void ALARMON(int loud);
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
    uint8_t smin;
    uint8_t shour;
} set;

struct
{
uint8_t amin;
uint8_t ahour;
} alrm;

uint8_t RTC_flag = 0;
uint8_t storage_location = 0; // used in the interrupt to store new data
uint8_t read_location = 0; // used in the main application to read valid data that hasn't been read yet

volatile uint32_t timeout ;
int ALARM = 0;
volatile int ONOFF = OFF;
int tspeed = 0;
enum states{           // Names the states
    CLOCK,
    ALARMSET,
    TIMESET,
    ALRMON
};

enum astates{
    aOFF,
    alHOUR,
    alMIN
};
enum tstates{
    tOFF,
    tHOUR,
    tMIN
};

enum states state = CLOCK;
enum astates atime = aOFF;
enum tstates t_time = tOFF;
int PM = 0;
int aPM = 0;
char aAMPM = 'A';
char AMPM = 'A';
int LIGHT = OFF;
int i, j;
int threesec = 0;
void main(void)
{
    char string[BUFFER_SIZE]; // Creates local char array to store incoming serial commands
    char time[30];
    char alarm[30];
    char uhour[30];
    char umin[30];
    char usec[30];

    LEDPWM(0);
    alrm.amin = 0;
    alrm.ahour = 5;
    ports();    //Initialize ports
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    SysTick_initialization();
    lcdInit();  // Initialize the LCD
    __disable_irq();//disable all interrupts during set-up
    Alarm_Button_Init();//call button interrupt set-up
    setupSerial();
    configRTC();
    NVIC_EnableIRQ(RTC_C_IRQn);
    NVIC_EnableIRQ(PORT3_IRQn);//interrupts are on port 3
    __enable_irq();//enable interrupts after set up

     lcdClear();


     while (1)
      {
         switch(state){
         case CLOCK:
             /*
             readInput(string);

                         if(string[0] != '\0'){

                         if(strcmp(string, "SETTIME"))
                                 {
                                  strcpy(uhour, &string[8]);
                                  RTC_C->TIM1 = atoi(uhour);
                                  strcpy(umin, &string[10]);
                                  RTC_C->TIM0 = atoi(umin)<<8;
                                  strcpy(usec, &string[12]);
                                  RTC_C->TIM0 = atoi(usec);
                                  AMPM = string[14];
                                 }
                         if(strcmp(string, "SETALARM"))
                         {
                             strcpy(uhour, &string[9]);
                             alrm.ahour = atoi(uhour);
                             strcpy(umin, &string[11]);
                             alrm.amin = atoi(umin);
                             aAMPM = string[13];
                         }
                         }
                         */
             ALARMON(0);
             if((now.hour <= alrm.ahour) && (PM == aPM) && (ONOFF == ON | ONOFF == SNOOZE) && 5 == (abs(alrm.amin-now.min)))
                          {
                              LIGHT = ON;

                          }
         if (RTC_flag)
              {
                  if(now.hour >= 13 && PM == 0)
                  {
                      now.hour = 1;
                      AMPM = 'P';
                      PM = 1;
                  }
                 if(now.hour >= 13 && PM == 1)
                  {
                      now.hour = 1;
                      AMPM = 'A';
                      PM = 0;
                  }
                 if(LIGHT == ON)
                 {
                      i++;
                      if(i == 3)
                      {
                      threesec++;
                      LEDPWM(threesec);
                      i = 0;
                      }
                 }

                  RTC_flag = 0;
              }



             if(now.hour == alrm.ahour && now.min == alrm.amin && PM == aPM && (ONOFF == ON | ONOFF == SNOOZE))
             {
                 state = ALRMON;
                 ALARM = 1;
             }





              sprintf(time, "%d:%02d:%02d %cM",now.hour, now.min, now.sec, AMPM);
              sprintf(alarm, "%d:%02d %cM",alrm.ahour, alrm.amin, aAMPM);


              lcdSetText(time, 3, 1);
              delay_ms(30);
              lcdSetText("Alarm", 2, 2);
              delay_ms(30);

              if(ONOFF == OFF)
              lcdSetText("OFF", 10, 2);

              if(ONOFF == ON)
              lcdSetText("ON", 10, 2);

              if(ONOFF == SNOOZE)
              lcdSetText("SNOOZE", 10, 2);

              delay_ms(50);
              lcdSetText(alarm, 4, 3);
              delay_ms(30);
             break;

         case ALARMSET:

            sprintf(alarm, "%d:%02d %cM",alrm.ahour, alrm.amin, aAMPM);

            delay_ms(10);
            if(atime == alHOUR)
                       {
                           lcdSetText("Set Alarm Hour", 1, 1);
                       }
            if(atime == alMIN)
                       {
                           lcdSetText("Set Alarm Min", 1, 1);
                       }
            delay_ms(50);
            lcdSetText(alarm, 4, 2);
            delay_ms(50);

             break;
         case TIMESET:
             delay_ms(10);
             sprintf(time, "%d:%02d %cM",set.shour, set.smin, AMPM);
             if(t_time == tHOUR)
             {
                       lcdSetText("Set Time Hour", 1, 1);
             }
             if(t_time == tMIN)
             {
                       lcdSetText("Set Time Min", 1, 1);
             }
             delay_ms(50);
             lcdSetText(time, 4, 2);
             delay_ms(50);

             break;
         case ALRMON:
             ALARMON(99);
                 if (RTC_flag)
                      {
                          if(now.hour >= 13 && PM == 0)
                          {
                              now.hour = 1;
                              AMPM = 'P';
                              PM = 1;
                          }
                         if(now.hour >= 13 && PM == 1)
                          {
                              now.hour = 1;
                              AMPM = 'A';
                              PM = 0;
                          }
                          RTC_flag = 0;
                      }
                     sprintf(time, "%d:%02d:%02d %cM",now.hour, now.min, now.sec, AMPM);
                     sprintf(alarm, "%d:%02d %cM",alrm.ahour, alrm.amin, aAMPM);


                     lcdSetText(time, 3, 1);
                     delay_ms(30);
                     lcdSetText("Alarm", 2, 2);
                     delay_ms(30);

                     if(ONOFF == OFF)
                     lcdSetText("OFF", 10, 2);

                     if(ONOFF == ON)
                     lcdSetText("ON", 10, 2);

                     if(ONOFF == SNOOZE)
                     lcdSetText("SNOOZE", 10, 2);

                     delay_ms(50);
                     lcdSetText(alarm, 4, 3);
                     delay_ms(30);

         break;
      }
      }
}

void configRTC(void)
{
 RTC_C->CTL0 = 0xA510; //Write Code, IE on RTC Ready
 RTC_C->CTL13 = 0x0000;
 RTC_C->TIM0 = 59<<8 | 50;
 RTC_C->TIM1 = 2<<8 | 4;

}

void RTC_C_IRQHandler(void)
{
    /*
    if(tspeed == 1)
    {
        if(now.sec<=59){
            now.sec = (RTC_C->TIM0>>0 & 0x00FF) + 1;
            if(now.sec == 60){
                now.sec = 0;
                now.min = (RTC_C->TIM0>>8 & 0x00FF) + 1;
                if(now.min==60){
                    now.min = 0;
                    now.hour = (RTC_C->TIM1>>0 & 0x00FF) + 1;
                    if(now.hour >= 13 && PM == 0)
                                               {
                                                   PM = 1;
                                                   AMPM = 'P';
                                                   now.hour = 1;
                                               }
                                               if(now.hour >= 13 && PM == 1)
                                               {
                                                   PM = 0;
                                                   AMPM = 'A';
                                                   now.hour = 1;
                                               }
                    }
                }
            }
        RTC_C->PS1CTL &= ~(BIT0);
        }




    if(tspeed == 2)
    {
        alrm.amin++;
        if(alrm.amin >= 60)
        {
            alrm.amin = 0;
            alrm.ahour++;
            if(alrm.ahour >= 13 && aPM == 0)
            {
                aPM = 1;
                aAMPM = 'P';
                alrm.ahour = 1;
            }
            if(alrm.ahour >= 13 && aPM == 1)
            {
                aPM = 0;
                aAMPM = 'A';
                alrm.ahour = 1;
            }

        }
    }
    */
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
    P3 -> SEL0 &= ~(BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//GPIO set-up
    P3 -> SEL1 &= ~(BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);
    P3 -> DIR |= (BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//input
    P3 -> REN |= (BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//enable resistor
    P3 -> OUT |= (BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//input defaults to '1'
    P3 -> IES |= (BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//P1.6 interrupt triggers when it goes from high to low
    P3 -> IE |= (BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//set interrupt on P1.7
    P3 -> IFG &= ~(BIT0 | BIT2 | BIT3 | BIT5 | BIT6 | BIT7);//clear flag
    P2->SEL0 |= (BIT4);
    P2->SEL1 &= ~(BIT4);
    P2->DIR |= (BIT4);
    P5->SEL0 |= BIT6;
    P5->SEL1 &=~ BIT6;
    P5->DIR |= BIT6;
}

void PORT3_IRQHandler(void)//interrupt function definition
{
    /*
    tspeed = 0;
    if(P3 -> IFG & BIT2)
        {
            tspeed = 1;
            P3 -> IFG &= ~BIT2;
        }
    if(P3 -> IFG &= BIT3)
        {
           tspeed = 2;
           P3 -> IFG &= ~BIT3;

        }
*/
    if(P3 -> IFG & BIT0) //Interrupt for setting the alarm time
    {

        lcdClear();
        printf("SET Alarm\n");
        state = ALARMSET;
        if(atime == aOFF)
        {
            atime = alHOUR;
            lcdClear();
        }
        else if(atime == alHOUR)
        {
            atime = alMIN;
            lcdClear();
        }
        else if(atime == alMIN)
        {
            atime = aOFF;
            state = CLOCK;
            lcdClear();
        }
        delay_ms(50);
        P3 -> IFG &= ~BIT0;//clear flag
    }
    if(P3 -> IFG & BIT5) // Interrupt for setting the time
       {

            lcdClear();
            printf("SET TIME\n");
            state = TIMESET;
            if(t_time == tOFF)
                   {
                      set.smin = now.min;
                      set.shour = now.hour;
                       t_time = tHOUR;
                       lcdClear();
                   }
                   else if(t_time == tHOUR)
                   {
                       t_time = tMIN;
                       lcdClear();
                   }
                   else if(t_time == tMIN)
                   {
                       RTC_C->TIM0 = set.smin<<8;
                       RTC_C->TIM1 = set.shour;
                       t_time = tOFF;
                       state = CLOCK;
                       lcdClear();
                   }
                   delay_ms(50);
           P3 -> IFG &= ~BIT5;//clear flag
       }

    if(P3 -> IFG & BIT6)//conditional to check if button on P1.6 has been pressed
    {
        lcdClear();
        printf("ON/OFF/UP\n");
        if(state == CLOCK | state == ALRMON)
        {


        if(ONOFF == OFF)
        {
            ONOFF = ON;
            printf("Alarm On");
            if(ALARM == 0)
            {
                ALARMON(0);
            }

        }
        else if(ONOFF == ON)
        {
            ONOFF = OFF;
            state = CLOCK;
            ALARM = 0;
        }
        }

        if(state == ALARMSET)
        {
            if(atime == alHOUR)
            {
                alrm.ahour++;
                if(alrm.ahour >= 13 && aPM == 0)
                {
                    aPM = 1;
                    aAMPM = 'P';
                    alrm.ahour = 1;
                }
                if(alrm.ahour >= 13 && aPM == 1)
                {
                    aPM = 0;
                    aAMPM = 'A';
                    alrm.ahour = 1;
                }
            }

            else if(atime == alMIN)
            {
                alrm.amin++;
                if(alrm.amin >= 59)
                {
                    alrm.amin = 0;
                }
            }

        }

        if(state == TIMESET)
        {
                        if(t_time == tHOUR)
                        {
                            set.shour++;
                            if(set.shour >= 13 && PM == 0)
                            {
                                PM = 1;
                                AMPM = 'P';
                                set.shour = 1;
                            }
                            if(set.shour >= 13 && PM == 1)
                            {
                                PM = 0;
                                AMPM = 'A';
                                set.shour = 1;
                            }
                        }
                        else if(t_time == tMIN)
                        {
                            set.smin++;
                            if(set.smin >= 59)
                            {
                                set.smin = 0;
                            }
                        }


        }
        if(ONOFF == SNOOZE)
        {
            ALARM = 0;
            ONOFF = OFF;
            alrm.amin = alrm.amin - 10;

        }
        lcdClear();
        delay_ms(50);
        lcdClear();
        P3 -> IFG &= ~BIT6;//clear flag
    }
    if(P3 -> IFG & BIT7)//conditional to check if button on P1.6 has been pressed
       {
            printf("SNOOZE/DOWN\n");

                if(state == ALARMSET)
                 {
                     if(atime == alHOUR)
                     {
                         alrm.ahour--;
                         if(alrm.ahour <= 1 && aPM == 0)
                         {
                             aPM = 1;
                             aAMPM = 'P';
                             alrm.ahour = 12;
                         }
                         if(alrm.ahour <= 1 && aPM == 1)
                         {
                             aPM = 0;
                             aAMPM = 'A';
                             alrm.ahour = 12;
                         }
                     }
                     else if(atime == alMIN)
                     {
                         alrm.amin = alrm.amin - 1;
                         if(alrm.amin <= 0)
                         {
                             alrm.amin = 59;
                         }
                     }
            }
                if(state == TIMESET)
                                 {
                                     if(t_time == tHOUR)
                                     {
                                         set.shour--;
                                         if(set.shour <= 1 && PM == 0)
                                         {
                                             PM = 1;
                                             AMPM = 'P';
                                             set.shour = 12;
                                         }
                                         if(set.shour <= 1 && PM == 1)
                                         {
                                             PM = 0;
                                             AMPM = 'A';
                                             set.shour = 12;
                                         }
                                     }
                                     else if(t_time == tMIN)
                                     {
                                         set.smin--;
                                         if(set.smin <= 0)
                                         {
                                             set.smin = 59;
                                         }
                                     }
                            }

                if(state == ALRMON)
                {
                    ALARMON(0);
                    ONOFF = SNOOZE;
                    alrm.amin = alrm.amin + 10;
                    state = CLOCK;
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
    lcdWriteCmd(0x0C);  // Make this into 0x0E and the cursor will stay on the screen.
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


void EUSCIA0_IRQHandler(void)
{
    if (EUSCI_A0->IFG & BIT0)  // Interrupt on the receive line
    {
        INPUT_BUFFER[storage_location] = EUSCI_A0->RXBUF; // store the new piece of data at the present location in the buffer
        EUSCI_A0->IFG &= ~BIT0; // Clear the interrupt flag right away in case new data is ready
        storage_location++; // update to the next position in the buffer
        if(storage_location == BUFFER_SIZE) // if the end of the buffer was reached, loop back to the start
            storage_location = 0;
    }
}

void readInput(char *string)
{
    int i = 0;  // Location in the char array "string" that is being written to
    // One of the few do/while loops I've written, but need to read a character before checking to see if a \n has been read
    do
    {
        // If a new line hasn't been found yet, but we are caught up to what has been received, wait here for new data
        while(read_location == storage_location && INPUT_BUFFER[read_location] != '\n');
        string[i] = INPUT_BUFFER[read_location];  // Manual copy of valid character into "string"
        INPUT_BUFFER[read_location] = '\0';
        i++; // Increment the location in "string" for next piece of data
        read_location++; // Increment location in INPUT_BUFFER that has been read
        if(read_location == BUFFER_SIZE)  // If the end of INPUT_BUFFER has been reached, loop back to 0
            read_location = 0;
    }
    while(string[i-1] != '\n'); // If a \n was just read, break out of the while loop
    string[i-1] = '\0'; // Replace the \n with a \0 to end the string when returning this function
}
void writeOutput(char *string)
{
    int i = 0;  // Location in the char array "string" that is being written to
    while(string[i] != '\0') {
        EUSCI_A0->TXBUF = string[i];
        i++;
        while(!(EUSCI_A0->IFG & BIT1));
    }
}

void setupSerial()
{
    P1->SEL0 |=  (BIT2 | BIT3); // P1.2 and P1.3 are EUSCI_A0 RX
    P1->SEL1 &= ~(BIT2 | BIT3); // and TX respectively.
    EUSCI_A0->CTLW0  = BIT0; // Disables EUSCI. Default configuration is 8N1
    EUSCI_A0->CTLW0 |= BIT7; // Connects to SMCLK BIT[7:6] = 10
    //EUSCI_A0->CTLW0 |= (BIT(15)|BIT(14)|BIT(11));  //BIT15 = Parity, BIT14 = Even, BIT11 = Two Stop Bits
    // Baud Rate Configuration
    // 3000000/(16*115200) = 1.628  (3 MHz at 115200 bps is fast enough to turn on over sampling (UCOS = /16))
    // UCOS16 = 1 (0ver sampling, /16 turned on)
    // UCBR  = 1 (Whole portion of the divide)
    // UCBRF = .628 * 16 = 10 (0x0A) (Remainder of the divide)
    // UCBRS = 3000000/115200 remainder=0.04 -> 0x01 (look up table 22-4)
    EUSCI_A0->BRW = 19;  // UCBR Value from above
    EUSCI_A0->MCTLW = 0xAA81; //UCBRS (Bits 15-8) & UCBRF (Bits 7-4) & UCOS16 (Bit 0)
    EUSCI_A0->CTLW0 &= ~BIT0;  // Enable EUSCI
    EUSCI_A0->IFG &= ~BIT0;    // Clear interrupt
    EUSCI_A0->IE |= BIT0;      // Enable interrupt
    NVIC_EnableIRQ(EUSCIA0_IRQn);
}

void ALARMON(int loud)
{
      TIMER_A0->CCR[0] = 30000-1; // PWM Period
      TIMER_A0->CCTL[1] = OUTMOD_7; // TA1CCR1 output mode = reset/set
      TIMER_A0->CCR[1] = (loud*300); // TA1CCR1 PWM duty cycle
      TIMER_A0->CTL = TASSEL_2 | MC_1 | TACLR; // SMCLK, Up Mode, Clear
}

void LEDPWM(int threesec)
{
    TIMER_A2->CCR[1] = 30000-1; // PWM Period
    TIMER_A2->CCTL[1] = OUTMOD_7; // TA1CCR1 output mode = reset/set
    TIMER_A2->CCR[1] = (threesec*300); // TA1CCR1 PWM duty cycle
    TIMER_A2->CTL = TASSEL_2 | MC_1 | TACLR; // SMCLK, Up Mode, Clear
}
