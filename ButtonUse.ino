// ---------- MAIN USER DEFINES SETTINGS FOR INPUTS -----------
#define numinputs 4                   // Set the pins for inputs from buttons (CURRENTLY EXTERNAL PULLUPS)
const int inpins[] = { 2, 3, 4, 5 };  // define your input pins here. Using array allows for loops to handle the reads.
#define inputidle 1                   // idle voltage for input pins. 0 if you are pulling down, 1 if you are pulling up w resist.
int debounce = 20;                    // Debounce time for inputs so they don't trigger multiple times.
unsigned long longpresstime = 600;    // Time for long presses, so it goes into a different option rather than a standard short press
unsigned long timeout = 1000;         // Time for stopping the current multipress check
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

void setup() {

  Serial.begin(9600);

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
}

void loop() {
  checkInput();
}

void checkInput() {

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
          Serial.print("Button ");
          Serial.print(i + 1);
          Serial.println(" was just released.");
          longover[i] = 1;  // park the long press value
        } else {            // if the input is not idle i.e. there is active input
          timeover = 1;     // park the timeout value
          numpresses[i]++;
          Serial.print("Button ");
          Serial.print(i + 1);
          Serial.print(" was just pressed. Count= ");
          Serial.println(numpresses[i]);
          longover[i] = timenow + longpresstime;  // set the long press value
        }

        inputaction();  // Take action on the change
      }

      // if the state has not changed, check for other valid actions
      else if (instate[i] != inputidle && longover[i] != 1 && longover[i] < timenow) {  // LONGPRESS =  if the input is not idle, longover is not parked, and it has been this way for at least the longpress time
        Serial.println("Long Press detected, taking action then clearing counters.");
        inputaction();  // Act then park long timer and clear press counter. This ensures multple presses then hold are still acted on, but will clear all data after that hold.
        longover[i] = 1;
        for (int j = 0; j < numinputs; j++) {  // reset all button presses since no inputs have been given in valid time
          numpresses[j] = 0;
        }
      } else if (instate[i] == inputidle && timeover != 1 && timeover < timenow) {  // NO PRESS = if the input is idle, timeover is not parked, and the timeout time has been passed
        Serial.println("Timeout reached, acting on presses then resetting press count.");
        inputaction();
        timeover = 1;
        for (int j = 0; j < numinputs; j++) {  // reset all button presses since no inputs have been given in valid time
          numpresses[j] = 0;
        }
      }
    }
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