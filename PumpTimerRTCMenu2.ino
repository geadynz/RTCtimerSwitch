#include <virtuabotixRTC.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "buildTime.h"
#include <AceButton.h>
#include <EEPROM.h>



using namespace ace_button;

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//define Menu Items;

#define MENU_SETTINGS     1
#define SETTINGS_SUB      100
#define MENU_SET_HR       11
#define MENU_SET_MIN      12
#define MENU_SET_DAY      13
#define MENU_SET_MONTH    14
#define MENU_SET_YEAR     15
#define MENU_SET_PROGRAM        2
#define MENU_SET_PROGRAMonHr    21
#define MENU_SET_PROGRAMonMin   22
#define MENU_SET_PROGRAMoffHr   23
#define MENU_SET_PROGRAMoffMin  24
#define MENU_PUMP_CONTROL       3
#define MENU_SETTINGS_BL        101

//define epprom address for program times
#define ERonTimeHR    0
#define ERonTimeMIN   1
#define ERonTimeSEC   3
#define ERoffTimeHR   4
#define ERoffTimeMIN  5
#define ERoffTimeSEC  6
#define ERset         7
#define ERBL   8 //Backlight setting

//define real time clock pins
#define CLKpin  6
#define DATpin  7
#define RSTpin  8



#define relay   10
#define BL      9

//define button pins
#define BTmenu 2
#define BTup   3
#define BTdown 4

//Variables for timer from epprom
byte onhour1;
byte onmin1;
byte onsec1;

byte offhour1;
byte offmin1;
byte offsec1;

//Variable for Backlight setting;
byte ERbacklight;

//To convert times to Single number
unsigned long Time;
unsigned long Hour;
unsigned long Min;
unsigned long Sec;

unsigned long on_time1;
unsigned long on_hour1;
unsigned long on_min1;
unsigned long on_sec1;

unsigned long off_time1;
unsigned long off_hour1;
unsigned long off_min1;
unsigned long off_sec1;

bool menuBtn = false;
bool up = false;
bool down = false;
bool buttonPress = false;
bool ProgramSet = false;
bool programSetLoaded = false;
bool PumpState = false;

unsigned long LCDstartMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 30000;  //the value is a number of milliseconds


//Creation of Button Oblects
AceButton BTN1(BTmenu);
AceButton BTN2(BTup);
AceButton BTN3(BTdown);

// Forward reference to prevent Arduino compiler becoming confused.
void handleEvent(AceButton*, uint8_t, uint8_t);

// Creation of the Real Time Clock Object
virtuabotixRTC myRTC(CLKpin, DATpin, RSTpin);

int menu = 0;  //Menu flag
int subMenu = 0; //sub Menu flag
int ss = BUILD_SEC;
int mm = BUILD_MIN;
int hh = BUILD_HOUR;
int day = 0;
int dd = BUILD_DAY;
int month = BUILD_MONTH;
int yy = BUILD_YEAR;

int backlight = 127;

void setup()  {
  Serial.begin(115200);
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  
  pinMode(BL, OUTPUT);
  pinMode(BTmenu, INPUT);
  pinMode(BTup, INPUT);
  pinMode(BTdown, INPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  
  //read Backlight value from EEPROM
  ERbacklight = EEPROM.read(ERBL);
  if (ERbacklight) backlight = ERbacklight;
  
  //myRTC.setDS1302Time(ss, mm, hh, day, dd, month, yy);
  lcd.init();
  analogWrite(BL,backlight);
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Pump Timmer:");
  lcd.setCursor(0,1);
  lcd.print("Version 1.0");
  delay(5000);
  if (!EEPROM.read(ERset)){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("No Program Set!");
    delay(5000);
  }else{
    readTimer();
    
  }
  
  
  
  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);
  LCDstartMillis = millis();
  delay(5000);
  lcd.clear();
}

/**********************************************************************************************************
 * Function to read timer program from EEPROM
 */
void readTimer(){
    char serialbuffer[12];
    Serial.println("reading EEPROM Valves");
    onhour1 = EEPROM.read(ERonTimeHR);
    onmin1 = EEPROM.read(ERonTimeMIN);
    onsec1 = EEPROM.read(ERonTimeSEC);
    offhour1 = EEPROM.read(ERoffTimeHR);
    offmin1 = EEPROM.read(ERoffTimeMIN);
    offsec1 = EEPROM.read(ERoffTimeSEC);
    
    on_hour1 = onhour1;
    on_min1 = onmin1;
    off_hour1 = offhour1;
    off_min1 = offmin1;

    programSetLoaded = true;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Program Loaded");
    sprintf(serialbuffer,"%02u:%02u",onhour1,onmin1);
    Serial.print("on Time: ");
    Serial.println(serialbuffer);
    delay(500);
}

/*********************************************************************************************************
 * Funtion to write Program times to eeprom
 */
void saveProgram(){
   lcd.clear();
   onhour1 = on_hour1;
   onmin1 = on_min1;
   offhour1 = off_hour1;
   offmin1 = off_min1;

   EEPROM.write(ERonTimeHR, onhour1);
   EEPROM.write(ERonTimeMIN, onmin1);
   EEPROM.write(ERoffTimeHR, offhour1);
   EEPROM.write(ERoffTimeMIN, offmin1);
   EEPROM.write(ERset, 1);
   
   if(EEPROM.read(ERset)){//Program has been saved
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(".....SAVED......");
      delay(500);
   }else{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("!!!!!ERROR!!!!!!");
      delay(500);
   }
   
   
   
}
/**************************************************************************************************************
*Funtion to set relay start on time and relay off time the save to epprom
* 
* 
 */
void StartTime(){
  if(!programSetLoaded) {
    readTimer();
    //programSetLoaded = true;
  }
  on_hour1 = onhour1;
  on_min1 = onmin1;
  

  int hh = on_hour1;
  int mm = on_min1;
  
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.print("Start Time:      ");
  
  //set start time Hours
  while (menu == MENU_SET_PROGRAMonHr){ 
    char timerBuffer[12];
    sprintf(timerBuffer,"%02u:%02u",hh,mm);
    lcd.setCursor(0,1);
    lcd.print(timerBuffer);
    lcd.setCursor(1,1);
    lcd.blink();
    
    BTN2.check();
    BTN3.check();
    if (up){
      hh++;
      up = false;
      if (hh > 23) hh = 0;
    }
    if (down){
      hh--;
      down = false;
      if (hh < 0) hh = 23;
      
    }
    on_hour1 = hh;
    BTN1.check();

    if (menuBtn){
      menuBtn = false;
      menu++;
    }
    
  }
  //Set Start time Minutes
  while (menu == MENU_SET_PROGRAMonMin){
    char timerBuffer[12];
    sprintf(timerBuffer,"%02u:%02u",hh,mm);
    lcd.setCursor(0,1);
    lcd.print(timerBuffer);
    lcd.setCursor(4,1);
    
    BTN2.check();
    BTN3.check();
    if (up){
      mm++;
      up = false;
      if (mm > 59) mm = 0; 
      Serial.println(mm); 
    }
    if(down){
      mm--;
      down = false;
      if(mm < 0) mm = 59;
      Serial.println(mm);
    }
    
    on_min1 = mm;
    BTN1.check();
    if (menuBtn){
      menuBtn = false;
      menu = 0;
      subMenu = 0;
      saveProgram();
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(0,0);
      
    }
  }
}

/****************************************************************************************/


void StopTime(){
  if(!programSetLoaded) {
    readTimer();
    //programSetLoaded = true;
  }
  off_hour1 = offhour1;
  off_min1 = offmin1;
  int hh = off_hour1;
  int mm= off_min1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Stop Time:");
  
  //Set Off time Hours
  while (menu == MENU_SET_PROGRAMoffHr){
    char timerBuffer[12];
    sprintf(timerBuffer,"%02u:%02u",hh,mm);
    lcd.setCursor(0,1);
    lcd.print(timerBuffer);
    lcd.setCursor(1,1);
    lcd.blink();
    BTN2.check();
    BTN3.check();
    if(up){
      hh++;
      if(hh > 23) hh = 0;
      up = false;
    }
    if(down){
      hh--;
      if(hh < 0) hh = 23;
      down = false;
    }
    off_hour1 = hh;
    BTN1.check();
    if (menuBtn){
      menuBtn = false;
      menu++;
    }
  }
  //Set Off Time Minutes
  while (menu == MENU_SET_PROGRAMoffMin){
    char timerBuffer[12];
    sprintf(timerBuffer,"%02u:%02u",hh,mm);
    lcd.setCursor(0,1);
    lcd.print(timerBuffer);
    lcd.setCursor(4,1);
    BTN2.check();
    BTN3.check();
    if (up){
      mm++;
      if(mm > 59) mm = 0;
      up = false;
    }
    if (down){
      mm--;
      if(mm < 0) mm = 59;
      down = false;
    }
    off_min1 = mm;
    BTN1.check();
    if(menuBtn){
      menuBtn = false;
      menu = 0;
      subMenu = 0;
      saveProgram();
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(0,0);
    }
  }
  
}

/****************************************************************************************************************
 * Pump Control
 ***************************************************************************************************************/
void pumpControl(){
  menuBtn = false;
  //lcd.clear();
  lcd.setCursor(12,0);
  lcd.print("- ");
  lcd.setCursor(0,1);
  if (PumpState){
    lcd.print("Pump State: ON");
  }
  else{
    lcd.print("Pump State: OFF");
  }
  while (menu == MENU_PUMP_CONTROL){
    BTN1.check();
    BTN2.check();
    BTN3.check();
    if (menuBtn){
      menuBtn = false;
      menu = 0;
      lcd.clear();
    }
    if (up){ //Turn ON Pump
      digitalWrite(relay, HIGH);
      PumpState = true;
      lcd.setCursor(12,1);
      lcd.print("ON ");
      up = false;
    }
  
    if (down){//Turn OFF Pump
      digitalWrite(relay, LOW);
      PumpState = false;
      lcd.setCursor(12,1);
      lcd.print("OFF");
      down = false;
    }
  }
  
}
/***************************************************************************************************
//Adjust Date/Time
***************************************************************************************************/
void SetDate(){
  myRTC.updateTime();
  int ss = myRTC.seconds;
  int mm = myRTC.minutes;
  int hh = myRTC.hours;
  int day = myRTC.dayofweek;
  int dd = myRTC.dayofmonth;
  int month = myRTC.month;
  int yy = myRTC.year;
  
  //Set time
  //Set Hour
  lcd.setCursor(0,0);
  lcd.print("Set Time:         ");
  while (subMenu == MENU_SET_HR){
    char timeBuffer[12];
    sprintf(timeBuffer,"%02u:%02u",hh,mm);
    lcd.setCursor(0,1);
    lcd.print(timeBuffer);
    lcd.setCursor(1,1);
    lcd.blink();

    BTN2.check();
    BTN3.check();
    if (up) {
      hh++;
      up = false;
      if (hh > 23) hh = 0;
    }
    if (down) {
      hh--;
      down = false;
      if (hh < 0) hh = 23;
      
    }
    myRTC.setDS1302Time(ss,mm,hh,day,dd,month,yy);
    myRTC.updateTime();
    BTN1.check();
    if (menuBtn){
      subMenu++;
      menuBtn = false;
    }
  }

  //set Minutes
  while (subMenu == MENU_SET_MIN){
    char timeBuffer[12];
    sprintf(timeBuffer,"%02u:%02u ",hh,mm);
    lcd.setCursor(0,1);
    lcd.print(timeBuffer);
    lcd.setCursor(4,1);
    lcd.blink();

    BTN2.check();
    BTN3.check();
    if (up) {
      mm++;
      up = false;
      if (mm > 59) mm = 0;
    }
    if (down){
      down = false;
      mm--;
      if (mm < 0) mm = 59;
      
    }
    myRTC.setDS1302Time(ss,mm,hh,day,dd,month,yy);
    
    BTN1.check();
    if(menuBtn){
      subMenu++;
      menuBtn = false;
    }
  }
  
  lcd.setCursor(0,0);
  lcd.print("Set Date:           ");
  
  //Set Day of month
  while (subMenu == MENU_SET_DAY){
    char dateBuffer[12];
    sprintf(dateBuffer,"%02u-%02u-%04u ",dd,month,yy);
    lcd.setCursor(0,1);
    lcd.print(dateBuffer);
    lcd.setCursor(0,1);
    lcd.blink();
    
    BTN2.check();
    BTN3.check();
    if (up) {
      dd ++;
      up = false;
      if (dd > 31) dd= 1;
    }
    if (down){
      dd--;
      down = false;
      if (dd < 1) dd = 31; 
    }
  myRTC.setDS1302Time(ss, mm, hh, day, dd, month, yy);
  
  BTN1.check();
  if(menuBtn){
      subMenu++;
      menuBtn = false;
    }
  }
  
  
  //Set month
  while (subMenu == MENU_SET_MONTH){
    char dateBuffer[12];
    sprintf(dateBuffer,"%02u-%02u-%04u ",dd,month,yy);
    lcd.setCursor(0,1);
    lcd.print(dateBuffer);
    lcd.setCursor(4,1);
    lcd.blink();
    
    BTN2.check();
    BTN3.check();
    if (up) {
      month ++;
      up = false;
      if (month > 12) month = 1;
    }
    if (down){
      month--;
      down = false;
      if (month < 1) month = 12; 
    }
    myRTC.setDS1302Time(ss, mm, hh, day, dd, month, yy);
    
    BTN1.check();
    if(menuBtn){
      subMenu++;
      menuBtn = false;
    }
    
  }
  

//Set Year
  while (subMenu == MENU_SET_YEAR){
    char dateBuffer[12];
    sprintf(dateBuffer,"%02u-%02u-%04u ",dd,month,yy);
    lcd.setCursor(0,1);
    lcd.print(dateBuffer);
    lcd.setCursor(10,1);
    lcd.blink();
    
    BTN2.check();
    BTN3.check();
    if (up) {
      yy ++;
      up = false;
      
    }
    if (down){
      yy--;
      down = false;
      if (yy < 2020) yy = 2020; 
    }
    myRTC.setDS1302Time(ss, mm, hh, day, dd, month, yy);// write date/time to RTC
    
    BTN1.check();
    if(menuBtn){
      subMenu++;
      menuBtn = false;
    }
  }
  
  lcd.noBlink();
  lcd.clear();
  subMenu = 0;
  menu = 0;
}

/*******************************************************************************************************
 * Update date clock displayed on LCD
 * 
 *******************************************************************************************************/
void displayDate(){
  char dateBuffer[12];
  sprintf(dateBuffer,"%02u-%02u-%04u ",myRTC.dayofmonth,myRTC.month,myRTC.year);
  //Serial.print(dateBuffer);
  lcd.setCursor(0,0);
  lcd.print("DATE: ");
  lcd.setCursor(6,0);
  lcd.print(dateBuffer);
}

//Update the time displayed on the LCD
void displayTime(){
  char dateBuffer[12];
  sprintf(dateBuffer,"%02u:%02u:%02u ",myRTC.hours,myRTC.minutes,myRTC.seconds);
  //Serial.println(dateBuffer);
  lcd.setCursor(0,1);
  lcd.print("TIME: ");
  lcd.setCursor(6,1);
  lcd.print(dateBuffer);

  
}
/***************************************************************************************************************
 * handle button presses
 **************************************************************************************************************/

void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState){
  
  switch (eventType) {
    case AceButton::kEventPressed:
      
      LCDstartMillis = millis();
      lcd.display();
      analogWrite(BL,127);
      if (button->getPin() == BTmenu) {
        Serial.println("Menu Button Pressed");
        menuBtn = true; 
      }
      if (button->getPin() == BTup){
        Serial.println("Up Button Pressed");
        up = true;
        
      }
      if (button->getPin() == BTdown){
        Serial.println("Down Button Pressed");
        down = true;
        
      }
      
      break;

  }


}

void loop()  {
  lcdTimeOut();//LCD Time Out
  BTN1.check();
  BTN2.check();
  BTN3.check();
  myRTC.updateTime();

  //Hour = myRTC.hours;
  //Min = myRTC.minutes;

  if (menu < 1){
    displayDate();
    displayTime();
  }
  if(menuBtn) displayMenu();
  
  //check program for run event
  if(!PumpState){
    if(myRTC.hours == on_hour1 && myRTC.minutes == on_min1){
      digitalWrite(relay,HIGH);
      PumpState = true;
      char timeBuffer[12];
      sprintf(timeBuffer,"%02u:%02u ",myRTC.hours,myRTC.minutes);
      Serial.print(timeBuffer);
      Serial.println("Pump Turned ON");
    } 
  }
  //check program for stop event
  if(PumpState){
    if(myRTC.hours == off_hour1 && myRTC.minutes == off_min1){
      digitalWrite(relay,LOW);
      PumpState = false;
      char timeBuffer[12];
      sprintf(timeBuffer,"%02u:%02u ",myRTC.hours,myRTC.minutes);
      Serial.print(timeBuffer);
      Serial.println("Pump Turned OFF");
    }
  }
  
  

  up = false;
  down = false;
  menuBtn = false;

}

/*********************************************************************************************************

//Menu Display
*********************************************************************************************************/
void displayMenu(){
  
  lcd.clear();
  //Serial.println(menu);
  //menuBtn = false;
  
  if(menu == 0) menu++;
  while(menu > 0){
    Serial.println(menu);
    switch(menu){
      case MENU_SETTINGS:
        lcd.setCursor(0,0);
        lcd.print("Settings");
        
        while (menu == MENU_SETTINGS){
          //Check Buttons
          menuBtn = false;
          lcdTimeOut();
          BTN1.check();
          BTN2.check();
          BTN3.check();

          if (menuBtn) { //enter settings menu
            menuBtn = false;
            subMenu = SETTINGS_SUB;
            displaySettings();
          }
          if (up){
            up = false; 
            nextItem();  
          }      
          if (down){ 
            down = false;
            previousItem();  
          }
        }    
      break;
      case MENU_SET_PROGRAM:
        lcd.setCursor(0,0);
        lcd.print("Program");
        Serial.println("Program");
        //check buttons
        while(menu == MENU_SET_PROGRAM){
          lcdTimeOut();
          menuBtn = false;
          BTN1.check();
          BTN2.check();
          BTN3.check();
          if (menuBtn) { //enter Program menu
            menuBtn = false;
            //menu = MENU_SET_PROGRAM;
            displayProgram();  
          }
          if (up){
            up = false; 
            nextItem();
          }      
          if (down){
            down = false; 
            previousItem();
          }
        } 
      break;

      case MENU_PUMP_CONTROL:
        lcd.setCursor(0,0);
        lcd.print("Pump Control");
        menuBtn = false;  
        while (menu == MENU_PUMP_CONTROL){
          lcdTimeOut();
          BTN1.check();
          BTN2.check();
          BTN3.check();
          //Serial.println(menu);
          //Serial.println(menuBtn);  
          if (menuBtn){ 
            menuBtn = false;
            Serial.println("Pump Control");          
            pumpControl();
          }
          if (up){
            up = false;
            nextItem();
          }
          if (down){
            down = false;
            previousItem();
          }
        }
      break;

      default:
        menuBtn = false;
        menu = 0;
      break;
    }
  }
}
/********************************************************************************
 * Funtion to display Pump Program
 */
 void displayProgram(){
   int timerMenu = 1;

   while(menu == MENU_SET_PROGRAM){
     lcdTimeOut();
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Start Time:      ");

      while(timerMenu == 1){
        lcdTimeOut();
        displayStartTime();
        BTN1.check();
        BTN2.check();
        BTN3.check();
        if(menuBtn){
         menuBtn = false;
          menu = MENU_SET_PROGRAMonHr;
          StartTime();
          timerMenu = 0;
        } 
        if(up || down){
          up = false;
          down = false;
          timerMenu = 2;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Stop Time:          ");
        }
      }
    
      while(timerMenu == 2){
        lcdTimeOut();
        displayStopTime();
        BTN1.check();
        BTN2.check();
        BTN3.check();
        if(menuBtn){
          menuBtn = false;
          menu = MENU_SET_PROGRAMoffHr;
          StopTime();
          timerMenu = 0;
        } 
        if(up || down){
          up = false;
          down = false;
          timerMenu = 1;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Start Time:          ");
        }
      }
      if(timerMenu == 0){
        menu = 0;
        subMenu = 0;
      }
   }
 } 
/********************************************************************************
 *Funrtion to display Stop Time
 */
void displayStopTime(){
  if(!programSetLoaded)
    readTimer();
  on_hour1 = onhour1;
  on_min1 = onmin1;
  off_hour1 = offhour1;
  off_min1 = offmin1;

  int hh = off_hour1;
  int mm = off_min1;

  //lcd.setCursor(0,0);
  //lcd.clear();
  //lcd.print("Stop Time:");
  char timerBuffer[12];
  sprintf(timerBuffer,"%02u:%02u",hh,mm);
  lcd.setCursor(0,1);
  lcd.print(timerBuffer);

}
/********************************************************************************
 * Function to display pump start time
 */
void displayStartTime(){
  if(!programSetLoaded)
    readTimer();
  on_hour1 = onhour1;
  on_min1 = onmin1;
  off_hour1 = offhour1;
  off_min1 = offmin1;
  
  int hh = on_hour1;
  int mm = on_min1;

  //lcd.setCursor(0,0);
  //lcd.clear();
  //lcd.print("Start Time:");
  char timerBuffer[12];
  sprintf(timerBuffer,"%02u:%02u",hh,mm);
  lcd.setCursor(0,1);
  lcd.print(timerBuffer);
}

/********************************************************************************
 * Funtion to display the settings Menu
 */
void displaySettings(){
  lcd.clear();

  while (menu == MENU_SETTINGS)
  {
    switch (subMenu)
    {
    case SETTINGS_SUB:
      lcd.setCursor(0,0);
      lcd.print("Set Time/Date       ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      while(subMenu == SETTINGS_SUB){
        lcdTimeOut();
        BTN1.check();
        BTN2.check();
        BTN3.check();
        if(menuBtn){
          menuBtn = false;
          subMenu = MENU_SET_HR;
          SetDate();
          break;
        }
        if(up){
          up = false;
          subMenu++;
          if(subMenu > MENU_SETTINGS_BL) subMenu = SETTINGS_SUB;
        }
        if(down){
          down = false;
          subMenu--;
          if (subMenu < SETTINGS_SUB) subMenu = MENU_SETTINGS_BL;
        }
        
      }
    case MENU_SETTINGS_BL:
      lcd.setCursor(0,0);
      lcd.print("Set Screen      ");
      lcd.setCursor(0,1);
      lcd.print("Brightness      ");
      while (subMenu == MENU_SETTINGS_BL){
        lcdTimeOut();
        BTN1.check();
        BTN2.check();
        BTN3.check();
        if (menuBtn){
          menuBtn = false;
          setBL();
        }
        if(up){
          up = false;
          subMenu++;
          if(subMenu > MENU_SETTINGS_BL) subMenu = SETTINGS_SUB;
        }
        if(down){
          down = false;
          subMenu--;
          if (subMenu < SETTINGS_SUB) subMenu = MENU_SETTINGS_BL;
         }
      }
      break;
     
    default:
      subMenu = 0;
      menu = 0;
      break;
    }
  }
}

/********************************************************************************
 * Function to move to next menu item
 */
void nextItem(){
  lcd.clear();
  menu++;
  if(menu > 3) menu = 1;
  up = false;
  //Serial.println(menu);
}

/********************************************************************************
 * Function to move to previous Menu Item
 */

void previousItem(){
  lcd.clear();
  menu--;
  if(menu < 1) menu = 3;
  down = false;
  //Serial.println(menu);
}

void lcdTimeOut(void){
  //Serial.print(currentMillis);
  //Serial.print(" - ");
  ///Serial.print(LCDstartMillis);
  //Serial.print(" = ");
  //Serial.println(currentMillis - LCDstartMillis);
  currentMillis = millis();
  if ((currentMillis - LCDstartMillis ) >= period){  
    lcd.clear();
    lcd.noDisplay();
    analogWrite(BL,0);
    menu = 0;
    subMenu = 0;
    }
  else {
    
    lcd.display();
    analogWrite(BL,127);
  }
}
/********************************************************************
 * Funrctio to set the brightness of the LCD Screen Backlight
 */

void setBL(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set Backlight");
  lcd.setCursor(0,1);
  lcd.print(backlight);
  lcd.setCursor(0,1);
  lcd.blink();

  while (subMenu == MENU_SETTINGS_BL){
    lcdTimeOut();
    BTN1.check();
    BTN2.check();
    BTN3.check();
    if(menuBtn){
      menuBtn=false;
      //Save Backlight value to eeprom
      EEPROM.write(ERBL,backlight);
      //exit menus
      menu = 0;
      subMenu = 0;
      lcd.noBlink();
      break;
    }
    if(up){
      up = false;
      //increase Backlight value
      backlight = backlight + 10;
      if (backlight > 250) backlight = 250;
      analogWrite(BL,backlight);
      char lcdbuffer[3];
      sprintf(lcdbuffer,"%03u ",backlight);
      lcd.print(lcdbuffer);
      lcd.setCursor(0,1);
      
    }
    if(down){
      down = false;
      //decrease Backlight value
      backlight = backlight - 10;
      if (backlight < 50) backlight = 50;
      analogWrite(BL,backlight);
      char lcdbuffer[3];
      sprintf(lcdbuffer,"%03u ",backlight);
      lcd.print(lcdbuffer);
      lcd.setCursor(0,1);
    }
  }
}