// SCL to 3 on pro micro, A5 on uno
// SDA to 2 on pro micro, A4 on uno
// VCC to 5V
// GND to GND
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// For OLED module
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <splash.h>                  //why
#include <Adafruit_GrayOLED.h>       //why
#include <Adafruit_SPITFT.h>         //why
#include <Adafruit_SPITFT_Macros.h>  //why
#include <gfxfont.h>                 //why
#define SCREEN_WIDTH 128             // OLED display width, in pixels
#define SCREEN_HEIGHT 32             // OLED display height, in pixels. 32 for full size, 16 if you use the smaller display
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// For RTC Module
#include <RTClib.h>
//#include <DS3231.h>
//#define DS3231_I2C_ADDRESS 0x68
RTC_DS3231 myclock;
DateTime now;


// ---------- MAIN USER DEFINES SETTINGS FOR INPUTS -----------
#define numinputs 4                   // Set the pins for inputs from buttons (CURRENTLY EXTERNAL PULLUPS)
const int inpins[] = { 2, 3, 4, 6 };  // define your input pins here. Using array allows for loops to handle the reads.
#define inputidle 1                   // idle voltage for input pins. 0 if you are pulling down, 1 if you are pulling up w resist.
int debounce = 20;                    // Debounce time for inputs so they don't trigger multiple times.
unsigned long longpresstime = 600;    // Time for long presses, so it goes into a different option rather than a standard short press
unsigned long timeout = 300;          // Time for stopping the current multipress check
int isjoystick = 0;                   // 0 is for buttons, 1 is for yes this is a joystick. Joysticks will need additional settings to set correct idle state and low and high trigger values.
unsigned long oleddelay = 100;        // delays the updating of OLED so it's not flashing new data and spending cycles constantly
unsigned long rtcdelay = 1000;        // delays the checking of RTC module so it's not always polling
// ------------------------------------------------------------

//Joystick Settings
int joysticklow[] = { 80, 80 };     // Threshold for low joystick. This differs for each joystick so call each from an array.
int joystickhigh[] = { 260, 260 };  // Threshold for high joystick. This differs for each joystick so call each from an array.
// ------------------------------------------------------------

// Current states of the inputs. Use this value when using digitalRead
int instate[numinputs];
int inlast[numinputs];  // The last state of the button. Used for comparing if the button was just now pressed or released.
int numpresses[numinputs];
unsigned long timenow = 0;            // Time variable for math time now vs time later
unsigned long intime[numinputs];      // the time the button was last interacted with - for long press and timeout timers
unsigned long indebounce[numinputs];  // Time (millis) setting for debounce timers
unsigned long longover[numinputs];    // Time when the long press will trigger. 1 is parking value to stop checking
unsigned long timeover;               // Time when the program will stop checking for multiple presses. 1 is parking value to stop checking.
// ------------------------There is only one multipress timer for all inputs, so you can use a combination of inputs without losing the multipress values of other buttons.

// Oled settings
int oledon = 1;
int scrolltextpos = SCREEN_WIDTH;  // value to compare for scrolling text only
unsigned long olednow = 0;         // timer to allow delay between checking and updating oled

int menunumber = 0;  // which menu you are currently on.
// -------------------------------------------------

// RTC setting
int clockon = 1;
int hourvalue = 0;
int minutevalue = 0;
int secondvalue = 0;
int dayvalue = 1;
int monthvalue = 1;
unsigned long rtcnow = 0;  // timer to allow delay between checking and updating rtc
// --------------------------------------------------

void setup() {

  Serial.begin(9600);

  //for RTC module
  Wire.begin();
  delay(1000);  // Wait so everything can actually begin

  // for inputs
  for (int i = 0; i < numinputs; i++) {
    pinMode(inpins[i], INPUT);
    instate[i] = inputidle;
    inlast[i] = inputidle;
    intime[i] = 0;
    indebounce[i] = 0;
    longover[i] = 1;  // park the long hold timer as you aren't yet holding
    timeover = 1;     // park the multipress timer as you aren't currently multipressing
    numpresses[i] = 0;
  }

  // For OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    oledon = 0;
  }
  if (oledon == 1) {
    delay(2000);  // Wait for the display initialize
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setTextWrap(false);
    display.setCursor(0, 10);
    // Display startup text
    display.println("Hello!");
    display.display();
  }

  if (!myclock.begin()) {
    clockon = 0;
    Serial.println("Clock did not start");
  }

  if (clockon == 1) {
    // Pull the time from the module. Don't know what happens if it hasn't been set.

    now = myclock.now();

    if (myclock.lostPower()) {
      myclock.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Auto set time based on system clock when you compile
    }
    //myclock.setClockMode(false);  // keep the clock in 24 hour mode
    //byte hourhere = myclock.getHour(false, false);
    //int minhere = myclock.getMinute();
    //int sechere = myclock.getSecond();
  }
}

void loop() {
  checkInput();
  if (clockon == 1)
    gettime();
  if (oledon == 1)
    oledtester();
}

void settime() {
  // myclock.adjust(DateTime(yearvalue, monthvalue, dayvalue, hourvalue, minutevalue, secondvalue));
}

void gettime() {

  timenow = millis();

  if (rtcnow < timenow) {
    hourvalue = now.hour();
    minutevalue = now.minute();
    secondvalue = now.second();
    //Serial.print(hourhere);
    //Serial.print(" , ");
    //Serial.print(minhere);
    //Serial.print(" , ");
    //Serial.println(sechere);

    rtcnow = timenow + rtcdelay;
  }
}

void oledtester() {

  timenow = millis();

  if (olednow < timenow) {


    /*
    char message[] = "Something";
    int incrementNumber = 0;
    char twodigitone[] = { '0', '0' };
    char twodigittwo69[] = { '0', '0' };
    int x = 0;
    int minX = -12 * strlen(message);  // 12 = 6 pixels/character * text size 2
    if (incrementNumber > 19)
      incrementNumber = 0;
    int digitone = 0;
    int digittwo = 0;
    if (digitone >= 60)
      digitone = 0;
    if (digittwo >= 60)
      digittwo = 0;
*/

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Time On:");
    display.setTextSize(2);
    display.setCursor(35, 10);
    if (numpresses[0] < 10)
      display.print('0');
    display.print(numpresses[0]);
    display.setCursor(60, 10);
    display.print(":");
    display.setCursor(73, 10);
    //if (incrementNumber % 2) { // originally here to show flashing. change this to timed.
    if (numpresses[3] < 10)
      display.print('0');
    display.print(numpresses[3]);
    //} // end of flashing if statement
    display.display();

    //x = x - 1;  // scroll speed, make more positive to slow down the scroll
    //if (x < minX) x = 64;
    //if (x < minX) x = display.width();
    //incrementNumber++;
    //if (incrementNumber % 2)
    //  digittwo++;

    olednow = timenow + oleddelay;
    //Serial.print("OLED Updating: ");
    //Serial.println(millis());
  }
}

void checkInput() {
  int multion = 1;     // prepping to make this an arguement check. Can give the option to ignore multi presses.
  int longholdon = 1;  // same here, can give the option to ignore long presses.
  // I'm seeing that it's not practical to pass arrays as parameters (dynamic memory and such) so having the checks run and update global values is the best choice sadly.
  // Just ignore multi presses or long holds in the originating function.

  timenow = millis();

  for (int i = 0; i < numinputs; i++) {     // check each input for activity
    if (indebounce[i] < timenow) {          //if you have passed the debounce time and are ready to read again
      inlast[i] = instate[i];               // record the last state before updating
      instate[i] = digitalRead(inpins[i]);  // update the current button state
    }
  }
  // AFTER ALL INPUTS HAVE BEEN UPDATED, check if you are taking action on changes.
  // This could have been bundled with the action above, but I want to ensure all pins are updated before taking action.
  // This prevents one pin from taking priority if a process takes too long to act on, potentially shadowing a short press
  for (int i = 0; i < numinputs; i++) {  //check all inputs for changes and act on that
    if (indebounce[i] < timenow) {       // again, validate that you are not checking during a bounce
      if (inlast[i] != instate[i]) {     // If the state has changed

        intime[i] = timenow;                 // Since the state changed, update the time the input changed
        indebounce[i] = timenow + debounce;  // Since the state changed, and you have taken action, update the debounce time

        if (instate[i] == inputidle) {   // if the input is idle i.e. there is no active input
          timeover = timenow + timeout;  // set the timeout value
          longover[i] = 1;               // park the long press value
          /*
          Serial.print("Button ");
          Serial.print(i + 1);
          Serial.println(" was just released.");
          */
        } else {                                  // if the input is not idle i.e. there is active input
          timeover = 1;                           // park the timeout value
          longover[i] = timenow + longpresstime;  // set the long press value

          if (multion == 1) {   // if you are counting multiple presses
            numpresses[i]++;    // increment the number of presses
          } else                // if you are not counting multiple presses
            numpresses[i] = 1;  // always just set the press to 1.
          /*
          Serial.print("Button ");
          Serial.print(i + 1);
          Serial.print(" was just pressed. Count= ");
          Serial.println(numpresses[i]);
          */
        }
      }
      // if the state has not changed, check for other valid actions
      else if (longholdon == 1 && instate[i] != inputidle && longover[i] != 1 && longover[i] < timenow) {  // LONGPRESS =  if the input is not idle, longover is not parked, and it has been this way for at least the longpress time
        // Serial.println("Long Press detected, taking action then clearing counters.");
        //inputaction();  // Act then park long timer and clear press counter. This ensures multple presses then hold are still acted on, but will clear all data after that hold.
        inputreset();
        //       longover[i] = 1;                       // park the long compare timer as you have already reached the condition.
        //       for (int j = 0; j < numinputs; j++) {  // reset all button presses since long has been reached (the last action allowed in any sequence)
        //         numpresses[j] = 0;
        //       }
      } else if (instate[i] == inputidle && timeover != 1 && timeover < timenow) {  // NO PRESS = if the input is idle, timeover is not parked, and the timeout time has been passed
        // Serial.println("Timeout reached, acting on presses then resetting press count.");
        //inputaction();
        inputreset();
        // longover has already been parked since you released the button. You could repeat it here but not really necesary since you aren't breaking this into another func.
        //        timeover = 1;                          // park the timeout value as it is no longer looking.
        //        for (int j = 0; j < numinputs; j++) {  // reset all button presses since no inputs have been given in valid time
        //          numpresses[j] = 0;
      }
    }
  }
}

void inputreset() {
  for (int k = 0; k < numinputs; k++) {
    longover[k] = 1;    // park the long compare timer as you have already reached the condition.
    timeover = 1;       // park the timeout value as it is no longer looking.
    numpresses[k] = 0;  // reset all button presses since long has been reached (the last action allowed in any sequence)
  }
}
void inputaction() {  // take action for new inputs. Don't change values in here, that should be done when the action functon returns to the checker.
  for (int i = 0; i < numinputs; i++) {
    // if the state has not changed, act on long presses or timeouts.
    // the program will wait until the multipress timeout has occurred to take action on single presses to ensure it is not going to be a multipress
    // (it wouldn't have sent it here for action if it wasn't valid, but you need to find out which needs action)
    if (instate[i] != inputidle && longover[i] != 1 && longover[i] < timenow) {  // LONG PRESS =  if the input is not idle, longover is not parked, and it has been this way for at least the longpress time
      // Act on long press
    } else if (instate[i] == inputidle && timeover != 1 && timeover < timenow) {  // SINGLE OR MULTI PRESS = if the input is idle, timeover is not parked, and the timeout time has been passed
      // Act on single or multiple presses
    }
  }
}

void menuzero() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Menu:");
  display.setTextSize(2);
  display.setCursor(5, 10);
  display.print("Set Clock");
}