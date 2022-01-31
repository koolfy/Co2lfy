/*
 * License file should be in the included LICENSE file
 * If not, just know this is GPLv3 code ;)
 *
 * v0.1-beta
 *
 * Made in a rush for friends, close ones and teachers around me
 * also a very cool teaching tool
 *
 * Let's wipe Covid from the face of the planet. 
 * It is not welcome on this solar system.
 *
 * TODO:
 * - easter eggs
 * - better serial debug
 */

#include <SPI.h>
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>

// Initialize co2 sensor
SensirionI2CScd4x scd4x;

// Initialize OLED display
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire1);

// Pin mapping for buttons A,B,C.
#define BUTTON_A  9
#define BUTTON_B  8
#define BUTTON_C  7

// last measurement sample timestamp
unsigned long previousMeasurement = 0;

// mesurement interval (less than 5s is probably unwise)
const int measurementInterval = 10000;

// whish display mode are we in
// used to enter different drawing logic modes
// display mode 1 is the default screen at startup
int displayMode = 1;

// blinking/etc timestamps:
unsigned long previousHealthAssessmentDisplayed = 0;

// button press cooldown to avoid multiple-presses when holding a button for a bit too long
unsigned long previousButtonPress = 0;

// timestamp for calibration timer
unsigned long calibrationStartTime = 0;
// calibration duration is 10 minutes so 10*60*1000 miliseconds
const unsigned long calibrationDuration = 600000;

// Value of the actual correction applied during forced recalibration
uint16_t atmCo2Correction;

// Measurement global variables
uint16_t co2;
float temperature;
float humidity;

int co2Increase = 0;

// Collection of data to graph, one point every 30s
int measurementArray[110];

// measurement array index, shifted as we store new values
unsigned long measurementArrayIndex = 0;

// Used to know when it's time to store a new history datapoint (30s)
unsigned long previousArrayMeasurement = 0;

void setup() {

    // Debug serial interface
    Serial.begin(115200);

    // Initiate I2C
    Wire1.begin();

    // Setup OLED display
    display.begin(0x3C, true);

    // Setup and reset co2 sensor
    uint16_t error;
    char errorMessage[256];
    scd4x.begin(Wire1);
    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    // CO2 sensor:
    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    // initialize all history measurement array values to 0
    for (int i=0; i<110; i++) {
        measurementArray[i] = 0;
    }

    // Clear the ROM adafruit splashscreen from the buffer
    display.clearDisplay();
    // By default the display is vertical, we want horizontal
    display.setRotation(1);

    // Set Buttons to "UP" (inactive) so they are activated (grounded "DOWN") when pressed
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);

    // text display tests
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,0);

    // welcome screen
    display.setTextSize(2);
    display.setCursor(30,20);
    display.println("CO2lfy");
    display.setTextSize(1);
    display.setCursor(33,display.getCursorY());
    display.print("air quality");
    display.display();
    
    Serial.println("Waiting for first measurement... (10 sec)");
    delay(10000);
}


// runtime loop
void loop() {

    // Reset OLED Display:
    // Clear screen
    display.clearDisplay();
    // Set cursor back at top left corner
    display.setCursor(0,0);
    // neat trick to better position things in the display
    int savedCursorX = 0;
    int savedCursorY = 0;
    // Reset text size in case it was left altered
    display.setTextSize(1);


    // is it time for the next measurement sample?
    if ( millis() - previousMeasurement >= measurementInterval ) {

        uint16_t error;
        char errorMessage[256];
        
        uint16_t previousCo2 = co2;
        float previousTemperature = temperature;
        float previousHumidity = humidity;
        
        // CO2 sensor:
        // Read Measurement
        error = scd4x.readMeasurement(co2, temperature, humidity);
        if (error) {
            Serial.print("Error trying to execute readMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        } else if (co2 == 0) {
            Serial.println("Invalid sample detected, skipping.");
        } else {
            Serial.print("Co2:");
            Serial.print(co2);
            Serial.print("\t");
            Serial.print("Temperature:");
            Serial.print(temperature);
            Serial.print("\t");
            Serial.print("Humidity:");
            Serial.println(humidity);
        }

        // Prepare co2 value increase formatting
        co2Increase = co2 - previousCo2;

        // store new measurement timestamp reference
        previousMeasurement = millis();

        if (millis() - previousArrayMeasurement >= 30000) {
            if (measurementArrayIndex < 110){
                measurementArray[measurementArrayIndex] = co2;
                measurementArrayIndex++;   
            } else {
              // if we have filled the array, on new value:
              // shift all values down by one index, then add the new value at the end
              for (int i=1; i<110; i++) {
                measurementArray[i-1] = measurementArray[i];
              }
              measurementArray[109] = co2;
            }
            
            previousArrayMeasurement = millis();
              
        }
    }


    // Handle display drawing and UI


    // Display mode 1 is the default main screen
    // It displays numeric values and a text rectangle
    if (displayMode == 1) {
        // "Co2: <value>ppm (<difference from previous>)"
        display.print("Co2: ");
        display.setTextSize(2);
        display.print(co2);
        display.setTextSize(1);
        savedCursorX= display.getCursorX();
        savedCursorY= display.getCursorY();
        display.print(" (");
        if (co2Increase >= 0) display.print("+");
        display.print(co2Increase);
        display.print(")");
        display.setCursor(savedCursorX, savedCursorY+7);
        display.println("ppm");    
    
        //1 small line vertical separation
        display.println("");
            
        // Health assessment display
        display.setTextSize(2);
        // Draw health assessment rectangle
        // start drawing 4px higher than cursor position
        // 42px high
        display.drawRect(0 ,display.getCursorY()-4, 128, 42, 1);
    
        // <470 ~= open air (accounting for mis-calibration, pollution, etc)
        if (co2<470) {
          // better center single-line message
          display.setCursor( display.getCursorX()+15, display.getCursorY()+8 );
          display.println("Open Air");
        }
        
        else if (co2<600) {
          // The word "acceptable" is long, so manually tweak cursor position
          display.setCursor( display.getCursorX()+4, display.getCursorY() );
          display.println("Acceptable");
          display.println("    air    ");
        }
        
        else if (co2 < 800) {
          display.println(" Wear Mask");
          display.setTextSize(1);
          display.println("");
          display.println(" (ffp2/n95 mask ONLY)");
        }
        else if (co2 < 1000) {
          display.println(" High Risk");
          display.setTextSize(1);
          display.println("");
          display.println("    Ventilate now!");
        }
        else if (co2 >= 1000) {
    
          Serial.print("current mili: ");
          Serial.println(millis());
    
          Serial.print("previous health assessment displayed: ");
          Serial.println(previousHealthAssessmentDisplayed);
          
          // reset display timestamp reference: start over
          if ( (millis() - previousHealthAssessmentDisplayed) >= 6000 ) previousHealthAssessmentDisplayed = millis();
    
          // if we are in the first 3s, blink health assessment message
          if ((millis() - previousHealthAssessmentDisplayed) <= 3000) {
            // 0s -> 1s : display message
            // 1s -> 1,5s :  blank rectangle (blink effect)
            //
            // 1,5s -> 2,5s : display message
            // 2,5s -> 3s: blank rectangle (blink effect)
    
            // Between 0 and 1s or 1,5s and 2,5s : display message
            // If we are outside this interval, nothing will be printed in the rectangle and it will be left blank (creating a blinking effet)
            if ((0 <= (millis() - previousHealthAssessmentDisplayed) ) && ( (millis() - previousHealthAssessmentDisplayed) < 1000) ||  (1500 <= (millis() - previousHealthAssessmentDisplayed)) && ((millis() - previousHealthAssessmentDisplayed) < 2500)) {
              
                // ensure big font as we change it for the second message
                display.setTextSize(2);
                display.println("   COVID");
                display.println("  KILLBOX");
            }       
          } else {
          // if we are in the last 3s, display warning instructions
          // reset cursor at the top left of rectangle
    
          // display emergency instructions
          // LINE1
          display.setTextSize(2);
          display.print("   OPEN");
          display.setTextSize(1);
          // align smaller text properly alongside bigger text
          display.setCursor(display.getCursorX(), display.getCursorY()+7);       
          display.println(" and");
    
          // LINE2
          display.setTextSize(2);
          display.print(" LEAVE");
          display.setTextSize(1);
          // align smaller text properly alongside bigger text
          display.setCursor(display.getCursorX(), display.getCursorY()+7);
          display.print(" 5+ min");        
          }
        }

        // ignore button presses if there was a press less than 0,5s ago
        // this is to avoid registering multiple presses from a single slowish human button press
        if ( (millis() - previousButtonPress) >= 500){
            // if user presses A from main screen , switch to display mode 1-1
            if(!digitalRead(BUTTON_A)) {
              displayMode = 11;
              previousButtonPress = millis();
            }
            
            // if user presses B, switch to display mode 2
             else if(!digitalRead(BUTTON_B)) {
              displayMode = 2;
              previousButtonPress = millis();
             }
    
            // if user presses C, switch to display mode 3
            else if(!digitalRead(BUTTON_C)) {
              displayMode = 3;
              previousButtonPress = millis();
            }
        }
        

    }

    // Display mode 1-1 (integer 11) is a graph representation of the last datapoints
    else if (displayMode == 11) {
      display.setTextSize(1);

      // horizontal axis
      display.drawLine(16,51, 126,51, 1);

      // min X marker (0min)
      display.drawLine(16,51, 16,53, 1);
      display.setCursor(13, 56);
      display.print("0");

      // X mark (5min)
      display.drawLine(26,51, 26,53, 1);

      // X mark (10min)
      display.drawLine(36,51, 36,53, 1);
      display.setCursor(30, 56);
      display.print ("10");

      // X mark (15min)
      display.drawLine(46,51, 46,53, 1);

      // X mark (20min)
      display.drawLine(56,51, 56,53, 1);
      display.setCursor(50, 56);
      display.print("20");

      // X mark (25min)
      display.drawLine(66,51, 66,53, 1);

      // X mark (30min)
      display.drawLine(76,51, 76,53, 1);
      display.setCursor(70, 56);
      display.print("30");

      // X mark (35min)
      display.drawLine(86,51, 86,53, 1);


      // X mark (40min)
      display.drawLine(96,51, 96,53, 1);
      display.setCursor(90, 56);
      display.print("40");

      // X mark (45min)
      display.drawLine(106,51, 106,53, 1);

      // X mark (50min)
      display.drawLine(116,51, 116,53, 1);
      display.setCursor(110, 56);
      display.print("50");

      // max X marker (55min)
      display.drawLine(126,51, 126,53, 1);
      
      
      // vertical axis
      display.drawLine(16,51, 16,1, 1);



      // min Y mark (400ppm)
      display.drawLine(16,51, 12,51, 1);
      display.setCursor(0,51);
      display.print("4");      

      // Y mark (500ppm)
      //display.drawLine(16,46, 14,46, 1);

      // Y mark (600ppm)
      display.drawLine(16,41, 12,41, 1);
      display.setCursor(0, 41);
      display.print("6");

      // Y mark (700ppm)
      //display.drawLine(16,36, 14,36, 1);

      // Y mark (800ppm)
      display.drawLine(16,31, 12,31, 1);
      display.setCursor(0, 31);
      display.print(8);

      // Y mark (900ppm)
      //display.drawLine(16,26, 14,26, 1);

      // Y mark (1000ppm)
      display.drawLine(16,21, 12,21, 1);
      display.setCursor(0,21);
      display.print(10);

      // Y mark (1100ppm)
      //display.drawLine(16,16, 14,16, 1);

      // Y mark (1200ppm)
      display.drawLine(16,11, 12,11, 1);
      display.setCursor(0,11);
      display.print(12);

      // Y mark (1300ppm)
      //display.drawLine(16,6,  14,6, 1);
           
      // max Y graduation (1400ppm)
      display.setCursor(0,1);
      display.println("14");

      // max Y mark
      display.drawLine(16,1, 12,1, 1);
   
      //actually plot points
      for (int i=0; i<110; i++) {

        // don't even try for values under 400 or over 1400ppm
        if (measurementArray[i] > 400 || measurementArray[i] < 1400){
            
            // first measurement is one pixel away from the X=0, we add i to it
            // Y value complicated:
            // our minimum plotted value is 400, so everything needs to be shifted down vertically by 400
            // then, we have only 5 pixels for every 100ppm increment, so we need to divide by 20
            // finally, Y drawing on the screen is top to bottom, so we substract from our "min Y value" (51) to plot higher points
            // then I just make sure we get a proper rounded up integer value to draw an actual pixel.
            display.drawPixel(17+i, (int) ceil(51-((measurementArray[i]-400)/20)), 1);
        }
      }

      // ignore button presses if there was a press less than 0,5s ago
      // this is to avoid registering multiple presses from a single slowish human button press
      if ( (millis() - previousButtonPress) >= 500){

          // From graph screen, pressing A again toggles main screen
          if(!digitalRead(BUTTON_A)){
            displayMode = 1;
            previousButtonPress = millis();
          }
          
          // Graph screen still behaves like default screen, button B and C do the same thing
          // if user presses B, switch to display mode 2
          else if(!digitalRead(BUTTON_B)){
            displayMode = 2;
            previousButtonPress = millis();
          }
    
          // if user presses C, switch to display mode 3
          else if(!digitalRead(BUTTON_C)) {
            displayMode = 3;
            previousButtonPress = millis();
          }
      }
   
    }

    // user pressed "B", draw display mode 2
    // This enables calibration mode
    else if (displayMode == 2) {
      // explain calibration to the user
      display.println("Sensor CALIBRATION:");
      display.println("once a week");
      display.println("");
      display.println("Put the device in");
      display.println("OPEN AIR");
      display.println("");
      display.println("press A to start");
      display.print("press C to cancel");

      // ignore button presses if there was a press less than 0,5s ago
      // this is to avoid registering multiple presses from a single slowish human button press
      if ( (millis() - previousButtonPress) >= 500){
          // Pressing A initiates calibration mode (display 2-1 or "21")
          if(!digitalRead(BUTTON_A)){
            // start calibration timer
            calibrationStartTime = millis();
            displayMode = 21;
            previousButtonPress = millis();
          }
        
          // From calibration screen, "C" gets back to main screen
          else if(!digitalRead(BUTTON_C)){
            displayMode = 1;
            previousButtonPress = millis();
          }
      }
      
    }

    else if (displayMode == 21) {
      display.println("CALIBRATING...");
      display.println("");
      display.println("Leave in open air for");
      display.print( (int) ceil( ((calibrationDuration - (millis() - calibrationStartTime))/1000)/60) +1);
      display.println(" minutes");
      display.println("");
      display.println("");
      display.println("");
      display.print("press C to cancel");

      // Once time is up, trigger calibration then show calibration complete display

      // calibration duration finished: re-start measurements and go to next screen
      if ( (millis() - calibrationStartTime) >= calibrationDuration) {
          
          uint16_t error;
          char errorMessage[256];
          
          // stop  previously started measurement
          error = scd4x.stopPeriodicMeasurement();
          if (error) {
              Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
              errorToString(error, errorMessage, 256);
              Serial.println(errorMessage);
          } else {
            Serial.println("Stopped measurement before forced calibration");
          }

          // initiate forced calibration
          // Reference atmospheric co2 concentration for calibration: 430
          // atmCo2Correction receives the value of the correction performed
          error = scd4x.performForcedRecalibration((uint16_t) 430, atmCo2Correction);
          if (error) {
              Serial.print("Error trying to execute performForcedRecalibration(): ");
              errorToString(error, errorMessage, 256);
              Serial.println(errorMessage);
          } else {
            Serial.println("Forced recalibration to atmospheric co2 value of 430ppm.");
            Serial.print("Performed correction of: ");
            // pff the Sensirion lib returns an unsigned value for the correction (signed) value...
            // their example does this substraction: https://github.com/Sensirion/arduino-snippets/blob/main/SCD4x_I2C_FRC_Forced_Recalibration_Example/SCD4x_I2C_FRC_Forced_Recalibration_Example.ino#L147
            Serial.print(atmCo2Correction-32768);
            Serial.println("ppm");
            
          }        
                    
          // Start Measurement again
          error = scd4x.startPeriodicMeasurement();
          if (error) {
              Serial.print("Error trying to execute startPeriodicMeasurement(): ");
              errorToString(error, errorMessage, 256);
              Serial.println(errorMessage);
          } else {
            Serial.println("Restarted periodic measurements following calibration");
          }

        displayMode = 22;
      }
      
      // ignore button presses if there was a press less than 0,5s ago
      // this is to avoid registering multiple presses from a single slowish human button press
      if ( (millis() - previousButtonPress) >= 500){
        
          // "C" cancels calibration and goes back to main screen
          if(!digitalRead(BUTTON_C)){
            displayMode = 1;
            previousButtonPress = millis();
          }
      }      
    }

    else if (displayMode == 22){
      display.setTextSize(2);
      display.println("CALIBRATED");
      display.setTextSize(1);
      display.println("");
      display.println("");
      display.println("Correction performed:");
      // pff the Sensirion lib returns an unsigned value for the correction (signed) value...
      // their example does this substraction: https://github.com/Sensirion/arduino-snippets/blob/main/SCD4x_I2C_FRC_Forced_Recalibration_Example/SCD4x_I2C_FRC_Forced_Recalibration_Example.ino#L147     
      display.print(atmCo2Correction-32768);
      display.println("ppm");
      display.println("");
      display.print("press A to go back");

      // ignore button presses if there was a press less than 0,5s ago
      // this is to avoid registering multiple presses from a single slowish human button press
      if ( (millis() - previousButtonPress) >= 500){
        
          // "A" goes back to main screen
          if(!digitalRead(BUTTON_A)){
            displayMode = 1;
            previousButtonPress = millis();
          }
      }      
    }

    // user pressed "C", draw third display mode
    // third display mode = credits screen!
    else if (displayMode == 3) {
      display.println("Co2lfy project");
      display.println("created by @koolfy");
      display.println("version: 0.1-beta");
      display.println("");
      display.println("Made in a rush :)");
      display.println("Code on github!");
      display.println("Licensed under GPLv3");
      
      

      // ignore button presses if there was a press less than 0,5s ago
      // this is to avoid registering multiple presses from a single slowish human button press
      if ( (millis() - previousButtonPress) >= 500){
          // For now any button press goes back to main screen
          if(!digitalRead(BUTTON_A)){
            displayMode = 1;
            previousButtonPress = millis();
          }
          else if(!digitalRead(BUTTON_B)){
            displayMode = 1;        
            previousButtonPress = millis();      
          }
          else if(!digitalRead(BUTTON_C)){
            displayMode = 1;        
            previousButtonPress = millis();      
          }
      }
    }
    
    // Print buffer to screen (only now does all this gets sent to the display)
    display.display();
    
}
