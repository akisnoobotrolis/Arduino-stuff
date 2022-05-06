// Real time clock and calendar with set buttons using DS1307 and Arduino
 
// include LCD library code
#include <LiquidCrystal.h>
// include Wire library code (needed for I2C protocol devices)
#include <Wire.h>
#include <SPI.h> 
#include <RFID.h>
#include <Servo.h> 


RFID rfid(10, 9);       
unsigned char status; 
unsigned char str[MAX_LEN]; 
Servo lockServo;                //Servo for locking mechanism
int lockPos = 15;               //Locked position limit
int unlockPos = 75;             //Unlocked position limit
boolean locked = true;

int greenLEDPin = 6;

String accessGranted [2] = {"310988016", "19612012715"};  //RFID serial numbers to grant access to
int accessGrantedSize = 2;                                //The number of serial numbers
 
// LCD module connections (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
unsigned long prevtime=0;


 
void setup() {
    Serial.begin(9600);     //Serial monitor is only required to get tag ID numbers and for troubleshooting
  SPI.begin();            //Start SPI communication with reader
  rfid.init();            //initialization 
  //LED startup sequence
  pinMode(greenLEDPin, OUTPUT);
  
  digitalWrite(greenLEDPin, HIGH);
  
 

  digitalWrite(greenLEDPin, LOW);
  lockServo.attach(3);
  lockServo.write(lockPos);         //Move servo into locked position
  Serial.println("Place card/tag near reader...");
  Serial.begin(9600);
  pinMode(8, INPUT_PULLUP);                      // button1 is connected to pin 8
  pinMode(9, INPUT_PULLUP);                      // button2 is connected to pin 9
  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  Wire.begin();                                  // Join i2c bus
}
 
char Time[]     = "TIME:  :  :  ";
char Calendar[] = "DATE:  /  /20  ";
byte i, second, minute, hour, date, month, year;
 
void DS1307_display(){
  // Convert BCD to decimal
  second = (second >> 4) * 10 + (second & 0x0F);
  minute = (minute >> 4) * 10 + (minute & 0x0F);
  hour   = (hour >> 4)   * 10 + (hour & 0x0F);
  date   = (date >> 4)   * 10 + (date & 0x0F);
  month  = (month >> 4)  * 10 + (month & 0x0F);
  year   = (year >> 4)   * 10 + (year & 0x0F);
  // End conversion
  Time[12]     = second % 10 + 48;
  Time[11]     = second / 10 + 48;
  Time[9]      = minute % 10 + 48;
  Time[8]      = minute / 10 + 48;
  Time[6]      = hour   % 10 + 48;
  Time[5]      = hour   / 10 + 48;
  Calendar[14] = year   % 10 + 48;
  Calendar[13] = year   / 10 + 48;
  Calendar[9]  = month  % 10 + 48;
  Calendar[8]  = month  / 10 + 48;
  Calendar[6]  = date   % 10 + 48;
  Calendar[5]  = date   / 10 + 48;
  lcd.setCursor(0, 0);
  lcd.print(Time);                               // Display time
  lcd.setCursor(0, 1);
  lcd.print(Calendar);                           // Display calendar
}
void blink_parameter(unsigned long ct){
  if(ct-prevtime>=25){
  byte j = 0;
  while(j < 10 && digitalRead(8) && digitalRead(9)){
    j++;
  }
  prevtime=ct;
  }
}

void clock(unsigned long currentmillis){
  if(currentmillis-prevtime>=200){
  if(!digitalRead(8)){                           // If button (pin #8) is pressed
      i = 0;
      hour   = edit(5, 0, hour,currentmillis);
      minute = edit(8, 0, minute,currentmillis);
      date   = edit(5, 1, date,currentmillis);
      month  = edit(8, 1, month,currentmillis);
      year   = edit(13, 1, year,currentmillis);
      // Convert decimal to BCD
      minute = ((minute / 10) << 4) + (minute % 10);
      hour = ((hour / 10) << 4) + (hour % 10);
      date = ((date / 10) << 4) + (date % 10);
      month = ((month / 10) << 4) + (month % 10);
      year = ((year / 10) << 4) + (year % 10);
      // End conversion
      // Write data to DS1307 RTC
      Wire.beginTransmission(0x68);               // Start I2C protocol with DS1307 address
      Wire.write(0);                              // Send register address
      Wire.write(0);                              // Reset sesonds and start oscillator
      Wire.write(minute);                         // Write minute
      Wire.write(hour);                           // Write hour
      Wire.write(1);                              // Write day (not used)
      Wire.write(date);                           // Write date
      Wire.write(month);                          // Write month
      Wire.write(year);                           // Write year
      Wire.endTransmission();                     // Stop transmission and release the I2C bus
     prevtime=currentmillis;    
  } // Wait 200ms
    }
    if(currentmillis-prevtime>=50){
    Wire.beginTransmission(0x68);                 // Start I2C protocol with DS1307 address
    Wire.write(0);                                // Send register address
    Wire.endTransmission(false);                  // I2C restart
    Wire.requestFrom(0x68, 7);                    // Request 7 bytes from DS1307 and release I2C bus at end of reading
    second = Wire.read();                         // Read seconds from register 0
    minute = Wire.read();                         // Read minuts from register 1
    hour   = Wire.read();                         // Read hour from register 2
    Wire.read();                                  // Read day from register 3 (not used)
    date   = Wire.read();                         // Read date from register 4
    month  = Wire.read();                         // Read month from register 5
    year   = Wire.read();                         // Read year from register 6
    DS1307_display();                             // Diaplay time & calendar
    prevtime=currentmillis;
    }// Wait 50ms
}


void DIFR(unsigned long ct){

  if (rfid.findCard(PICC_REQIDL, str) == MI_OK)   //Wait for a tag to be placed near the reader
  { 
   
    String temp = "";                             //Temporary variable to store the read RFID number
    if (rfid.anticoll(str) == MI_OK)              //Anti-collision detection, read tag serial number 
    { 
       
      for (int i = 0; i < 4; i++)                 //Record and display the tag serial number 
      { 
        temp = temp + (0x0F & (str[i] >> 4)); 
        temp = temp + (0x0F & str[i]); 
      } 
   
      checkAccess (temp,ct);     //Check if the identified tag is an allowed to open tag
    } 
    rfid.selectTag(str); //Lock card to prevent a redundant read, removing the line will make the sketch read cards continually
  }
  rfid.halt();
}

void checkAccess (String temp,unsigned long ct)    //Function to check if an identified tag is registered to allow access
{
  boolean granted = false;
  for (int i=0; i <= (accessGrantedSize-1); i++)    //Runs through all tag ID numbers registered in the array
  {
    if(accessGranted[i] == temp)            //If a tag is found then open/close the lock
    {
     
      granted = true;
      if (locked == true)         //If the lock is closed then open it
      {
          lockServo.write(unlockPos);
          locked = false;
      }
      else if (locked == false)   //If the lock is open then close it
      {
          lockServo.write(lockPos);
          locked = true;
      }
      if(ct-prevtime>=200){
      digitalWrite(greenLEDPin, HIGH);    //Green LED sequence
      prevtime=ct;
      }
      if(ct-prevtime>=200){
      digitalWrite(greenLEDPin, LOW);
       prevtime=ct;
      }
      if(ct-prevtime>=200){
      digitalWrite(greenLEDPin, HIGH);
       prevtime=ct;
      }
      if(ct-prevtime>=200){
      digitalWrite(greenLEDPin, LOW);
       prevtime=ct;
      }
    }
  }
 
}










  

byte edit(byte x, byte y, byte parameter,unsigned long ct){
  char text[3];
  while(!digitalRead(8));                        // Wait until button (pin #8) released
  while(true){
    while(!digitalRead(9)){                      // If button (pin #9) is pressed
      if(ct-prevtime>=200){
      parameter++;
      if(i == 0 && parameter > 23)               // If hours > 23 ==> hours = 0
        parameter = 0;
      if(i == 1 && parameter > 59)               // If minutes > 59 ==> minutes = 0
        parameter = 0;
      if(i == 2 && parameter > 31)               // If date > 31 ==> date = 1
        parameter = 1;
      if(i == 3 && parameter > 12)               // If month > 12 ==> month = 1
        parameter = 1;
      if(i == 4 && parameter > 99)               // If year > 99 ==> year = 0
        parameter = 0;
      sprintf(text,"%02u", parameter);
      lcd.setCursor(x, y);
      lcd.print(text);
      prevtime=ct;
      }                  // Wait 200ms
    }
    lcd.setCursor(x, y);
    lcd.print("  ");                             // Display two spaces
    blink_parameter(ct);
    sprintf(text,"%02u", parameter);
    lcd.setCursor(x, y);
    lcd.print(text);
    blink_parameter(ct);
    if(!digitalRead(8)){                         // If button (pin #8) is pressed
      i++;                                       // Increament 'i' for the next parameter
      return parameter;                          // Return parameter value and exit
    }
  }
}
 
void loop() {
  unsigned long currentmillis=millis();
 // clock(currentmillis);
  DIFR(currentmillis);

}
