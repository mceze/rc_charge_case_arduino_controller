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
const int OU0_PIN = A0;
const int OU1_PIN = A1;
const int PS0_PIN = A2;
const int PS1_PIN = A3;
const int FAN_PIN = 7;

const double Vref = 3.8;
const int NSAMPLES = 15;

// these are state boolean used to flash the  numbers on the display
bool PS0 = true;
bool PS1 = true;
bool OU0 = true;
bool OU1 = true;
bool FAN_ON = true;

bool FLASH_PS0 = false;
bool FLASH_PS1 = false;
bool FLASH_OU0 = false;
bool FLASH_OU1 = false;

bool PS0_HIGH = false;
bool PS1_HIGH = false;
bool OU0_HIGH = false;
bool OU1_HIGH = false;

// put state variables for buttons here
int screen_state = 0;

class MovingAverage {
  public:
    MovingAverage(const float InitialValue)
    {
      k = 0;
      for (int i = 0; i < NSAMPLES; i++) 
      {
        samples[i] = 0.0;  
      }
      mean = Update(InitialValue);
    }
    ~MovingAverage() {}
    float Update(const float NewValue)
    {
        // make space for new value
        for (int i = k; i < NSAMPLES-1; i++) {
          samples[i+1] = samples[i];
        }
        samples[k] = NewValue;
        float sum = 0;  
        for (int i = 0; i < k + 1; i++) {
          sum += samples[i];  
        }
        mean = sum / (k+1);
        k = (k+1) % NSAMPLES;

        return mean;
    }
  private:
    float mean;
    float samples[NSAMPLES];
    int k;
    
};



float ps0_temp;
float ps1_temp;
float ou0_temp;
float ou1_temp;

MovingAverage ps0_filter(0.0), ps1_filter(0.0), ou0_filter(0.0), ou1_filter(0.0);

// digital control parameters. 
// these will be settable in a page of the display with + - and enter buttons
float TEMP_HYSTERISIS = 5.0;
float TEMP_THRESHOLD = 30.0;
float TEMP_FLASH = 50.0;

void setup()   {               
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // This comes from the display example
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128X64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // setup and initialize IO pins
  pinMode(FAN_PIN,OUTPUT);

  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  
  digitalWrite(FAN_PIN, LOW);
}

void read_temp()
{
  // PS0
  int reading = analogRead(PS0_PIN);
  float voltage = (reading * Vref)/1024;
  
  ps0_temp = ps0_filter.Update((voltage - 0.5) * 100);
  // PS1
  reading = analogRead(PS1_PIN);
  voltage = (reading * Vref)/1024;
  ps1_temp = ps1_filter.Update((voltage - 0.5) * 100);

  // OU0
  reading = analogRead(OU0_PIN);
  voltage = (reading * Vref)/1024;
  ou0_temp = ou0_filter.Update((voltage - 0.5) * 100);
  // OU1
  reading = analogRead(OU1_PIN);
  voltage = (reading * Vref)/1024;
  ou1_temp = ou1_filter.Update((voltage - 0.5) * 100);
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

String get_bool_sub_string(const bool temp, bool flash)
{
  String buff;
  char cbuff[16];
  char str_temp[16];
  if (temp) {
    sprintf(cbuff,"ON",str_temp);
  }
  else {
    sprintf(cbuff,"OFF",str_temp);
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
    const String HEADER = "PSU Outlet";
    display.setCursor(0,0);
    display.println(HEADER);
    display.setTextSize(2);
    display.setCursor(0,16);
    display.println(get_temp_sub_string(ps0_temp,FLASH_PS0,PS0) + "C  " + get_temp_sub_string(ou0_temp,FLASH_OU0,OU0) + "C");
    display.setCursor(0,32);
    display.println(get_temp_sub_string(ps1_temp,FLASH_PS1,PS1) + "C  " + get_temp_sub_string(ou1_temp,FLASH_OU1,OU1) + "C");
    display.setCursor(0,48);
    display.println("FAN: " + get_bool_sub_string(FAN_ON,false));
    display.display();
}

void apply_threshold(const float value, const float threshold, bool& state)
{
  if (value >= threshold) {
    state = true;
  }
  if (state == true & value < threshold - TEMP_HYSTERISIS)
  {
    state = false;
  }
}

void process_state()
{
  apply_threshold(ps0_temp, TEMP_THRESHOLD, PS0_HIGH);
  apply_threshold(ps0_temp, TEMP_FLASH, FLASH_PS0);
  apply_threshold(ps1_temp, TEMP_THRESHOLD, PS1_HIGH);
  apply_threshold(ps1_temp, TEMP_FLASH, FLASH_PS1);
  apply_threshold(ou0_temp, TEMP_THRESHOLD, OU0_HIGH);
  apply_threshold(ou0_temp, TEMP_FLASH, FLASH_OU0);
  apply_threshold(ou1_temp, TEMP_THRESHOLD, OU1_HIGH);
  apply_threshold(ou1_temp, TEMP_FLASH, FLASH_OU1);

  if (PS0_HIGH || PS1_HIGH || OU0_HIGH || OU1_HIGH) {
    digitalWrite(FAN_PIN, HIGH);
    FAN_ON = true; 
  }
  else {
    digitalWrite(FAN_PIN, LOW); 
    FAN_ON = false;
  }
}
// main loop
void loop()
{
  // this sets the freequency of the loop
  delay(100);
  read_temp();
  
  monitor_screen();
  
  process_state();
}
