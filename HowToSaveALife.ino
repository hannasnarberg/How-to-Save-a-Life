
// Information on performance of CPR was retreived from here:
// https://www.nhs.uk/conditions/first-aid/cpr/

/*-------------------SET UP----------------------*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
Servo servo;  // Create servo object to control a servo

// Pairing sensors to pins
const int lightPin = A1;
const int servoPin = 9;
const int compressionButtonPin = 2;
const int buzzerPin = 8;
const int soundPin = A0;
const int phoneButtonPin1 = 3;
const int phoneButtonPin2 = 4;

// Sets thresholds
const int brightnessThreshold = 500;
const int darknessThreshold = 100;
const int accThreshold = 0;

// Variables used when calling 112
int phoneButton2State = 0;
int phoneButton1State = 0;
String phonePresses;
boolean pressed112 = false;
int soundReading;
unsigned long currentTime = 0;
String phoneBuzzerState = "LOW";
unsigned long timeChangeOfBuzzerState = 0;

// Variables regarding compression performance
int compressionButtonState = 0;
int compressionCount = 0;
unsigned long timeButtonPress;
unsigned long timePreviousButtonPress = 0;
unsigned long timeSincePressed = 0;
int totalTimeCompressions = 0;
float compressionRate = 0;

// Variables regarding rescue breath performance
unsigned long blowStopTime = 0;
unsigned long blowStartTime = 0;
int blowCount = 0;
int blowTotalTime = 0;
boolean currentlyBlowing = false;
float lightReading;
float accY = 0;
float accReading;
#define offsetY 0.04  // For calibrating accelerometer readings
#define gainY 10.12   // For calibrating accelerometer readings
boolean untiltedChin = false;

// Variables for running average of light sensor measurments
const int lightNumReadings = 10;
int lightReadings[lightNumReadings];  // The readings from the analog input
int lightReadIndex = 0;               // The index of the current reading
float lightTotal = 0;                 // The running total

// Variables for running average of accelerometer measurments
const int accNumReadings = 10;
int accReadIndex = 0;               // The index of the current reading
float accReadings[accNumReadings];  // The readings from the analog input
float accTotal = 0;                 // The running total

// Variable regarding servo motor
int angle = 0;  // Sets the start angle of the servo to 0.

// Other variables
String feedbackMessage;  // For storing feedbackmessages to be displayed in processing
int score = 0;           // Variable for storing score
int state = 0;           // Starting the program in state 0

//Assign a unique ID to this sensor at the same time
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);


RF24 radio(10, 7);  // CE, CSN
const byte addresses[][6] = { "00001", "00002" };
char recievedValue[32] = "";


void setup() {
  Serial.begin(9600);
  servo.attach(servoPin);
  servo.write(angle);
  pinMode(lightPin, INPUT);
  pinMode(compressionButtonPin, INPUT);
  pinMode(phoneButtonPin1, INPUT);
  pinMode(phoneButtonPin2, INPUT);
  pinMode(buzzerPin, OUTPUT);

  if (!accel.begin()) {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while (1)
      ;
  }
  accel.setRange(ADXL345_RANGE_16_G);

  for (int acc_thisReading = 0; acc_thisReading < accNumReadings; acc_thisReading++) {  // Initialise all the x readings to 0
    accReadings[acc_thisReading] = 0;
  }

  for (int light_thisReading = 0; light_thisReading < lightNumReadings; light_thisReading++) {  // Initialise all the readings to 0
    lightReadings[light_thisReading] = 0;
  }

  radio.begin();
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(10);
}

/*------------------LOOP----------------------*/

void loop() {

  // Sending CPR-feedback to proccessing
  if (feedbackMessage != "") {
    feedbackBuzzer();  // Buzzer makes sound when feedback is displayed in processing
    Serial.println(feedbackMessage);
  }
  feedbackMessage = "";

  // Reading sensors
  accReading = readAcc();
  lightReading = readLightSensor();
  compressionButtonState = digitalRead(compressionButtonPin);
  soundReading = analogRead(soundPin);
  phoneButton1State = digitalRead(phoneButtonPin1);
  phoneButton2State = digitalRead(phoneButtonPin2);

  // Reading from radio
  radio.startListening();
  if (radio.available()) {
    while (radio.available()) {
      radio.read(&recievedValue, sizeof(recievedValue));
      Serial.println(recievedValue);
    }
    radio.stopListening();

    // Checking if score is high enough for doll to start breathing
    if (score >= 20) {
      state = 5;
      Serial.println("breathing");  // Tell processing that doll is breathing
    }

    // Checking if ambulance has arrived
    if (receivedValue == 2) {
      state = 5;                           // Ambulance staff makes the doll breath
      Serial.println("ambulanceArrived");  // Tell processing ambulance has arrived
    }

    switch (state) {
        /*
      Case 0: Stabilize lightsensor readings
      Case 1: Press 112
      Case 2: Talk to emergency staff
      Case 3: Compressions
      Case 4: Rescue breaths
      Case 5: Doll starts breathing
      */

      case 0:
        while (lightReading < brightnessThreshold) {
          lightReading = readLightSensor();
        }
        state = 1;
        break;

      case 1:
        if (lightReading < darknessThreshold || compressionButtonState == HIGH) {  // If user starts compressions or rescue breaths
          Serial.println(compressionButtonState);
          Serial.println(lightReading);
          Serial.println("call112Reminder");  // remind user to call 112 first
        } else {
          if (receivedValue == 1) {  // If emergency staff has recieved the call
            state = 2;               // move to case 1 to talk to emergency staff
          } else if (pressed112) {   // If user has pressed 112
            int transmitted = 2;
            radio.write(&transmitted, sizeof(transmitted));  // Transmit a 0 to the emergency staff, meaning we have called them
            outGoingCallBuzzer();                            // Sound simulating outgoing call

          } else {
            pressed112 = registerPhonePresses();  // Otherwise, register new phone presses
          }
        }
        break;

      case 2:
        connectedCallBuzzer();                                                     // Make a sound when the emergency staff answers the phone
        if (lightReading < darknessThreshold || compressionButtonState == HIGH) {  // If user starts compressions or rescue breaths
          Serial.println("call112Reminder");                                       // Remind user to complete 112 call first
        }
        if (soundReading > 40) {  // If talking is detected
          int transmitted = 1;    // Transmit a 1, indicating we have started speaking to emergency staff
          radio.write(&transmitted, sizeof(transmitted));
          state = 3;                        // Call for help completed, move on to compressions.
          Serial.println("clearReminder");  // Clear any "Call-112"-reminder in Processing
        }
        break;

      case 3:
        // If player moves on to rescue breaths
        if (lightReading < darknessThreshold) {
          state = 4;
          blowCount++;  // Register first blow
          currentlyBlowing = true;
          blowStartTime = millis();
          if (accReading > accThreshold) {
            untiltedChin = true;  // Register if chin was tilted while blowing, for feedback later
          }

          // Evaluating compression performance
          compressionRate = ((float)compressionCount / totalTimeCompressions) * 60000;  // Number of compressions per minute
          score = compressionFeedback(compressionRate, compressionCount, score);        // Evaluating compression performance

          // Setting compression variables to zero for next round of compressions
          compressionCount = 0;
          timePreviousButtonPress = 0;
          totalTimeCompressions = 0;
        }

        // Register compressions
        else {
          if (compressionButtonState == HIGH) {
            timeButtonPress = millis();
            if (timePreviousButtonPress != 0) {
              timeSincePressed = timeButtonPress - timePreviousButtonPress;
            }
            compressionCount++;
            totalTimeCompressions += (float)timeSincePressed;
            timePreviousButtonPress = timeButtonPress;
            delay(500);
          }
        }
        break;

      case 4:
        // If player moves on to compressions
        if (compressionButtonState == HIGH) {
          state = 3;
          compressionCount++;
          timePreviousButtonPress = millis();  // Register time of first compression

          // Evaluate rescue breath performance
          blowStopTime = millis();
          blowTotalTime = (int)(blowStopTime - blowStartTime);
          score = rescueBreathFeedback(blowTotalTime, blowCount, score);
          blowCount = 0;
        }

        // Register rescue breaths
        else {
          currentlyBlowing = registerBlow(currentlyBlowing);  // Register blow returns true if the user is currently blowing and false otherwise
        }
        break;

      case 5:
        moveServo();  // Simulating that the doll is breathing
        break;
    }
  }

  /*-------------------FUNCTIONS----------------------*/

  boolean registerPhonePresses() {
    /* Returns true if 112 has been succesfully pressed, otherwise false */

    if (phoneButton1State == HIGH) {
      feedbackBuzzer();
      phonePresses = phonePresses + "1";
    }
    if (phoneButton2State == HIGH) {
      feedbackBuzzer();
      phonePresses = phonePresses + "2";
    }

    // If the user has clicked the buttons in the wrong order, reset the storing of button presses
    if (phonePresses != "1" && phonePresses != "11" && phonePresses != "112") {
      phonePresses = "";
    }
    delay(500);
    if (phonePresses == "112") {
      return true;
    } else {
      return false;
    }
  }

  boolean registerBlow(boolean currentlyBlowing) {

    /* A new blow can only be registered if a paus has
 occured since the last blow, i.e. a moment where the light sensor
 gives a value above the brightness threshold. Returns true if
 the user is currently currentlyBlowing and false if not. */

    if (currentlyBlowing == true) {              // If user is currently blowing
      if (lightReading > brightnessThreshold) {  // look for a paus
        return false;
      }
      return true;
    }

    else {                                     // If user is not currently blowing
      if (lightReading < darknessThreshold) {  // look for a new blow
        blowCount++;
        if (accReading > accThreshold) {
          untiltedChin = true;  // Register if chin was tilted while blowing, for feedback later
        }
        return true;
      }
      return false;
    }
  }

  int rescueBreathFeedback(int blowTotalTime, int blowCount, int score) {
    if (blowTotalTime >= 5000 && blowTotalTime <= 8000) {  // Total time of the blows should not be more than 8 seconds nor less than 5 seconds
      score += 5;
    } else {
      if (5000 > blowTotalTime) {
        feedbackMessage += "b1,";  // b1 = Too fast rescue breaths
      } else {
        feedbackMessage += "b2,";  // b2 = Too slow rescue breaths
      }
    }
    if (blowCount == 2) {
      score += 5;
    } else {
      if (blowCount < 2) {
        feedbackMessage += "b3,";  // b3 = Too few rescue breaths.
      } else {
        feedbackMessage += "b4,";  // b4 = Too many rescue breaths.
      }
    }
    if (untiltedChin == true) {
      feedbackMessage += "b5,";  // b5 = Don't forget to tilt the chin.
      untiltedChin = false;
      score -= 5;
    }
    return score;
  }

  int compressionFeedback(float compressionRate, int compressionCount, int score) {
    if (compressionRate >= 100 && compressionRate <= 120) {  // The optimal compression rate is between 100 and 120 compressions per minute
      score += 5;
    } else {
      if (compressionRate < 100) {
        feedbackMessage += "a1,";  // a1 = Compression rate too low
      } else {
        feedbackMessage += "a2,";  // a2 = Compression rate too high
      }
    }
    if (compressionCount >= 28 && compressionCount <= 32) {
      score += 5;
    } else {
      if (compressionCount < 28) {
        feedbackMessage += "a3,";  // a3 = Too few compressions
      } else {
        feedbackMessage += "a4,";  // a4 = Too many compressions
      }
    }
    return score;
  }

  void feedbackBuzzer() {
    digitalWrite(buzzerPin, HIGH);  // Turn on the buzzer
    delay(200);
    digitalWrite(buzzerPin, LOW);  // Turn off the buzzer
  }

  void outGoingCallBuzzer() {
    currentTime = millis();
    if (phoneBuzzerState == "HIGH") {
      if ((int)(currentTime - timeChangeOfBuzzerState) > 1000) {
        digitalWrite(buzzerPin, LOW);
        timeChangeOfBuzzerState = currentTime;
        phoneBuzzerState = "LOW";
      }
    }

    else {
      if ((int)(currentTime - timeChangeOfBuzzerState) > 2000) {
        digitalWrite(buzzerPin, HIGH);
        timeChangeOfBuzzerState = currentTime;
        phoneBuzzerState = "HIGH";
      }
    }
  }

  void connectedCallBuzzer() {
    for (int i = 1; i <= 3; ++i) {
      digitalWrite(buzzerPin, HIGH);  // Turn on the buzzer
      delay(75);
      digitalWrite(buzzerPin, LOW);  // Turn off the buzzer
      delay(75);
    }
  }

  void moveServo() {
    // Change angle of servo motor
    if (angle == 0) {
      angle = 45;
    } else if (angle == 45) {
      angle = 0;
    }
    // Control servo motor according to the angle
    servo.write(angle);
    delay(500);
  }

  float readAcc() {
    sensors_event_t event;
    accel.getEvent(&event);

    accY = (event.acceleration.y - offsetY) / gainY;
    accTotal = accTotal - accReadings[accReadIndex];  // Subtract the last reading
    accReadings[accReadIndex] = accY;                 // Read from the sensor
    accTotal = accTotal + accReadings[accReadIndex];  // Add the reading to the total
    accReadIndex = accReadIndex + 1;                  // Advance to the next position in the array

    if (accReadIndex >= accNumReadings) {  // If we're at the end of the array
      accReadIndex = 0;                    // We start over from the beginning
    }
    return accTotal / accNumReadings;  // Return average
  }

  int readLightSensor() {
    lightTotal = lightTotal - lightReadings[lightReadIndex];  // Subtract the last reading
    lightReadings[lightReadIndex] = analogRead(lightPin);     // Read from the sensor
    lightTotal = lightTotal + lightReadings[lightReadIndex];  // Add the reading to the total
    lightReadIndex = lightReadIndex + 1;                      // Advance to the next position in the array

    if (lightReadIndex >= lightNumReadings) {  // If we're at the end of the array
      lightReadIndex = 0;                      // We start over from the beginning
    }
    return lightTotal / lightNumReadings;  // Return average
  }
