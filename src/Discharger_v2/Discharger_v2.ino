#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define supplyVoltage 5.0

#define battery01VoltagePin A0
#define battery02VoltagePin A1

#define battery01DischargerAddress 0x60
#define battery02DischargerAddress 0x61

#define HALF_AMP 408
#define ONE_AMP 818

#define MCP4726_CMD_WRITEDAC 0x40

#define buttonPin 12

double battery01Capacity = 0;
double battery02Capacity = 0;

enum STATE {
  IDLE,
  DISCHARGE_500,
  DISCHARGE_1000
} ;

enum STATE battery01State = IDLE;
enum STATE battery02State = IDLE;

Adafruit_PCD8544 display = Adafruit_PCD8544(11, 10, 9, 8, 7);

void setVoltage(uint8_t dischargerAddress, int output) {
  Wire.beginTransmission(dischargerAddress);
  Wire.write(MCP4726_CMD_WRITEDAC);
  Wire.write(output >> 4);
  Wire.write((output & 15) << 4);
  Wire.endTransmission();   
}

double readVoltage(uint8_t voltagePin) {
  return (supplyVoltage * analogRead(voltagePin)) / 1024;
}

void logMilliampHoursForTheLastSecond(double *capacity, enum STATE *batteryState, uint8_t voltagePin) {
  double current = 0;
  double voltage = readVoltage(voltagePin);
  if(*batteryState == DISCHARGE_500) current = 0.5;
  if(*batteryState == DISCHARGE_1000) current = 1;
  *capacity += current * 0.000277777777778;
}

void protectFromOverDischarge(uint8_t dischargerAddress, enum STATE *batteryState, uint8_t voltagePin) {
  double voltage = readVoltage(voltagePin);
  if(voltage <= 3.0) {
    setDischarge(dischargerAddress, batteryState, IDLE);
  }
}

void setDischarge(uint8_t dischargerAddress, enum STATE *batteryState, enum STATE newState) {
  *batteryState = newState;
  switch(newState) {
    case IDLE: 
      setVoltage(dischargerAddress, 0);
      break;
   case DISCHARGE_500:
      setVoltage(dischargerAddress, HALF_AMP);
      break;
   case DISCHARGE_1000:
      setVoltage(dischargerAddress, ONE_AMP);
      break;   
   default: 
      setVoltage(dischargerAddress, 0);
      break;
  }
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

void updateDisplayLine(uint8_t battery, enum STATE *batteryState, uint8_t voltagePin, double *capacity) {
  double voltage = readVoltage(voltagePin);
  display.print(battery);
  display.print(*batteryState != IDLE ? "D" : "-");
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
  updateDisplayLine(1, &battery01State, battery01VoltagePin, &battery01Capacity);
  updateDisplayLine(2, &battery02State, battery02VoltagePin, &battery02Capacity);
  display.display();
}

void setup() {
  Wire.begin();
  setDischarge(battery01DischargerAddress, &battery01State, IDLE);
  setDischarge(battery02DischargerAddress, &battery02State, IDLE);
  setupDisplay();
  pinMode(buttonPin, INPUT_PULLUP); 
  delay(2000);
}

void loop() {

  logMilliampHoursForTheLastSecond(&battery01Capacity, &battery01State, battery01VoltagePin);
  logMilliampHoursForTheLastSecond(&battery02Capacity, &battery02State, battery02VoltagePin);

  protectFromOverDischarge(battery01DischargerAddress, &battery01State, battery01VoltagePin);
  protectFromOverDischarge(battery02DischargerAddress, &battery02State, battery02VoltagePin);

  updateDisplay();

  uint8_t buttonStatus = digitalRead(buttonPin);
  if(buttonStatus == 0) {
    setDischarge(battery01DischargerAddress, &battery01State, DISCHARGE_500);
    setDischarge(battery02DischargerAddress, &battery02State, DISCHARGE_500); 
  } 
  delay(1000);
}
