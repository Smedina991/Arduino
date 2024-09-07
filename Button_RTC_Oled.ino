// For OLED module
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels. 32 for full size, 16 if you use the smaller display
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// For RTC Module
#include <SPI.h>
#include <RTClib.h>
//#include <DS3231.h>
//#define DS3231_I2C_ADDRESS 0x68
RTC_DS3231 myclock;
DateTime now;

// ---------- MAIN USER DEFINES SETTINGS FOR INPUTS -----------
#define numinputs 4                    // Set the pins for inputs from buttons (CURRENTLY EXTERNAL PULLUPS)
const byte inpins[] = { 2, 3, 4, 6 };  // define your input pins here. Using array allows for loops to handle the reads.
#define inputidle 1                    // idle voltage for input pins. 0 if you are pulling down, 1 if you are pulling up w resist.
byte debounce = 20;                    // Debounce time for inputs so they don't trigger multiple times.
word longpresstime = 600;              // Time for long presses, so it goes into a different option rather than a standard short press
word timeout = 300;                    // Time for stopping the current multipress check
byte isjoystick = 0;                   // 0 is for buttons, 1 is for yes this is a joystick. Joysticks will need additional settings to set correct idle state and low and high trigger values.
word oleddelay = 100;                  // delays the updating of OLED so it's not flashing new data and spending cycles constantly
word oledflashdelay = 300;
word rtcdelay = 3000;  // delays the checking of RTC module so it's not always polling

// ------------------------------------------------------------

// Current states of the inputs. Use this value when using digitalRead
byte instate[numinputs];
byte inlast[numinputs];  // The last state of the button. Used for comparing if the button was just now pressed or released.
byte numpresses[numinputs];
unsigned long timenow = 0;            // Time variable for math time now vs time later
unsigned long intime[numinputs];      // the time the button was last interacted with - for long press and timeout timers
unsigned long indebounce[numinputs];  // Time (millis) setting for debounce timers
unsigned long longover[numinputs];    // Time when the long press will trigger. 1 is parking value to stop checking
unsigned long timeover;               // Time when the program will stop checking for multiple presses. 1 is parking value to stop checking.
// ------------------------There is only one multipress timer for all inputs, so you can use a combination of inputs without losing the multipress values of other buttons.

// Oled settings
boolean oledon = 1;
unsigned long olednow = 0;  // timer to allow delay between checking and updating oled
unsigned long oledflash = 0;

// -------------------------------------------------

// RTC setting
boolean clockon = 0;
byte hourvalue = 0;
byte minutevalue = 0;
byte secondvalue = 0;
byte dayvalue = 1;
byte monthvalue = 1;
word yearvalue = 2024;
unsigned long rtcnow = 0;  // timer to allow delay between checking and updating rtc

byte timersset = 0;  // how many timers are set. Will change as the user adds timer triggers.
byte currenttimer = 0;
int timers[10];

// --------------------------------------------------

// Menu values
byte menuoptions = 4 + timersset;  // the maximum number of menu items written here. Checked for out of bounds entries.
byte menunumber = 0;               // which menu you are currently on.
byte menuzerosubnumber = 0;        // which sub option you are on for menu zero
byte menuonesubnumber = 0;         // which sub number you are on for menu one
byte menutwosubnumber = 0;

#define menu_mainmenu 0
#define menu_setclock 1
#define menu_triggermenu 2
boolean idlescreen = true;

// --------------------------------------------------

void setup() {

  Serial.begin(9600);

  // OLED Startups
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (oledon == 1) {
    delay(2000);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      oledon = 0;
    } else {
      Serial.println("Display *Should* be good.");
      display.clearDisplay();  // clear display

      display.setTextSize(2);          // text size
      display.setTextColor(WHITE);     // text color
      display.setCursor(0, 10);        // position to display
      display.println("Plant Life!");  // text to display
      display.display();               // show on OLED
    }
  }

  //for RTC module
  Wire.begin();
  delay(1000);  // Wait so everything can actually begin
  // for RTC Clock
  if (clockon == 1) {
    if (!myclock.begin()) {
      clockon = 0;
      //Serial.println("Clock did not start");
    } else {
      if (myclock.lostPower()) {
        myclock.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Auto set time based on system clock when you compile
      }
    }
  }

  // for inputs
  for (byte i = 0; i < numinputs; i++) {
    pinMode(inpins[i], INPUT);
    instate[i] = inputidle;
    inlast[i] = inputidle;
    intime[i] = 0;
    indebounce[i] = 0;
    longover[i] = 1;  // park the long hold timer as you aren't yet holding
    timeover = 1;     // park the multipress timer as you aren't currently multipressing
    numpresses[i] = 0;
  }
}

void loop() {
  checkInput();

  if (idlescreen == true) {
    if (timeover != 1)
      timeover = 1;
    if (numpresses[3] > 0) {  // forward/select button is pressed
      showmemenu();
      idlescreen = false;
      inputreset();
    }
  } else {
    //gettime();
    // checktriggers();
    if (menunumber == menu_setclock) {
      setclockmenu();
    } else if (menunumber == menu_triggermenu) {
      triggermenu();
    } else {
      mainmenu();
    }
  }
}

void gettime() {
  if (clockon == 1) {
    timenow = millis();
    if (rtcnow < timenow) {
      hourvalue = now.hour();
      minutevalue = now.minute();
      secondvalue = now.second();
      rtcnow = timenow + rtcdelay;
    }
  }
}

void settime() {
  if (clockon == 1) {
    myclock.adjust(DateTime(yearvalue, monthvalue, dayvalue, hourvalue, minutevalue, secondvalue));
    Serial.println("Time Updated");
  }
}

void settimer() {
  Serial.print("In Function for setting timer: ");
  Serial.println(currenttimer);
  int tominutesmath = hourvalue * 60;
  tominutesmath = tominutesmath + minutevalue;
  timers[currenttimer] = tominutesmath;
  Serial.print("Time written as: ");
  Serial.println(timers[currenttimer]);
}

void checkInput() {
  boolean multion = 1;     // prepping to make this an arguement check. Can give the option to ignore multi presses.
  boolean longholdon = 1;  // same here, can give the option to ignore long presses.
  // I'm seeing that it's not practical to pass arrays as parameters (dynamic memory and such) so having the checks run and update global values is the best choice sadly.
  // Just ignore multi presses or long holds in the originating function.

  timenow = millis();

  for (int i = 0; i < numinputs; i++) {     // check each input for activity
    if (indebounce[i] < timenow) {          // if you have passed the debounce time and are ready to read again
      inlast[i] = instate[i];               // record the last state before updating
      instate[i] = digitalRead(inpins[i]);  // update the current button state
    }
  }
  // AFTER ALL INPUTS HAVE BEEN UPDATED, check if you are taking action on changes.
  // This could have been bundled with the action above, but I want to ensure all pins are updated before taking action.
  // This prevents one pin from taking priority if a process takes too long to act on, potentially shadowing a short press
  for (int i = 0; i < numinputs; i++) {  // check all inputs for changes and act on that
    if (indebounce[i] < timenow) {       // validate that you are not checking this specific button during a bounce

      // If the state has changed
      if (inlast[i] != instate[i]) {

        intime[i] = timenow;                 // Since the state changed, update the time the input changed
        indebounce[i] = timenow + debounce;  // Since the state changed, and you have taken action, update the debounce time

        if (instate[i] == inputidle) {  // if the input is idle i.e. there is no active input
          if (multion == 1)
            timeover = timenow + timeout;  // set the timeout value
          else
            timeover = timenow;
          longover[i] = 1;  // park the long press value

          //Serial.print("Button ");
          //Serial.print(i);
          //Serial.println(" Just released.");

        } else {         // if the input is not idle i.e. there is active input
          timeover = 1;  // park the timeout value

          if (longholdon == 1)
            longover[i] = timenow + longpresstime;  // set the long press value
          else
            longover[i] = 1;
          if (multion == 1) {   // if you are counting multiple presses
            numpresses[i]++;    // increment the number of presses
          } else                // if you are not counting multiple presses
            numpresses[i] = 1;  // always just set the press to 1.

          //Serial.print("Button ");
          //Serial.print(i);
          //Serial.println(" Just pressed.");
        }
      }                                                                                                    // if the state has not changed, check for other valid actions
      else if (longholdon == 1 && instate[i] != inputidle && longover[i] != 1 && longover[i] < timenow) {  // LONGPRESS =  if the input is not idle, longover is not parked, and it has been this way for at least the longpress time
        Serial.println("Long Press detected, taking action then clearing counters.");
        //inputaction();  // Act then park long timer and clear press counter. This ensures multple presses then hold are still acted on, but will clear all data after that hold.
        inputreset();
        //       longover[i] = 1;                       // park the long compare timer as you have already reached the condition.
        //       for (int j = 0; j < numinputs; j++) {  // reset all button presses since long has been reached (the last action allowed in any sequence)
        //         numpresses[j] = 0;
        //       }
      } else if (instate[i] == inputidle && timeover != 1 && timeover < timenow) {  // NO PRESS = if the input is idle, timeover is not parked, and the timeout time has been passed
        Serial.println("Timeout reached, acting on presses then resetting press count.");
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

void mainmenu() {
  timenow = millis();
  if (olednow < timenow) {    // if it's time to update the screen.
    if (numpresses[3] > 0) {  // forward/select button is pressed
      if (menuzerosubnumber == 0) {
        Serial.println("View Clock - not set.");
        showmemenu();
      } else if (menuzerosubnumber == 1) {
        menunumber = menu_setclock;
        Serial.println("Going to clock setting menu. Start by setting hours.");
      } else if (menuzerosubnumber == menuoptions - 1) {
        Serial.println("Display sleep - not set.");
        showmemenu();
      } else if (menuzerosubnumber == menuoptions - 2 && timersset < 10) {
        Serial.print("New timer ");
        Serial.print(timersset + 1);
        Serial.println(" added, set now.");
        timersset++;
        menuoptions++;
        Serial.println("Going to timer setting menu. Start by setting hours.");
        menunumber = menu_setclock;
        currenttimer = menuzerosubnumber - 1;
      } else if (menuzerosubnumber == menuoptions - 2 && timersset == 10) {
        Serial.println("Max timers set, please modify other timer.");
      } else {
        Serial.println("Going to timer menu.");
        menunumber = menu_triggermenu;
        currenttimer = menuzerosubnumber - 1;
        menutwosubnumber = 0;
        showmemenu();
      }
    } else if (numpresses[1] > 0) {  // down button is pressed
      if (menuzerosubnumber < menuoptions - 1) {
        menuzerosubnumber++;
        showmemenu();
      }
    } else if (numpresses[2] > 0) {  // up button is pressed
      if (menuzerosubnumber > 0) {
        menuzerosubnumber--;
        showmemenu();
      }
    } else if (numpresses[0] > 0) {
      menuzerosubnumber = 0;
      inputreset();
      Serial.println("Screen going to idle");
      idlescreen = true;
    }

    inputreset();  // all inputs processed, clear old data.

    if (oledon == 1) {        // if you are allowing the display to display
      if (menunumber == 0) {  // make sure you didn't select another menu in the above button check. If you are still here, display
        if (menuzerosubnumber == 0) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.setTextSize(1);
          display.print("Menu:");
          display.setTextSize(2);
          display.setCursor(5, 10);
          display.print("View Clock");
          display.display();
        } else if (menuzerosubnumber == 1) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.setTextSize(1);
          display.print("Menu:");
          display.setTextSize(2);
          display.setCursor(5, 10);
          display.print("Set Clock");
          display.display();
        } else if (menuzerosubnumber == menuoptions - 2) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.setTextSize(1);
          display.print("Menu:");
          display.setTextSize(2);
          display.setCursor(5, 10);
          display.print("New Timer");
          display.display();
        } else if (menuzerosubnumber == menuoptions - 1) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.setTextSize(1);
          display.print("Menu:");
          display.setTextSize(2);
          display.setCursor(5, 10);
          display.print("Screen Off");
          display.display();
        } else if (menuzerosubnumber < menuoptions - 2) {  // showing added timers
          display.clearDisplay();
          display.setCursor(0, 0);
          display.setTextSize(1);
          display.print("Menu:");
          display.setTextSize(2);
          display.setCursor(5, 10);
          display.print("Timer ");
          display.print(menuzerosubnumber - 1);
          display.display();
        }
        olednow = timenow + oleddelay;
      }
    }
  }
}


void showmemenu() {

  if (menunumber == menu_mainmenu) {
    Serial.print("Menu: ");
    Serial.print(menuzerosubnumber);
    if (menuzerosubnumber == 0)
      Serial.println(" View Clock");
    else if (menuzerosubnumber == 1)
      Serial.println(" Set Clock");
    else if (menuzerosubnumber == menuoptions - 2)
      Serial.println(" New Timer");
    else if (menuzerosubnumber == menuoptions - 1)
      Serial.println(" Turn Off Screen");
    else if (menuzerosubnumber < menuoptions - 2) {
      Serial.print(" Configure Timer: ");
      Serial.println(menuzerosubnumber - 1);
    }
  } else if (menunumber == menu_triggermenu) {
    Serial.print("Triggers for timer ");
    Serial.print(currenttimer);
    Serial.print(" menu item: ");
    Serial.print(menutwosubnumber);
    if (menutwosubnumber == 0)
      Serial.println(" Idk yet...");
    else if (menutwosubnumber == 1)
      Serial.println(" Idk yet...");
    else if (menutwosubnumber == 2)
      Serial.println(" Idk yet...");
    else
      Serial.println("Sloppy code, you are outside your menu bounds.");
  }
}

void setclockmenu() {
  timenow = millis();
  if (olednow < timenow) {
    if (menuonesubnumber == 0) {  // if you are changing the hour
      if (numpresses[0] > 0) {    // if back is pressed, go back to main menu
        menunumber = 0;
        Serial.println("You have pressed back, going back to main menu.");
        showmemenu();
      } else if (numpresses[2] > 0) {
        hourvalue++;
        Serial.print("Increasing hours to: ");
        Serial.println(hourvalue);
      } else if (numpresses[1] > 0) {
        hourvalue--;
        Serial.print("Decreasing hours to: ");
        Serial.println(hourvalue);
      } else if (numpresses[3] > 0) {
        inputreset();
        menuonesubnumber = 1;  // move forward to the minute setting
        Serial.println("Now setting Minutes.");
      }
    } else {                    // if you are changing the minute
      if (numpresses[0] > 0) {  // if back is pressed, move back to the hour selection
        menuonesubnumber = 0;
        Serial.println("Going back to set minutes.");
      } else if (numpresses[2] > 0) {
        minutevalue++;
        Serial.print("Increasing minutes to: ");
        Serial.println(minutevalue);
      } else if (numpresses[1] > 0) {
        minutevalue--;
        Serial.print("Decreasing minutes to: ");
        Serial.println(minutevalue);
      } else if (numpresses[3] > 0) {
        inputreset();
        if (currenttimer == 0) {
          settime();
          Serial.print("You have set the time as: ");
        } else {
          settimer();
          Serial.print("You have set timer ");
          Serial.print(currenttimer);
          Serial.print(" as: ");
          currenttimer = 0;
        }
        Serial.print(hourvalue);
        Serial.print(":");
        Serial.println(minutevalue);
        hourvalue = 0;
        minutevalue = 0;
        menunumber = menu_mainmenu;
        Serial.print("Going back to main menu ");
        Serial.println(menuzerosubnumber);
        menuonesubnumber = 0;
        showmemenu();
      }
    }

    inputreset();  // inputs have been received, clear the old data.

    if (menunumber == 1) {
      if (hourvalue > 23 && hourvalue < 253) {
        hourvalue = 0;
        Serial.println("Hours OOB, rolling over to: 0");
      } else if (hourvalue < 0 || hourvalue > 253) {
        hourvalue = 23;
        Serial.println("Hours OOB, rolling over to: 23");
      }
      if (minutevalue > 59 && minutevalue < 253) {
        minutevalue = 0;
        Serial.println("Minutes OOB, rolling over to: 0");
      } else if (minutevalue < 0 || minutevalue > 253) {
        minutevalue = 59;
        Serial.println("Minutes OOB, rolling over to: 59");
      }


      if (oledon == 1) {  // if you are allowing the display to display
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        if (currenttimer == 0)
          display.print("Set Time:");
        else {
          display.print("Set Timer: ");
          display.print(currenttimer);
        }
        display.setTextSize(2);
        display.setCursor(35, 10);

        if (menuonesubnumber == 0 && oledflash < timenow) {
          if (hourvalue < 10)
            display.print('0');
          display.print(hourvalue);
          oledflash = timenow + oledflashdelay;
        } else if (menuonesubnumber == 1) {
          if (hourvalue < 10)
            display.print('0');
          display.print(hourvalue);
        }

        display.setCursor(60, 10);
        display.print(":");
        display.setCursor(73, 10);


        if (menuonesubnumber == 1 && oledflash < timenow) {
          if (minutevalue < 10)
            display.print('0');
          display.print(minutevalue);
          oledflash = timenow + oledflashdelay;
        } else if (menuonesubnumber == 0) {
          if (minutevalue < 10)
            display.print('0');
          display.print(minutevalue);
        }
        display.display();
      }
    }

    olednow = timenow + oleddelay;
  }
}

void triggermenu() {
  //Serial.println("Whomp Whomp, your code is in another castle.");
  //menunumber = menu_mainmenu;
  //showmemenu();

  timenow = millis();
  if (olednow < timenow) {
    if (numpresses[0] > 0) {  // if back is pressed, go back to main menu
      menunumber = menu_mainmenu;
      Serial.println("You have pressed back, going back to main menu.");
      showmemenu();
    } else if (numpresses[1] > 0) {  // down button is pressed
      if (menutwosubnumber < 2) {
        menutwosubnumber++;
        showmemenu();
      }
    } else if (numpresses[2] > 0) {  // up button is pressed
      if (menutwosubnumber > 0) {
        menutwosubnumber--;
        showmemenu();
      }
    } else if (numpresses[3] > 0) {  // if select/next button is pressed
      if (menutwosubnumber == 0) {
        Serial.println("Idk, something about your selection. Whatever you selected is good.");
        showmemenu();
      } else if (menutwosubnumber == 1) {
        Serial.println("Idk, something about your selection. Whatever you selected is good.");
        showmemenu();
      } else if (menutwosubnumber == 2) {
        Serial.println("Idk, something about your selection. Whatever you selected is good.");
        showmemenu();
      }
    }
    inputreset();
  }
}
