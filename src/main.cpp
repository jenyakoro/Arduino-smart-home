#include <Arduino.h>
//call the relevant library file
#include <Servo.h>
//#include <Wire.h>
#include <LCD_1602_RUS.h>  // подключаем библиотеку LCD_1602_RUS
#include <Adafruit_NeoPixel.h>

#define STRIP_PIN 3
//Set the communication address of I2C to 0x27, display 16 characters every line, two lines in total
LCD_1602_RUS mylcd(0x27, 16, 2);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, STRIP_PIN, NEO_GRB + NEO_KHZ800);


//set ports of two servos to digital 9 && 10c.lf $$
Servo servo_10;

void printWelcome();
void checkMotionLight();
void checkRain();
void switchFan();
void trafficLight();
void checkSerialInput();
void switchNightLight();
void printDebugMessage(String message);
void tryClearScreen();
void blinkLeds();
void blinkOff();
void tryPrintWelcome();
void printAnalogSensor(int sensorPin, String message);
void colorWipe(uint32_t c);

volatile int infrar;
String led2;
volatile int light;
volatile unsigned long startBlinking;
volatile bool isLedsOn = false;
volatile bool isFanPushed = false;
volatile bool debugEnabled = false;
volatile int val;

volatile bool isNightLightPushed = false;
volatile bool isNightLightOn = false;
volatile bool isTrafficButtonPushed = false;
volatile bool isTrafficEnabled = false;
volatile bool isGreenNeedWork = false;
volatile bool isGreenOn = false;
volatile bool isYellowOn = false;
volatile bool isRedOn = false;
volatile unsigned long greenBlinkStart = 0;
volatile unsigned long yellowBlinkStart = 0;
volatile unsigned long redBlinkStart = 0;
volatile unsigned long nightLightStart = 0;
unsigned long nightLightTime = 600L * 60 * 120;
// unsigned long nightLightTime = 3000;

int windowServoPin = 2;
//int yellowLedPin = 3;
int redPin = 4;
int yellowPin = 5;
int greenPin = 6;
int fanPin = 7;
int fanPin2 = 8;
int relayPin = 9;

int photocellPin = 1;
int steamPin = 2;
int leftButton = 3;
int rightButton = 4;
int nightLightButton = 10;
int motionPin = 5;

volatile bool isRain = false;
volatile bool isGas = false;
String EMPTY = "                ";

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  mylcd.init();
  mylcd.backlight();
  printWelcome();

  servo_10.attach(windowServoPin);
  servo_10.write(180);

  pinMode(nightLightButton, INPUT);

  pinMode(relayPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(fanPin2, OUTPUT);
//  pinMode(yellowLedPin, OUTPUT);

  digitalWrite(fanPin, LOW);
  digitalWrite(fanPin2, LOW);
}

void loop() {
  checkMotionLight();
  checkRain();
  switchFan();
  trafficLight();
  checkSerialInput();
  switchNightLight();
}

void switchNightLight() {
  if (digitalRead(nightLightButton) == LOW && !isNightLightPushed) {
    isNightLightPushed = true;
    if (isNightLightOn) {
      printDebugMessage("Night light: OFF");
      digitalWrite(relayPin, LOW);
      isNightLightOn = false;
      mylcd.backlight();
    } else {
      digitalWrite(relayPin, HIGH);
      nightLightStart = millis();
      Serial.print("Night light ON in ");
      Serial.println(nightLightStart);
      isNightLightOn = true;
      mylcd.noBacklight();
    }
  }
  if (digitalRead(nightLightButton) != LOW && isNightLightPushed) {
    isNightLightPushed = false;
  }
  if (isNightLightOn && millis() - nightLightStart > nightLightTime and digitalRead(relayPin) != LOW) {
    Serial.print("Night light auto OFF in ");
    Serial.println(millis());
    // isNightLightOn = false;
    digitalWrite(relayPin, LOW);
  }

}

void checkMotionLight() {
  if (!isTrafficEnabled) {
    light = analogRead(photocellPin);
    if (light < 300) {
      infrar = analogRead(motionPin);
      if (infrar > 200) {
       colorWipe(strip.Color(0, 0, 255)); // Blue
      } else {
       colorWipe(0);
      }
    } else if (light >= 300) {
     colorWipe(0);
    }
  }
}

void colorWipe(uint32_t c) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
  }
}

void checkRain() {
  if (analogRead(steamPin) > 800 && !isRain) {
    tryClearScreen();
    mylcd.setCursor(5, 0);
    mylcd.print("ДОЖДЬ!");
    if (!isGas) {
      servo_10.write(0);
    }
    isRain = true;
  } else if (analogRead(steamPin) < 100 && isRain) {
    isRain = false;
    if (!isGas) {
      servo_10.write(180);
    }
    tryPrintWelcome();
  }
}

void blinkLeds() {
  int valLeds = (((millis() - startBlinking) / 500) % 2);
  if (valLeds == 0 && !isLedsOn) {
    digitalWrite(greenPin, HIGH);
    digitalWrite(yellowPin, HIGH);
    digitalWrite(redPin, HIGH);
//    digitalWrite(yellowLedPin, HIGH);
    mylcd.backlight();
    isLedsOn = true;
  } else if (valLeds == 1 && isLedsOn) {
    digitalWrite(greenPin, LOW);
    digitalWrite(yellowPin, LOW);
    digitalWrite(redPin, LOW);
//    digitalWrite(yellowLedPin, LOW);
    mylcd.noBacklight();
    isLedsOn = false;
  }
}

void blinkOff() {
  digitalWrite(greenPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(redPin, LOW);
//  digitalWrite(yellowLedPin, LOW);
  mylcd.backlight();
  isLedsOn = false;
}

void switchFan() {
  if (analogRead(rightButton) < 100 && !isFanPushed) {
    isFanPushed = true;
    if (digitalRead(fanPin2) == 1) {
      digitalWrite(fanPin2, LOW);
    } else {
      digitalWrite(fanPin2, HIGH);
    }
  }
  if (analogRead(rightButton) > 100 && isFanPushed) {
    isFanPushed = false;
  }
}

void blinkGreen() {
  int valG = ((millis() - greenBlinkStart) / 3000) % 2;
  if (valG == 0 && isGreenOn && digitalRead(greenPin) == LOW) {
    digitalWrite(greenPin, HIGH);
    colorWipe(strip.Color(0, 100, 0));
  } else if (valG > 0 && isGreenOn && digitalRead(greenPin) == HIGH) {
    digitalWrite(greenPin, LOW);
    yellowBlinkStart = millis();
    isGreenOn = false;
    isYellowOn = true;
  }
}

void blinkYellow() {
  int valY = ((millis() - yellowBlinkStart) / 3000) % 2;
  if (valY == 0 && isYellowOn && digitalRead(yellowPin) == LOW) {
    digitalWrite(yellowPin, HIGH);
    colorWipe(strip.Color(100, 25, 0));
  } else if (valY > 0 && isYellowOn && digitalRead(yellowPin) == HIGH) {
    digitalWrite(yellowPin, LOW);
    redBlinkStart = millis();
    isYellowOn = false;
    isRedOn = true;
  }
}

void blinkRed() {
  int valR = ((millis() - redBlinkStart) / 3000) % 2;
  if (valR == 0 && isRedOn && digitalRead(redPin) == LOW) {
    digitalWrite(redPin, HIGH);
    colorWipe(strip.Color(100, 0, 0));
  } else if (valR > 0 && isRedOn && digitalRead(redPin) == HIGH) {
    digitalWrite(redPin, LOW);
    greenBlinkStart = millis();
    isRedOn = false;
    isGreenOn = true;
  }
}

void trafficLight() {
  if (analogRead(leftButton) < 100 && !isTrafficButtonPushed) {
    isTrafficButtonPushed = true;
    if (!isTrafficEnabled) {
      printDebugMessage("Traffic light: ON");
      isTrafficEnabled = true;
      greenBlinkStart = millis();
      isGreenOn = true;
      isYellowOn = false;
      isRedOn = false;
    } else {
      printDebugMessage("Traffic light: OFF");
      digitalWrite(greenPin, LOW);
      digitalWrite(yellowPin, LOW);
      digitalWrite(redPin, LOW);
      colorWipe(0);
      isGreenOn = false;
      isYellowOn = false;
      isRedOn = false;
      isTrafficEnabled = false;
    }
  }

  if (isTrafficEnabled) {
    blinkGreen();
    blinkYellow();
    blinkRed();
  }

  if (analogRead(leftButton) > 100 && isTrafficButtonPushed) {
    isTrafficButtonPushed = false;
  }
}

void printWelcome() {
  mylcd.clear();
  mylcd.setCursor(0, 0);            // ставим курсор на 3 символ первой строки
  mylcd.print("ДОБРО ПОЖАЛОВАТЬ");  // печатаем символ на первой строке
  mylcd.setCursor(1, 1);            // ставим курсор на 3 символ первой строки
  mylcd.print("В КОСТИН ДОМИК");    // печатаем символ на первой строке
}

void printSensors() {
  Serial.println("Analog sensors:");
  printAnalogSensor(photocellPin, "  Photocell sensor");
  printAnalogSensor(steamPin, "  Humidity sensor");
  printAnalogSensor(leftButton, "  Left button");
  printAnalogSensor(rightButton, "  Right button");
  printAnalogSensor(motionPin, "  Motion sensor");
}

void tryClearScreen() {
  if (!isRain) {
    mylcd.clear();
  }
}

void tryPrintWelcome() {
  if (!isRain) {
    printWelcome();
  }
}

void printAnalogSensor(int sensorPin, String message) {
  Serial.print(message);
  Serial.print(" (");
  Serial.print(sensorPin);
  Serial.print("): ");
  Serial.println(analogRead(sensorPin));
}

void printDebugMessage(String message) {
  if(debugEnabled) {
    Serial.println(message);
  }
}

void checkSerialInput() {
  if (Serial.available() > 0) {
    val = Serial.read();  //set val to character read by serial
  }
  switch (val) {
    case 'd':                  //if val is character 'c'，program will circulate
      if(debugEnabled) {
        Serial.println("debug messages OFF");
        debugEnabled = false;
      } else {
        Serial.println("debug messages ON");
        debugEnabled = true;
      }
      break;                             //exit loop
    case 'z':  //if val is character 'k'，program will circulate
      printSensors();
      break;  //exit loop
  }
}