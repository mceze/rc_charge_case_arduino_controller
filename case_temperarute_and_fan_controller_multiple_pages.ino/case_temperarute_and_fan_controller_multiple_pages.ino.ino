#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
/*
#define OLED_MOSI   9 // display sda
#define OLED_CLK   10 // display scl
#define OLED_DC    11 // - display dc
#define OLED_CS    12
#define OLED_RESET 13 // display res
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define TEXT_SIZE 1

// define here the digital pin numbers
const int FWD_BUTTON_PIN = 12;
const int BWD_BUTTON_PIN = 11;
const int FAN_PIN = 6;
const int BUZZER_PIN = 5;

const int NUM_SCREEN = 3;

// processed commands from buttons
enum command {
  CMD_NONE,
  CMD_NEXT,
  CMD_PREV,
  CMD_ENT
};

// these are state boolean used to flash the  numbers on the display
bool IN0 = true;
bool IN1 = true;
bool PS0 = true;
bool PS1 = true;
bool OU0 = true;
bool OU1 = true;
bool TOGGLE_STATE = true;

bool TOGGLE_ACTIVE = false;

bool FLASH_IN0 = false;
bool FLASH_IN1 = false;
bool FLASH_PS0 = false;
bool FLASH_PS1 = false;
bool FLASH_OU0 = false;
bool FLASH_OU1 = false;

bool IN0_HIGH = false;
bool IN1_HIGH = false;
bool PS0_HIGH = false;
bool PS1_HIGH = false;
bool OU0_HIGH = false;
bool OU1_HIGH = false;

bool BUZZER_ON = true;

// put state variables for buttons here
int screen_state = 0;


// these are here for testing, they will be read from the sensors
float in0_temp = 50.0;
float in1_temp = 60.0;
float ps0_temp = 51.0;
float ps1_temp = 52.0;
float ou0_temp = 65.0;
float ou1_temp = 65.0;

// digital control parameters. 
// these will be settable in a page of the display with + - and enter buttons
float TEMP_HYSTERISIS = 5.0;
float TEMP_THRESHOLD = 25.0;

void setup()   {               
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // This comes from the display example
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128X64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // setup and initialize IO pins
  pinMode(FWD_BUTTON_PIN, INPUT);
  pinMode(BWD_BUTTON_PIN, INPUT);
  pinMode(FAN_PIN,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);

  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void read_temp()
{
  // IN0
  int reading = analogRead(A0);
  float voltage = (reading * 5.0)/1024;
  in0_temp = (voltage - 0.5) * 100;
  // IN1
  reading = analogRead(A1);
  voltage = (reading * 5.0)/1024;
  in1_temp = (voltage - 0.5) * 100;
}

// this builds the strings for temperature
String get_temp_sub_string(const float temp, bool flash, bool& state)
{
  if (flash) {
    state = !state;
  }
  else {
    state = true;
  }
  String buff;
  char cbuff[16];
  char str_temp[16];
  if (state) {
    dtostrf(temp, 2, 0, str_temp);
    sprintf(cbuff,"%s",str_temp);
  }
  else {
    sprintf(cbuff,"  ");
    
  }
  buff = cbuff;
  
  return String(buff);
}

String get_bool_sub_string(const bool temp, bool flash, bool& state)
{
  if (flash) {
    state = !state;
  }
  else {
    state = true;
  }
  String buff;
  char cbuff[16];
  char str_temp[16];
  if (state) {
    if (temp) {
      sprintf(cbuff,"ON",str_temp);
    }
    else {
      sprintf(cbuff,"OFF",str_temp);
    }
  }
  else {
    sprintf(cbuff," ");
  }
  buff = cbuff;
  
  return String(buff);
}

void monitor_screen()
{
    display.clearDisplay();
    display.setTextSize(TEXT_SIZE);
    display.setTextColor(WHITE); 
    display.clearDisplay();
    // header for the display
    display.setTextSize(2);
    const String HEADER = "In PSU Out";
    display.setCursor(0,0);
    display.println(HEADER);
    display.setTextSize(2);
    display.setCursor(0,16);
    display.println(get_temp_sub_string(in0_temp,FLASH_IN0,IN0) + "  " + get_temp_sub_string(ps0_temp,FLASH_PS0,PS0) + "  " + get_temp_sub_string(ou0_temp,FLASH_OU0,OU0));
    display.setCursor(0,32);
    display.println(get_temp_sub_string(in1_temp,FLASH_IN1,IN1) + "  " + get_temp_sub_string(ps1_temp,FLASH_PS1,PS1) + "  " + get_temp_sub_string(ou1_temp,FLASH_OU1,OU1));
    display.println("  Celsius");
    display.display();
}

void fan_temp_threshold_screen()
{
    display.clearDisplay();
    display.setTextColor(WHITE); 
    display.clearDisplay();
    // header for the display
    display.setTextSize(2);
    const String HEADER = " Fan ON @";
    display.setCursor(0,0);
    display.println(HEADER);
    display.setTextSize(2);
    display.setCursor(40,24);
    display.println(get_temp_sub_string(TEMP_THRESHOLD,TOGGLE_ACTIVE,TOGGLE_STATE) + " C");
    display.display(); 
}

void buzzer_toggle_screen()
{
    display.clearDisplay();
    display.setTextColor(WHITE); 
    display.clearDisplay();
    // header for the display
    display.setTextSize(2);
    const String HEADER = "  Buzzer ";
    display.setCursor(0,0);
    display.println(HEADER);
    display.setTextSize(2);
    display.setCursor(40,24);
    display.println(get_bool_sub_string(BUZZER_ON,TOGGLE_ACTIVE,TOGGLE_STATE));
    display.display(); 
}

void process_command(command& cmd)
{
    cmd = CMD_NONE;
    if (digitalRead(BWD_BUTTON_PIN) == HIGH && digitalRead(FWD_BUTTON_PIN) == LOW) {
      cmd = CMD_PREV;
      delay(500);
    }
    if(digitalRead(BWD_BUTTON_PIN) == LOW && digitalRead(FWD_BUTTON_PIN) == HIGH) {
      cmd = CMD_NEXT;
      delay(500);
    }
    if(digitalRead(BWD_BUTTON_PIN) == HIGH && digitalRead(FWD_BUTTON_PIN) == HIGH) { 
      cmd = CMD_ENT;
      delay(500);
    }
}

void apply_threshold(const float value, const float threshold, bool& state)
{
  state = (value >= TEMP_THRESHOLD);
}

void process_state()
{
  apply_threshold(in0_temp, TEMP_THRESHOLD, IN0_HIGH); FLASH_IN0 = IN0_HIGH;
  apply_threshold(in1_temp, TEMP_THRESHOLD, IN1_HIGH); FLASH_IN1 = IN1_HIGH;
  apply_threshold(ps0_temp, TEMP_THRESHOLD, PS0_HIGH); FLASH_PS0 = PS0_HIGH;
  apply_threshold(ps1_temp, TEMP_THRESHOLD, PS1_HIGH); FLASH_PS1 = PS1_HIGH;
  apply_threshold(ou0_temp, TEMP_THRESHOLD, OU0_HIGH); FLASH_OU0 = OU0_HIGH;
  apply_threshold(ou1_temp, TEMP_THRESHOLD, OU1_HIGH); FLASH_OU1 = OU1_HIGH;
  
}
// main loop
void loop()
{
  // this sets the freequency of the loop
  delay(100);
  read_temp();

  if (screen_state < 0) {
    screen_state = NUM_SCREEN - 1;
  }
  else {
    screen_state = screen_state  % NUM_SCREEN;
  }
  
  if (screen_state == 0) {
    monitor_screen();
    command cmd;
    process_command(cmd);
    switch (cmd) {
      case CMD_PREV:
        screen_state--;
        break;
      case CMD_NEXT:
        screen_state++;
        break;
      default:
        break;
    }
  }
  else if (screen_state == 1) {
    fan_temp_threshold_screen();
    command cmd;
    process_command(cmd);
    switch (cmd) {
      case CMD_PREV:
        if (TOGGLE_ACTIVE) {
          TEMP_THRESHOLD -= 5.0;
        }
        else {
          screen_state--;
        }
        break;
      case CMD_NEXT:
        if (TOGGLE_ACTIVE) {
          TEMP_THRESHOLD += 5.0;
        }
        else {
          screen_state++;
        }
        break;
      case CMD_ENT:
        TOGGLE_ACTIVE = !TOGGLE_ACTIVE;
        break;
      default:
        break;
    }
  }
  else if (screen_state == 2) {
    buzzer_toggle_screen();
    command cmd;
    process_command(cmd);
    switch (cmd) {
      case CMD_PREV:
        if (TOGGLE_ACTIVE) {
          BUZZER_ON = !BUZZER_ON;
        }
        else {
          screen_state--;
        }
        break;
      case CMD_NEXT:
        if (TOGGLE_ACTIVE) {
          BUZZER_ON = !BUZZER_ON;
        }
        else {
          screen_state++;
        }
        break;
      case CMD_ENT:
        TOGGLE_ACTIVE = !TOGGLE_ACTIVE;
        break;
      default:
        break;
    }
  }

  process_state();
  
//  if (in0_temp >= TEMP_THRESHOLD) {
//    digitalWrite(FAN_PIN, HIGH);
//    //tone(BUZZER_PIN, 1000,100);
//  }
//  else {
//    //noTone(BUZZER_PIN); 
//  }
//  if (in0_temp < TEMP_THRESHOLD - TEMP_HYSTERISIS) {
//    digitalWrite(FAN_PIN, LOW);
//  }
  
}
