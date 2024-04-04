#include <Arduino.h>
#include <Servo.h>
#include <LCD_1602_RUS.h>
#include <Adafruit_NeoPixel.h>

//Set the communication address of I2C to 0x27, display 16 characters every line, two lines in total
LCD_1602_RUS mylcd(0x27, 16, 2);

//set ports of two servos to digital 9 && 10c.lf $$
Servo barrierServo;

void printWelcome();
void checkMotionLight();
void switchFan();
void trafficLight();
void switchBarrier();
void checkSerialInput();
void switchNightLight();
void printDebugMessage(String message);
void printAnalogSensor(int sensorPin, String message);
void colorWipe(uint32_t c);
void checkGarageLight();

int infrar;
int light;
bool isFanPushed = false;
bool debugEnabled = false;
int val;

bool isNightLightPushed = false;
bool isBarrierPushed = false;
bool isNightLightOn = false;
bool isTrafficButtonPushed = false;
bool isTrafficEnabled = false;
bool isGreenNeedWork = false;
bool isGreenOn = false;
bool isYellowOn = false;
bool isRedOn = false;
bool isGarageLightEnabled = false;
bool isMotionLightEnabled = false;
unsigned long garageLightStart = 0;
unsigned long greenBlinkStart = 0;
unsigned long yellowBlinkStart = 0;
unsigned long redBlinkStart = 0;
unsigned long nightLightStart = 0;
unsigned long nightLightTime = 600L * 60 * 120; // 600L * 60 * 120 is about 2 hours

//digital pins
const int RED_PIN = 4;
const int YELLOW_PIN = 5;
const int GREEN_PIN = 6;
const int BARRIES_SERVO_PIN = 7;
const int STRIP_PIN = 8;
const int RELAY_PIN = 9;
const int GARAGE_LIGHT_PIN = 10;
const int GARAGE_ROOM_LIGHT_PIN = 11;
const int ECHO_PIN = 12;
const int TRIG_PIN = 13;
const int FAN_PIN_2 = 14; // use analog pin as digital A0
const int FAN_PIN = 15; // use analog pin as digital A1

//analog pins
const int NIGHT_LIGHT_BUTTON_PIN = 2;
const int TRAFFIC_BUTTON_PIN = 3;
const int FAN_BUTTON_PIN = 4;
const int MOTION_PIN = 5;
const int BARRIER_BUTTON_PIN = 6;  // keyestudio super additional analog pin A6
const int PHOTOCELL_PIN = 7; // keyestudio super additional analog pin A7

String EMPTY = "                ";
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, STRIP_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  mylcd.init();
  mylcd.backlight();
  printWelcome();

  barrierServo.attach(BARRIES_SERVO_PIN);
  
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(FAN_PIN_2, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(FAN_PIN, LOW);
  digitalWrite(FAN_PIN_2, LOW);
}

void loop() {
  checkMotionLight();
  switchFan();
  trafficLight();
  checkSerialInput();
  switchNightLight();
  switchBarrier();
  checkGarageLight();
}

void checkGarageLight() {
  int duration;
  int distance;
  digitalWrite(TRIG_PIN , HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN , LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration/2) / 28.5 ;
  if (distance < 10 && !isGarageLightEnabled) {
    isGarageLightEnabled = true;
    garageLightStart = millis();
    digitalWrite(GARAGE_LIGHT_PIN, HIGH);
    digitalWrite(GARAGE_ROOM_LIGHT_PIN, HIGH);
  } else if (distance > 25 && isGarageLightEnabled && (millis() - garageLightStart > 2000 || millis() < garageLightStart)) {
    garageLightStart = 0;
    isGarageLightEnabled = false;
    digitalWrite(GARAGE_LIGHT_PIN, LOW);
    digitalWrite(GARAGE_ROOM_LIGHT_PIN, LOW);   
  } else if (isGarageLightEnabled and distance < 10) {
    garageLightStart = millis();
  }
}

void switchNightLight() {
  if (analogRead(NIGHT_LIGHT_BUTTON_PIN) < 100 && !isNightLightPushed) {
    isNightLightPushed = true;
    if (isNightLightOn) {
      printDebugMessage("Night light: OFF");
      digitalWrite(RELAY_PIN, LOW);
      isNightLightOn = false;
      mylcd.backlight();
    } else {
      printDebugMessage("Night light: ON");
      digitalWrite(RELAY_PIN, HIGH);
      nightLightStart = millis();
      isNightLightOn = true;
      mylcd.noBacklight();
    }
  }
  if (analogRead(NIGHT_LIGHT_BUTTON_PIN) > 100 && isNightLightPushed) {
    isNightLightPushed = false;
  }
  if (isNightLightOn && digitalRead(RELAY_PIN) != LOW && (millis() - nightLightStart > nightLightTime || millis() < nightLightStart)) {
    // isNightLightOn = false;
    digitalWrite(RELAY_PIN, LOW);
  }

}

void switchBarrier() {
  if (analogRead(BARRIER_BUTTON_PIN) < 100 && !isBarrierPushed) {
    isBarrierPushed = true;
    int currentAngle = barrierServo.read();
    if (currentAngle > 50) {
      for (int i = currentAngle; i > 0; i--) {
        barrierServo.write(i);
        delay(8);
      }
    } else {
      for (int i = currentAngle; i < currentAngle + 100; i++) {
        barrierServo.write(i);
        delay(8);
      }
    }

  }
  if (analogRead(BARRIER_BUTTON_PIN) > 100 && isBarrierPushed) {
    isBarrierPushed = false;
  }
}

void checkMotionLight() {
  if (!isTrafficEnabled) {
    light = analogRead(PHOTOCELL_PIN);
    if (light < 300) {
      infrar = analogRead(MOTION_PIN);
      if (infrar > 200 && !isMotionLightEnabled) {
        colorWipe(strip.Color(0, 0, 255)); // Blue
        isMotionLightEnabled = true;
      } else if (infrar < 200 && isMotionLightEnabled) {
        colorWipe(0);
        isMotionLightEnabled = false;
      }
    } else if (light >= 300 && isMotionLightEnabled) {
      colorWipe(0);
      isMotionLightEnabled = false;
    }
  }
}

void colorWipe(uint32_t c) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void switchFan() {
  if (analogRead(FAN_BUTTON_PIN) < 100 && !isFanPushed) {
    isFanPushed = true;
    if (digitalRead(FAN_PIN_2) == 1) {
      digitalWrite(FAN_PIN_2, LOW);
    } else {
      digitalWrite(FAN_PIN_2, HIGH);
    }
  }
  if (analogRead(FAN_BUTTON_PIN) > 100 && isFanPushed) {
    isFanPushed = false;
  }
}

void blinkGreen() {
  int valG = ((millis() - greenBlinkStart) / 3000) % 2;
  if (valG == 0 && isGreenOn && digitalRead(GREEN_PIN) == LOW) {
    digitalWrite(GREEN_PIN, HIGH);
    colorWipe(strip.Color(0, 100, 0));
  } else if (valG > 0 && isGreenOn && digitalRead(GREEN_PIN) == HIGH) {
    digitalWrite(GREEN_PIN, LOW);
    yellowBlinkStart = millis();
    isGreenOn = false;
    isYellowOn = true;
  }
}

void blinkYellow() {
  int valY = ((millis() - yellowBlinkStart) / 3000) % 2;
  if (valY == 0 && isYellowOn && digitalRead(YELLOW_PIN) == LOW) {
    digitalWrite(YELLOW_PIN, HIGH);
    colorWipe(strip.Color(100, 25, 0));
  } else if (valY > 0 && isYellowOn && digitalRead(YELLOW_PIN) == HIGH) {
    digitalWrite(YELLOW_PIN, LOW);
    redBlinkStart = millis();
    isYellowOn = false;
    isRedOn = true;
  }
}

void blinkRed() {
  int valR = ((millis() - redBlinkStart) / 3000) % 2;
  if (valR == 0 && isRedOn && digitalRead(RED_PIN) == LOW) {
    digitalWrite(RED_PIN, HIGH);
    colorWipe(strip.Color(100, 0, 0));
  } else if (valR > 0 && isRedOn && digitalRead(RED_PIN) == HIGH) {
    digitalWrite(RED_PIN, LOW);
    greenBlinkStart = millis();
    isRedOn = false;
    isGreenOn = true;
  }
}

void trafficLight() {
  if (analogRead(TRAFFIC_BUTTON_PIN) < 100 && !isTrafficButtonPushed) {
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
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(YELLOW_PIN, LOW);
      digitalWrite(RED_PIN, LOW);
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

  if (analogRead(TRAFFIC_BUTTON_PIN) > 100 && isTrafficButtonPushed) {
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
  printAnalogSensor(BARRIER_BUTTON_PIN, "  Barrier button");
  printAnalogSensor(PHOTOCELL_PIN, "  Photocell sensor");
  printAnalogSensor(NIGHT_LIGHT_BUTTON_PIN, "  Night light button");
  printAnalogSensor(TRAFFIC_BUTTON_PIN, "  Left button");
  printAnalogSensor(FAN_BUTTON_PIN, "  Right button");
  printAnalogSensor(MOTION_PIN, "  Motion sensor");
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
    Serial.println(val);
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