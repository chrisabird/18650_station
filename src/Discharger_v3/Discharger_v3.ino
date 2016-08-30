#include <SPI.h>
#include <Wire.h>
#include <PCD8544.h>

#define supplyVoltage 5.0

#define battery01VoltagePin A0
#define battery02VoltagePin A1

#define battery01ChargePin 2
#define battery02ChargePin 3

#define battery01ChargeStatusPin 4
#define battery02ChargeStatusPin 5

#define battery01DischargerAddress 0x60
#define battery02DischargerAddress 0x61

#define MCP4736_ONE_VOLT 818
#define MCP4736_ZERO_VOLT 0
#define MCP4726_CMD_WRITEDAC 0x40

#define buttonPin 12

double battery01Capacity = 0;
double battery02Capacity = 0;

enum STATE {
  IDLE,
  CHARGE,
  DISCHARGE
} ;

enum STATE battery01State = IDLE;
enum STATE battery02State = IDLE;

PCD8544 display = PCD8544(11, 10, 9, 7, 8);

void setDACVoltage(uint8_t dischargerAddress, int output) {
  Wire.beginTransmission(dischargerAddress);
  Wire.write(MCP4726_CMD_WRITEDAC);
  Wire.write(output >> 4);
  Wire.write((output & 15) << 4);
  Wire.endTransmission();   
}

void setChargeStatus(uint8_t batteryChargePin, uint8_t enabled) {
  digitalWrite(batteryChargePin, enabled);
}

double readVoltage(uint8_t voltagePin) {
  return (supplyVoltage * analogRead(voltagePin)) / 1024;
}

void logMilliampHoursForTheLastSecond(double *capacity, enum STATE *batteryState) {
  if(*batteryState == DISCHARGE) {
    *capacity += 0.000277777777778;
  }
}

void protectFromOverDischarge(uint8_t dischargerAddress, enum STATE *batteryState, uint8_t voltagePin, uint8_t batteryChargePin) {
  double voltage = readVoltage(voltagePin);
  if(voltage <= 3.0) {
    setState(dischargerAddress, batteryChargePin, batteryState, IDLE);
  }
}

void checkForBatteryCharged(uint8_t dischargerAddress, enum STATE *batteryState, uint8_t batteryChargeStatusPin, uint8_t batteryChargePin) {
  if(*batteryState == CHARGE) {
    int chargeStatus  = digitalRead(batteryChargeStatusPin);
    if(chargeStatus == 1) {
      setState(dischargerAddress, batteryChargePin, batteryState, DISCHARGE);
    }
  }
}


void setState(uint8_t dischargerAddress, uint8_t batteryChargePin, enum STATE *batteryState, enum STATE newState) {
  *batteryState = newState;
  switch(newState) {
    case IDLE: 
      setChargeStatus(batteryChargePin, 0);
      setDACVoltage(dischargerAddress, MCP4736_ZERO_VOLT);
      break;
   case CHARGE:
      setDACVoltage(dischargerAddress, MCP4736_ZERO_VOLT);
      setChargeStatus(batteryChargePin, 1);
      break;
   case DISCHARGE:
      setChargeStatus(batteryChargePin, 0);
      setDACVoltage(dischargerAddress, MCP4736_ONE_VOLT);      
      break;   
   default: 
      setDACVoltage(dischargerAddress, MCP4736_ZERO_VOLT);
      setChargeStatus(batteryChargePin, 0);
      break;
  }
}

void setupDisplay() {
  display.begin(84, 48);
  display.setCursor(0,0);
  display.print("18650 Tester");
}

void updateDisplayLine(uint8_t battery, enum STATE *batteryState, uint8_t voltagePin, double *capacity) {
  double voltage = readVoltage(voltagePin);
  display.print(battery);
  switch(*batteryState) {
    case IDLE: display.print("I"); break;
    case CHARGE: display.print("C"); break;
    case DISCHARGE: display.print("D"); break;
  }
  display.print(" ");
  display.print((int)(*capacity * 1000));
  display.print(" ");
  display.print(voltage);
}

void updateDisplay() {
  display.setCursor(0,0);
  display.println("18650 Tester");
  display.setCursor(0,1);
  updateDisplayLine(1, &battery01State, battery01VoltagePin, &battery01Capacity);
  display.setCursor(0,2);
  updateDisplayLine(2, &battery02State, battery02VoltagePin, &battery02Capacity);
}

void setup() {
  Wire.begin();
  setState(battery01DischargerAddress, battery01ChargePin, &battery01State, IDLE);
  setState(battery02DischargerAddress, battery02ChargePin, &battery02State, IDLE);
  setupDisplay();
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(battery01ChargeStatusPin, INPUT_PULLUP);
  pinMode(battery02ChargeStatusPin, INPUT_PULLUP);
  pinMode(battery01ChargePin, OUTPUT);
  pinMode(battery02ChargePin, OUTPUT);
  delay(2000);
}

void loop() {

  logMilliampHoursForTheLastSecond(&battery01Capacity, &battery01State);
  logMilliampHoursForTheLastSecond(&battery02Capacity, &battery02State);

  checkForBatteryCharged(battery01DischargerAddress, &battery01State, battery01ChargeStatusPin, battery01ChargePin);
  checkForBatteryCharged(battery02DischargerAddress, &battery02State, battery02ChargeStatusPin, battery02ChargePin);

  protectFromOverDischarge(battery01DischargerAddress, &battery01State, battery01VoltagePin, battery01ChargePin);
  protectFromOverDischarge(battery02DischargerAddress, &battery02State, battery02VoltagePin, battery02ChargePin);

  uint8_t buttonStatus = digitalRead(buttonPin);
  if(buttonStatus == 0) {
    setState(battery01DischargerAddress, battery01ChargePin, &battery01State, CHARGE);
    setState(battery02DischargerAddress, battery02ChargePin, &battery02State, CHARGE); 
    updateDisplay();
    delay(9000);
  } else {
    updateDisplay();  
  }
  
  delay(1000);
}
