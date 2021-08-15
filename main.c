// Daniel Tregea
// Microwave Simulation
#include <hidef.h>      /* common defines and macros */
#include <mc9s12dg256.h>     /* derivative information */
#pragma LINK_INFO DERIVATIVE "mc9s12dg256b"

#include "main_asm.h" /* interface to the assembly module */
char * newLine = "\n";
int i,j, hour = 0, minutes = 0, seconds = 0, open = 0, on, timeDigit, wait = 1;
int ticks1 = 0, ticks3 = 0, ticks5 = 0, clicks_per_ms = 24000, powerValue, temp, turnSpeed, cook = 0;

int time[] = {
  0,0,0,0
};

// Print to MiniIde
void printLine(char * line){
  i = 0;
 while(line[i] !=0){
  outchar0(line[i]);
  i = i + 1;
 }
 i = 0;
 while(newLine[i] !=0){
  outchar0(newLine[i]);
  i = i + 1;
 }
}

// Print to the LCD
void printLCD(char * line){
  type_lcd(line);
}

// Get user time input
void getTime(){
 i = 0;
 timeDigit = 16;
  while(timeDigit != 15){
   timeDigit = getkey();
   wait_keyup();
   if(i == 0){  // clear "Enter time" message on first iteration
    clear_lcd();
   }
    // if digits 0-9 entered, move all digits in time array up one index
    // to later be displayed as a 4 digit number on LCD
    if(timeDigit < 10){
      for(j = 3; j >= 0; j--){
        time[j] = time[j - 1];
      }
      time[0] = timeDigit;
      i++;
    } else if (timeDigit == 15){
    } else{
     printLine("Invalid key"); 
    }
    set_lcd_addr(0x00);
    write_int_lcd(time[3] * 10 + time[2]);
    printLCD(" M");
    set_lcd_addr(0x08);
    write_int_lcd(time[1] * 10 + time[0]);
    set_lcd_addr(0x0E);
    printLCD("S");
    set_lcd_addr(0x40);
    printLCD("#=DONE");
 }
}

// Convert user data to time
void clockTimeConvert(){
  seconds = 0; // reset from last run
  minutes = 0;
  hour = 0;
  minutes = time[3] * 10 + time[2];
  seconds = time[1] * 10 + time[0];
  if(seconds > 59){
   seconds -= 60;
   minutes++; 
  }
  if(minutes > 59){
   hour = 1;
   minutes = minutes - 60;
  }
  // Time set. Clear user input
  for(j = 0; j < 4; j++){
      time[j] = 0;
  }
}

// Print the countdown to the LCD
void printTime(){
 set_lcd_addr(0x00);
 write_int_lcd(hour);
 write_int_lcd(minutes);
 write_int_lcd(seconds); 
}

// Turn the microwave chamber light on
void lightOn(){
  PORTE = PORTE | 0x10;
}

// Turn the microwave chamber light off
void lightOff(){
  PORTE = PORTE & 0xEF;
}

// Close the microwave door
void lock(){
  set_servo76(2000);
  open = 0;
  lightOff();
  printLine("Microwave door closed");
}

// Open the microwave door
void unlock(){
  set_servo76(4000);
  open = 1;
  lightOn();
  printLine("Microwave door opened");
}

// Sound the microwave alarm
void alarm(){
 led_enable();
  for(j = 0; j< 4; j++){
     PORTB = ~PORTB;
     sound_init();
     sound_on();
     ms_delay(500);
     sound_off();
     ms_delay(500);
  }
  led_disable(); 
}

// Three second timer for end display message
void interrupt 9 threeSecondTimer(){
  TFLG1 = TFLG1 | 0x02;
  TC1 = TC1 + clicks_per_ms;
  ticks3++;
  if (ticks3 == 3000){
   wait = 0;
   ticks3 = 0; 
  }
  
}

// Five second timer for temperature update
void interrupt 8 fiveSecondTimer(){
 TFLG1 = TFLG1 | 0x01;
 TC0 = TC0 + clicks_per_ms;
 ticks5++;
 if(ticks5 == 5000){
  if(on){
    if(temp < 200){
      temp = temp + powerValue;
     }
  }
  ticks5 = 0;
 }
}

// Alarm beep
void interrupt 13 beep(){
 tone(1439);
}

// One second timer for countdown
void interrupt 15 oneSecondTimer(){ // one second timer
   TFLG1 = TFLG1 | 0x80;                       
   TC7 = TC7 + clicks_per_ms;                  
   ticks1++;
   if(ticks1 == 1000){
    // Countdown if microwave is in operation
    if(on){
     seconds--;
     if(seconds == -1){
       if(minutes == 0 && hour == 0){
         seconds = 0;
         on = 0; // Turn off if no time remaining
       }else{
         seconds = 59;
         minutes--;
         if(minutes == -1){
           if(hour == 1){
           minutes = 59;
           hour = 0;
         }
        }
       }
     }
    }
    ticks1 = 0;
   } 
}

// switch interrupt for turning off the microwave or opening / closing the door
void interrupt 25 switchInterrupt(){ 
  if(SW5_down()){ // Cancel Cooking or timing
    if(on){
      printLine("Operation Aborted");
      on = 0;
    }
    if(wait){ // End 3 second end message early
     wait = 0; 
    }
  }
  if(SW4_down()){
   ms_delay(20); // debounce handler
    if(on && cook){
     printLine("Cannot open while microwave is cooking");
    } else {
     if(open){
      lock(); 
     } else {
      unlock(); 
     }
    }
  }
  PIFH = 0xFF;
}

void main(void) {
 PLL_init();
 DDRB = 0xFF;
 DDRH = 0xFF;
 DDRE = 0xFF;
 lightOff();
 led_disable();
 seg7_disable();
 SW_enable();
 keypad_enable();
 lcd_init();
 motor0_init();
 servo76_init();
 set_servo76(3000);
 ad0_enable();
 SCI0_init(9600);
 _asm CLI;     // System Interrupts
 PIEH = 0xFF;  // Port H interrupts
 TSCR1 = 0x80; // Timer Interrupts
 TSCR2 = 0x00;
 TIE = 0x83;
 TIOS = 0x83;
 TC0 = TCNT + clicks_per_ms;
 TC1 = TCNT + clicks_per_ms;
 TC7 = TCNT + clicks_per_ms;
 while(1){  
  clear_lcd();
  // Ask user for Cook or Timer option
  cook = -1;
  printLCD("Cook = SW2");
  set_lcd_addr(0x40);
  printLCD("Timer = SW3");
  while (cook < 0){
   if(SW2_down()){
    cook = 1;
   }
   if(SW3_down()){
    cook = 0;
   }
  }
  
  // Ask user for time
  clear_lcd();
  printLCD("Enter Time (KEY)");
  set_lcd_addr(0x40);
  if(cook){
   printLCD("Cooking Mode"); 
  } else{
   printLCD("Timer Mode"); 
  }
  getTime();
  clear_lcd();
  
  // Convert user data to time
  clockTimeConvert();
  
  type_lcd("START=SW3");
  while(SW3_up()){
  
  }
  // Ask user to close microwave if Cook is selected and door is open
  if(open && cook){
    printLine("Microwave door must be closed before starting");
    clear_lcd();
    set_lcd_addr(0x40);
    printLCD("Close door (SW4)");
    while(open){
    
    }
  }
  clear_lcd();
  
  on = 1;
  ticks1 = 0; // Restart one second timer for countdown
  if(cook){
    printLine("The microwave is now cooking");
  } else{
    printLine("Timer has started");
  }
  printLine("Press SW5 to end");
  
  // Cook option executed
  if(cook){
    printLine("Adjust the external potentiometer to adjust rotation speed");
    printLine("Adjust The onboard potentiometer to adjust the power level");
    PORTB = 0x01;   // motor turn
    ticks5 = 0; // Restart three second timer for temperature update
    temp = (((ad0conv(5) >> 1) * 9)/5) + 32; // Get temperature of food when put in (fahrenheit)
    lightOn(); 
    while(on){
      powerValue = ((ad0conv(7)>> 6) / 3) + 1; // On board potentiometer controls power level (1-6)
      turnSpeed = ad0conv(2) >> 4; // External potentiometer controls plate turn speed
      motor0(turnSpeed);
      printTime();
      set_lcd_addr(0x40);
      write_int_lcd(powerValue);
      set_lcd_addr(0x45);
      printLCD("PWR");
      set_lcd_addr(0x48);
      write_int_lcd(temp);
      set_lcd_addr(0x4D);
      printLCD("TMP");
    }
    printLine("Microwave has finished cooking.");
    lightOff();
    PORTB = 0x00;         // motor stop turning
    }
  // Timer option is executed if selected
  else{
    while(on){
      printTime();
    }
    printLine("Timer has ended");
  }
  
  // Alarm 
  PWME = PWME & 0xFE; // Temporarily disable PWME0 to turn off 7-segment display 0. 
  alarm();
  PWME = PWME | 0x01;
  
  // Re enable timer and interrupts after sound_off() call
  _asm CLI;
  TSCR1 = 0x80;
  TSCR2 = 0x00;
  TIE = 0x83;
  TIOS = 0x83;
  
  // Display message
  clear_lcd();
  set_lcd_addr(0x00);
  if (cook){
    printLCD("Cooking complete");
    set_lcd_addr(0x40);
    printLCD("Enjoy!");
  } else{
    printLCD("Timer complete");
  }
  printLine("Wait or press SW5 to return to the mode select menu.");
  
  // Set 3 second timer for ending message
  wait = 1;
  ticks3 = 0;
  while(wait){
  }
 }
}