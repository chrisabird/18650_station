#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define supplyVoltage 5.0
#define battery01DischagePin 3 
#define battery02DischagePin 4
#define battery03DischagePin 5
#define battery04DischagePin 6

#define battery01CurrentPin A0
#define battery02CurrentPin A1
#define battery03CurrentPin A2
#define battery04CurrentPin A3

#define battery01VoltagePin A4
#define battery02VoltagePin A5
#define battery03VoltagePin A6
#define battery04VoltagePin A7

#define buttonPin 12

double battery01Capacity = 0;
double battery02Capacity = 0;
double battery03Capacity = 0;
double battery04Capacity = 0;

Adafruit_PCD8544 display = Adafruit_PCD8544(11, 10, 9, 8, 7);

void logMilliampHoursForTheLastSecond(double *capacity, uint8_t dischargePin, uint8_t currentPin, uint8_t voltagePin) {
  uint8_t isDischarging  = digitalRead(dischargePin);
  double current = (supplyVoltage * analogRead(currentPin)) / 1024;
  double voltage = (supplyVoltage * analogRead(voltagePin)) / 1024;

  if(isDischarging) {
    *capacity += current * 0.000277777777778;
  }
}

void protectFromOverDischarge(uint8_t dischargePin, uint8_t voltagePin) {
  double voltage = (supplyVoltage * analogRead(voltagePin)) / 1024;
  if(voltage <= 3.0) {
    digitalWrite(dischargePin, LOW); 
  }
}

void setupDischargePin(uint8_t dischargePin) {
  pinMode(dischargePin, OUTPUT);
  digitalWrite(dischargePin, LOW); 
}

void setupDisplay() {
  display.begin();
  display.setContrast(50);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("18650 Tester");
  display.display();
}


void updateDisplayLine(uint8_t battery, uint8_t dischargePin, uint8_t voltagePin, double *capacity) {
  double voltage = (supplyVoltage * analogRead(voltagePin)) / 1024;
  display.print(battery);
  display.print(digitalRead(dischargePin) > 0 ? "D" : "-");
  display.print(" ");
  display.print((int)(*capacity * 1000));
  display.print(" ");
  display.println(voltage);
    
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("18650 Tester");
  updateDisplayLine(1, battery01DischagePin, battery01VoltagePin, &battery01Capacity);
  updateDisplayLine(2, battery02DischagePin, battery02VoltagePin, &battery02Capacity);
  updateDisplayLine(3, battery03DischagePin, battery03VoltagePin, &battery03Capacity);
  updateDisplayLine(4, battery04DischagePin, battery04VoltagePin, &battery04Capacity);
  display.display();
}

void setup() {
  setupDischargePin(battery01DischagePin);
  setupDischargePin(battery02DischagePin);
  setupDischargePin(battery03DischagePin);
  setupDischargePin(battery04DischagePin);  
  setupDisplay();
  //analogReference(EXTERNAL);
  pinMode(buttonPin, INPUT_PULLUP); 
  delay(2000);
}

void loop() {

  logMilliampHoursForTheLastSecond(&battery01Capacity, battery01DischagePin, battery01CurrentPin, battery01VoltagePin);
  logMilliampHoursForTheLastSecond(&battery02Capacity, battery02DischagePin, battery02CurrentPin, battery02VoltagePin);
  logMilliampHoursForTheLastSecond(&battery03Capacity, battery03DischagePin, battery03CurrentPin, battery03VoltagePin);
  logMilliampHoursForTheLastSecond(&battery04Capacity, battery04DischagePin, battery04CurrentPin, battery04VoltagePin);

  protectFromOverDischarge(battery01DischagePin, battery01VoltagePin);
  protectFromOverDischarge(battery02DischagePin, battery02VoltagePin);
  protectFromOverDischarge(battery03DischagePin, battery03VoltagePin);
  protectFromOverDischarge(battery04DischagePin, battery04VoltagePin);

  updateDisplay();

  uint8_t buttonStatus = digitalRead(buttonPin);
  if(buttonStatus == 0) {
    digitalWrite(battery01DischagePin, HIGH); 
    digitalWrite(battery02DischagePin, HIGH); 
    digitalWrite(battery03DischagePin, HIGH); 
    digitalWrite(battery04DischagePin, HIGH);   
  } 
  delay(1000);
}
