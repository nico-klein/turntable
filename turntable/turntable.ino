                                                                                                          
#include <LiquidCrystal_I2C.h> 
#include <Adafruit_NeoPixel.h> 
#include <Encoder.h>
#include <Servo.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_TCS34725.h>

#define LED_PIN       12  // internal LED
#define LED_COUNT     72  // LED stripe

#define ENCODER_PIN_1 2   // encoder
#define ENCODER_PIN_2 3   // encoder

#define SERVO_PIN     5   // servo

#define COLOR_SENSOR_LED_PIN  13 // same as build in led

#define IR_SENSOR_1_PIN 0      // analog
#define IR_SENSOR_2_PIN 1      // analog
#define IR_THRESHOLD  400    // sensor returns ~200 if object otherwise ~1000

#define TONEARM_UNSURE    0
#define TONEARM_NORMPOS   1
#define TONEARM_ENDPOS    2
#define TONEARM_STARTPOS  3
#define TONEARM_CHANGEDURATION 2  // after x seconds of sensor value the position will be regarded as changed
 
#define MODE_MENU_IDLE   1
#define MENU_MENU_SELECT 2

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C 0x27, 16 columns, 2 rows
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Encoder encoder(ENCODER_PIN_1, ENCODER_PIN_2);
Servo servo;
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_600MS, TCS34725_GAIN_1X);


long          encPos               = -999;
unsigned long timestampLastEvent   = millis();
unsigned long tonearmEndPosTime    = 0;
unsigned long tonearmStartPosTime  = 0;
unsigned long tonearmNormPosTime   = 0;
char menu_strings[][16] = {
  "Color          ", 
  "Saturation     ",  
  "Brightness     ", 
  "Autocolor      ", 
  "Test Servo     ", 
  "Test IR Sensors", 
  "Test Tornarm   ", 
  "Test Color Sens",
  "Write EEPROM   "};
byte  menu_position = 0;

byte  stripe_hue = 84;
byte  stripe_sat = 255;
byte  stripe_val = 30;
byte  auto_color = 0;
byte  stripe_always_on = 0;

void readFromEeprom() {
  stripe_hue = EEPROM.read(0);
  stripe_sat = EEPROM.read(1);
  stripe_val = EEPROM.read(2);
  auto_color = EEPROM.read(3);
}

void writeToEeprom() {
  EEPROM.write(0, stripe_hue);
  EEPROM.write(1, stripe_sat);
  EEPROM.write(2, stripe_val);
  EEPROM.write(3, auto_color);    
  lcd.setCursor(0, 1);
  lcd.print("stored in eeprom");
  delay(1000);
}
 
void setup() {
  readFromEeprom();
  
  // init onboard led
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on 
  //delay(500);                       // wait for half a second
  //digitalWrite(LED_BUILTIN, LOW);    // turn the LED off 
  //delay(500);                       // wait for half a second

  // init and test lcd
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.setCursor(0, 0);         
  lcd.print("Turntable V 0.1"); 

  // init and test stripe
  strip.begin();
  int offset = 4;
  for (uint16_t i = 0; i < LED_COUNT + offset; i++) {

      if (i < LED_COUNT) {
        strip.setPixelColor(i, strip.ColorHSV(i * 256 / LED_COUNT * 256, 255, 50));  
      }

      if (i >= offset) {
        strip.setPixelColor(i - offset, 0, 0, 0);
      }
      strip.show();  
      delay(20); 

  }

  // init servo
  /*
  servo.attach(SERVO_PIN);
  servo.write(30);
  delay(200);
  servo.write(0);
  */
  
  lcd.noBacklight();
  checkEncoder();
}

// returns 0 if not changed. otherwise the delta of encPos
int checkEncoder() {  
  int return_value = 0;
  long encNewPos = encoder.read();

  // rotation detected
  if (encNewPos != encPos) {
    return_value = encNewPos - encPos;
    encPos = encNewPos;
    timestampLastEvent = millis();
  }

 return return_value; 
}


int checkTonearm() {
  // end position detected
  if (analogRead(IR_SENSOR_1_PIN) < IR_THRESHOLD) {
    tonearmNormPosTime = 0;
    if (tonearmEndPosTime == 0) {
      tonearmEndPosTime = millis();
    }
    else {
      if (millis() - tonearmEndPosTime > TONEARM_CHANGEDURATION * 1000) {
        return TONEARM_ENDPOS;
      }
    }
  }

  // end position detected
  else if (analogRead(IR_SENSOR_2_PIN) < IR_THRESHOLD) {
    tonearmNormPosTime = 0;
    if (tonearmStartPosTime == 0) {
      tonearmStartPosTime = millis();
    }
    else {
      if (millis() - tonearmStartPosTime > TONEARM_CHANGEDURATION * 1000) {
        return TONEARM_STARTPOS;
      }
    }
  }

  else {
    tonearmEndPosTime = 0;
    tonearmStartPosTime = 0;

    if (tonearmNormPosTime == 0) {
      tonearmNormPosTime = millis();
    }
    else {
      if (millis() - tonearmNormPosTime > TONEARM_CHANGEDURATION * 1000) {
        return TONEARM_NORMPOS;
      }
    }
  }
  return TONEARM_UNSURE;  
}

void setStripeHue() {
  timestampLastEvent = millis();
  
  while (millis() - timestampLastEvent < 2000) {

    int encoder_return = checkEncoder();  
    if (encoder_return != 0) {
      if (encoder_return > 0) {
        stripe_hue += 10;
        if (stripe_hue > 255){
          stripe_hue -= 255;
        }
      }
      else if ((encoder_return < 0) && (stripe_hue > 0) ) {
        stripe_hue -= 1;
      } 

      strip.fill(strip.ColorHSV(stripe_hue * 256, stripe_sat, stripe_val), 0, LED_COUNT);
      strip.show();
      
    }

    char buffer[16];
    lcd.setCursor(0, 1);
    sprintf(buffer,"hue %6d", stripe_hue);      
    lcd.print(buffer);
    delay(20);
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  return;
}


void setStripeVal() {
  timestampLastEvent = millis();
  
  while (millis() - timestampLastEvent < 2000) {

    int encoder_return = checkEncoder();  
    if (encoder_return != 0) {
      if (encoder_return > 0) {
        stripe_val += 10;
        if (stripe_val > 255){
          stripe_val -= 255;
        }
      }
      else if ((encoder_return < 0) && (stripe_val > 0) ) {
        stripe_hue -= 1;
      } 

      strip.fill(strip.ColorHSV(stripe_hue * 256, stripe_sat, stripe_val), 0, LED_COUNT);
      strip.show();
      
    }

    char buffer[16];
    lcd.setCursor(0, 1);
    sprintf(buffer,"val %6d", stripe_val);      
    lcd.print(buffer);
    delay(20);
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  return;
}


void showEncoder() {
  timestampLastEvent = millis();
  
  while (millis() - timestampLastEvent < 2000){
    char buffer[6];
    lcd.setCursor(0, 1);
    checkEncoder();  
    
    sprintf(buffer,"%6d", encPos);      
    lcd.print(buffer);
    delay(20);
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  return;
}

void testServo() {
  lcd.setCursor(0, 1);
  lcd.print(0);
  timestampLastEvent = millis();
  int servoPos = 0;
  servo.write(servoPos);
  delay(200);
  
  while (millis() - timestampLastEvent < 2000){
    char buffer[6];
    int newServoPos = servoPos + checkEncoder();  
    newServoPos = max(newServoPos, 0);
    newServoPos = min(newServoPos, 180);
    
    if (newServoPos != servoPos) {
      servoPos = newServoPos;
      servo.write(servoPos);
      delay(10);
      sprintf(buffer,"%6d", servoPos);      
      lcd.setCursor(0, 1);
      lcd.print(buffer);
    }
    
  }

  // timeout - end of menu
  servo.write(0);
  delay(100);
  timestampLastEvent = millis();
  return;
}

// show input of ir sensor (binary state < 100 if object within 2-8 cm)
void showIRSensor() {
  timestampLastEvent = millis();
  
  while (millis() - timestampLastEvent < 5000){
    char buffer[16];
    lcd.setCursor(0, 1);  
    sprintf(buffer,"1: %4d 2: %4d ", analogRead(IR_SENSOR_1_PIN), analogRead(IR_SENSOR_2_PIN));      
    lcd.print(buffer);
    delay(20);
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  return;
}


// show tonearm position
void showTonearmPos() {
  timestampLastEvent = millis();
  
  while (millis() - timestampLastEvent < 5000){
    lcd.setCursor(0, 1);  
    switch (checkTonearm()) {
      case TONEARM_NORMPOS:
        lcd.print("normal          ");
        break;
      case TONEARM_ENDPOS:
        lcd.print("end             ");
        break;
      case TONEARM_STARTPOS:
        lcd.print("start           ");
        break;
      default:
        lcd.print("not sure - wait ");
    }
    delay(100);
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  return;
}

// show color sensor
void showColorSensor() {
  timestampLastEvent = millis();
  char buffer[16];
  uint16_t r, g, b, c;
  
  while (millis() - timestampLastEvent < 20000) {
    lcd.setCursor(0, 1);
    digitalWrite(COLOR_SENSOR_LED_PIN, HIGH);
    delay(10);  
    tcs.getRawData(&r, &g, &b, &c);
    digitalWrite(COLOR_SENSOR_LED_PIN, LOW);  
    sprintf(buffer,"%3d %3d %3d %3d", r, g, b, c);      
    lcd.print(buffer);
    
    strip.setPixelColor(1, strip.Color(r, g, b));
    strip.show();  
    
    delay(100);
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  return;
}


void menu() {
  unsigned int menu_mode = MENU_MENU_SELECT;
    
  lcd.backlight();
  lcd.setCursor(0, 0);  
  lcd.print(menu_strings[menu_position]);
  while (millis() - timestampLastEvent < 10000) {
    int encoder_return = checkEncoder();

    // encoder changed
    if (encoder_return != 0) {
       menu_mode = MENU_MENU_SELECT;     
      if ((encoder_return > 0) && (menu_position < 8)) {
        menu_position +=1;
        lcd.setCursor(0, 0);  
        lcd.print(menu_strings[menu_position]);
      }
     else if ((encoder_return < 0) && (menu_position > 0)) {
        menu_position -=1;
        lcd.setCursor(0, 0);  
        lcd.print(menu_strings[menu_position]);
     }
  
   }
   // no change of encoder
   else {
    // enter menu entry
    if ((menu_mode == MENU_MENU_SELECT) && (millis() - timestampLastEvent > 2000)) {
      switch (menu_position) {
        case 0: // Color
          setStripeHue();
          break;
        case 1: // Saturation
          break;
        case 2: // Brightness
          setStripeVal();
          break;
        case 3: // Autocolor
          break;
        case 4: // Test Servo
          testServo();
          break;
        case 5: // Test IR Sensors
          showIRSensor();
          break;
        case 6: // Test Tornarm
          showTonearmPos();
          break;
        case 7: // Test Color Sens
          showColorSensor();
          break;
        case 8: // write to eeprom
          writeToEeprom();
        default:
        ;

      }
      lcd.setCursor(0, 1);
      lcd.print("                ") ;
      menu_mode = MODE_MENU_IDLE;
    }
   }
   delay(100);   
  }

  // timeout - end of menu
  timestampLastEvent = millis();
  lcd.noBacklight();
  return;
 
}




// the loop function runs over and over again forever
void loop() {
  

  //checkTonearm();

  // debug encoder
  if (checkEncoder() != 0) {
    menu();
    // showIRSensor();
    // showTonearmPos();
    // showEncoder();
    // testServo();
  };
  
  //if  {
  //  strip.fill(strip.ColorHSV(stripe_hue * 256, stripe_sat, stripe_val), 0, LED_COUNT);
  //}

  delay(100);
  
  
}
