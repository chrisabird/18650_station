#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

#define batteryArraySize 6
#define batteriesPerSelector 6
#define numberOfChargers 6
#define numberOfDischargers 2
#define numberOfSelectors 2
#define supplyVoltage 5.0

#define buttonPin 8

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATA 0x14
#define MCP23017_OLATB 0x15

#define UNUSED 255

#define DEBUG 0

enum STATE {
  WAITING_CHARGE,
  CHARGE,
  WAITING_DISCHARGE,
  DISCHARGE,
  DONE
};

enum SELECTOR {
  CHARGER,
  DISCHARGER
};

uint8_t page = 0;
uint8_t cycleStarted = 0;
uint8_t dischargerStatus[numberOfDischargers] = {UNUSED, UNUSED};
uint8_t dischargerPins[numberOfDischargers] = {A0, A1};
double batteryCapacities[batteryArraySize];
enum STATE batteryStates[batteryArraySize];
uint8_t chargerPins[numberOfChargers] = {2, 3, 4, 5, 6, 7};
uint8_t selectorAddresses[numberOfSelectors] = {0x20, 0x21};
uint8_t chargerStatus[numberOfChargers] = {UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED};


LiquidCrystal_PCF8574 display(0x3F);

uint8_t readIOExpander(uint8_t i2caddr, uint8_t addr) {
  Wire.beginTransmission(i2caddr);
  Wire.write(addr);
  Wire.endTransmission();
  Wire.requestFrom(i2caddr, 1);
  return Wire.read();
}

void updateIOExpander(uint8_t i2caddr, uint8_t regAddr, uint8_t regValue) {
  Wire.beginTransmission(i2caddr);
  Wire.write(regAddr);
  Wire.write(regValue);
  Wire.endTransmission();
}

void updateSelector(int8_t batteryNumber, enum SELECTOR reg, uint8_t enabled) {
  uint8_t selectorNumber = batteryNumber / batteriesPerSelector;
  uint8_t i2caddr = selectorAddresses[selectorNumber];
  uint8_t gpio = readIOExpander(i2caddr, (reg == CHARGER) ? MCP23017_OLATA : MCP23017_OLATB);
  bitWrite(gpio, batteryNumber % batteriesPerSelector, enabled);
  updateIOExpander(selectorAddresses[selectorNumber], (reg == CHARGER) ? MCP23017_GPIOA : MCP23017_GPIOB, gpio);
}

uint8_t setBatteryChargeStatus(uint8_t batteryNumber, uint8_t enabled) {
  uint8_t chargerNumber = batteryNumber % numberOfChargers;
  if (enabled == 1 && chargerStatus[chargerNumber] != UNUSED) {
    return 0;
  }
  updateSelector(batteryNumber, CHARGER, enabled);
  chargerStatus[chargerNumber] = (enabled == 1) ? batteryNumber : UNUSED;
  return 1;
}

uint8_t setBatteryDischargeStatus(uint8_t batteryNumber, uint8_t enabled) {
  uint8_t dischargerNumber = batteryNumber % numberOfDischargers;
  if (enabled == 1 && dischargerStatus[dischargerNumber] != UNUSED) {
    return 0;
  }
  updateSelector(batteryNumber, DISCHARGER, enabled);
  dischargerStatus[dischargerNumber] = (enabled == 1) ? batteryNumber : UNUSED;
  return 1;
}

void logMilliampHoursForTheLastSecond() {
  for (uint8_t j = 0; j < numberOfDischargers; j++) {
    uint8_t batteryNumber = dischargerStatus[j];
    if(batteryNumber != UNUSED) {
      batteryCapacities[batteryNumber] = batteryCapacities[batteryNumber] + 0.000277777777778;
   }
  }
}

double readVoltage(uint8_t voltagePin) {
  return (supplyVoltage * analogRead(voltagePin)) / 1024;
}

void protectFromOverDischarge() {
  for (uint8_t j = 0; j < numberOfDischargers; j++) {
    uint8_t batteryNumber = dischargerStatus[j];
    if(batteryNumber != UNUSED) {
      double voltage = readVoltage(dischargerPins[j]);
      
      #if defined(DEBUG)
      Serial.print("Battery ");
      Serial.print(batteryNumber);
      Serial.print(" discharge voltage ");
      Serial.println(voltage);
      #endif
      
      if (voltage <= 3.0) {
        setState(batteryNumber, DONE);
      }
    }
  }
}

void checkForBatteryCharged() {
  for (uint8_t j = 0; j < numberOfChargers; j++) {
    uint8_t batteryNumber = chargerStatus[j];
    if(batteryNumber != UNUSED) {
      int chargeStatus = digitalRead(chargerPins[j]);
      
      #if defined(DEBUG)
      Serial.print("Battery ");
      Serial.print(batteryNumber);
      Serial.print(" charge status ");
      Serial.println(chargeStatus);
      #endif
      
      if (chargeStatus == 1) {
        setState(batteryNumber, WAITING_DISCHARGE);
      }
    }
  }
}

void checkForFreeChargers() {
  uint8_t i;
  for (uint8_t i = 0; i < batteryArraySize; i++ ) {
    if(batteryStates[i] == WAITING_CHARGE) {
      setState(i, CHARGE);  
    }
  }
}

void checkForFreeDischargers() {
  uint8_t i;
  for (uint8_t i = 0; i < batteryArraySize; i++ ) {
    if(batteryStates[i] == WAITING_DISCHARGE) {
      setState(i, DISCHARGE);  
    }
  }
}

void checkForDone() {
  uint8_t i;
  bool done = true;
  for (uint8_t i = 0; i < batteryArraySize; i++ ) {
    if(batteryStates[i] != DONE) {
        done = false;
    }
  }
  cycleStarted = done ? 0 : 1;
}

void setState(uint8_t batteryNumber, enum STATE newState) {
  switch (newState) {
    case WAITING_CHARGE:
      batteryStates[batteryNumber] = newState;
      setBatteryChargeStatus(batteryNumber, 0);
      setBatteryDischargeStatus(batteryNumber, 0);
      #if defined(DEBUG)
      Serial.print("Battery ");
      Serial.print(batteryNumber);
      Serial.println(" being waiting on charge ");
      #endif
      break;
    case CHARGE:    
      setBatteryDischargeStatus(batteryNumber, 0);
      if(setBatteryChargeStatus(batteryNumber, 1) == 1) {
        batteryStates[batteryNumber] = newState;  
        #if defined(DEBUG)
        Serial.print("Battery ");
        Serial.print(batteryNumber);
        Serial.println(" being charged ");
        #endif
      }
      break;
    case WAITING_DISCHARGE:
      batteryStates[batteryNumber] = newState;
      setBatteryChargeStatus(batteryNumber, 0);
      setBatteryDischargeStatus(batteryNumber, 0);
      #if defined(DEBUG)
      Serial.print("Battery ");
      Serial.print(batteryNumber);
      Serial.println(" being waiting on discharge ");
      #endif
      break;
    case DISCHARGE:
      setBatteryChargeStatus(batteryNumber, 0);
      if(setBatteryDischargeStatus(batteryNumber, 1) == 1) {
        batteryStates[batteryNumber] = newState;
        #if defined(DEBUG)
        Serial.print("Battery ");
        Serial.print(batteryNumber);
        Serial.println(" being discharged ");
        #endif
      } 
      break;
    default:
      batteryStates[batteryNumber] = newState;
      setBatteryDischargeStatus(batteryNumber, 0);
      setBatteryChargeStatus(batteryNumber, 0);
      #if defined(DEBUG)
      Serial.print("Battery ");
      Serial.print(batteryNumber+1);
      Serial.println(" being done ");
      #endif
      break;
  }
}

void updateDisplayLine(uint8_t batteryNumber) {
  if (batteryNumber < 10) {
    display.print("0");
  }
  display.print(batteryNumber);
  switch (batteryStates[batteryNumber]) {
    case WAITING_CHARGE:
    case WAITING_DISCHARGE:
      display.print("W ");
      break;
    case CHARGE:
      display.print("C");
      break;
    case DISCHARGE:
      display.print("D ");
      display.print((int)(batteryCapacities[batteryNumber] * 1000));
      break;
    case DONE:
      display.print("F ");
      display.print((int)(batteryCapacities[batteryNumber] * 1000));
      break;
  }
}

void updateDisplay() {
  uint8_t i, j;
  uint8_t startBattery = page * batteriesPerSelector;
  uint8_t endBattery = startBattery + batteriesPerSelector;

  for (uint8_t j = startBattery; j < endBattery; j++ ) {
    display.setCursor(j > (startBattery + 2) ? 9 : 0, j % 3);
    updateDisplayLine(j);
  }
  display.setCursor(18, 4);
  display.print("P");
  display.print(page);

  if (endBattery == batteryArraySize) {
    page = 0;
  } else {
    page++;
  }
}

void voltageCheck() {
  for (uint8_t j = 0; j < batteryArraySize; j++ ) {
    updateSelector(j, DISCHARGER, 1);
    delay(100);
    Serial.print(j);
    Serial.print(" ");
    Serial.print(readVoltage(A0));
    Serial.print(" ");
    Serial.println(readVoltage(A1));
    updateSelector(j, DISCHARGER, 0);
  }
}

void loop() {

  if(cycleStarted == 1) {
    logMilliampHoursForTheLastSecond();
    checkForBatteryCharged();
    protectFromOverDischarge();
    checkForFreeChargers();
    checkForFreeDischargers();
    checkForDone();
  }

  uint8_t buttonStatus = digitalRead(buttonPin);

  #if defined(DEBUG)
  if (Serial.available() > 0) {
    char in = Serial.read();
    if(in == 'g') {
      buttonStatus = 0; 
    }
    if(in == 't') {
      voltageCheck();
    }
  }
  #endif
  
  if (buttonStatus == 0) {
    cycleStarted = 1;
    checkForFreeChargers();
    updateDisplay();
    delay(500);
  } else {
    updateDisplay();
  }

  delay(1000);
}

void setupDisplay() {
  display.begin(20, 4);
  display.setBacklight(255);
}

void setupSelectors() {
  uint8_t i;
  for (uint8_t i = 0; i < (batteryArraySize / batteriesPerSelector); i++ ) {
    updateIOExpander(selectorAddresses[i], MCP23017_IODIRA, 0x00);
    updateIOExpander(selectorAddresses[i], MCP23017_GPIOA, 0x00);
    updateIOExpander(selectorAddresses[i], MCP23017_IODIRB, 0x00);
    updateIOExpander(selectorAddresses[i], MCP23017_GPIOB, 0x00);
  }
}

void setupDefaultState() {
  uint8_t i;
  for (uint8_t i = 0; i < batteryArraySize; i++ ) {
    setState(i, WAITING_CHARGE);
    batteryCapacities[i] = 0;
  }
}

void setupButtonIOPins() {
  pinMode(buttonPin, INPUT_PULLUP);
}

void setupChargeIOPins() {
  for (uint8_t i = 0; i < numberOfChargers; i++ ) {
    pinMode(chargerPins[i], INPUT_PULLUP);
  }
}

void setup() {
  #if defined(DEBUG)
  Serial.begin(9600);
  Serial.println("Discharge Station in Debug Mode");
  #endif
  
  Wire.begin();
  setupDisplay();
  setupSelectors();
  setupDefaultState();
  setupButtonIOPins();
  setupChargeIOPins();
}


