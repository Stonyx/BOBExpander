// BOB Expander
// Copyright (C) 2016 Stonyx
//
// This software is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License.
//
// You can redistribute and/or modify this software for non-commerical purposes under the terms
// of the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
//
// This software is provided "as is" without express or implied warranty.

// Debug related definitions
// #define DEBUG
#ifdef DEBUG
  #define DEBUG_SERIAL_BEGIN() Serial.begin(9600)
  #define DEBUG_LOG(string) Serial.print(string)
  #define DEBUG_LOG_LN(string) Serial.println(string)
#else
  #define DEBUG_SERIAL_BEGIN()
  #define DEBUG_LOG(string)
  #define DEBUG_LOG_LN(string)
#endif
#define DEBUG_LOG_FREE_RAM() DEBUG_LOG(F("Free RAM: ")); DEBUG_LOG_LN(FreeRam())

// Define digital input pins
#define DIGITAL_INPUT_PINS 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4, A5

// Define analog input pins
#define ANALOG_INPUT_PINS A6, A7

// Define digital output pins
#define DIGITAL_OUTPUT_PINS 0, 1, 2, 3, 4, 5

// Define counts
#define INPUT_BANKS_COUNT 4
#define INPUT_PINS_PER_BANK_COUNT 4
#define OUTPUT_FLAG_PINS_COUNT 2
#define OUTPUT_STATE_PINS_COUNT INPUT_PINS_PER_BANK_COUNT

// Define structures
struct InputPin
{
  byte pin;
  boolean digital : 2;
  boolean previousState : 3;
  boolean state : 3;
};
struct InputBank
{
  byte number;
  InputPin pins[INPUT_PINS_PER_BANK_COUNT];
};
struct OutputPin
{
  byte pin;
};
struct OutputBank
{
  OutputPin flagPins[OUTPUT_FLAG_PINS_COUNT];
  OutputPin statePins[OUTPUT_STATE_PINS_COUNT];
};

// Define global variables
static InputBank gInputBanks[INPUT_BANKS_COUNT];
static OutputBank gOutputBank;

// Setup code
void setup()
{
  // Populate the input banks global variable and set known states on the input pins
  byte digitalInputPins[] = { DIGITAL_INPUT_PINS };
  byte analogInputPins[] = { ANALOG_INPUT_PINS };
  for (int i = 0; i < INPUT_BANKS_COUNT; ++i)
  {
    for (int j = 0; j < INPUT_PINS_PER_BANK_COUNT; ++j)
    {
      // Populate the data
      gInputBanks[i].number = i;
      if (i * INPUT_BANKS_COUNT + j < sizeof(digitalInputPins))
      {
        gInputBanks[i].pins[j].pin = digitalInputPins[i * INPUT_BANKS_COUNT + j];
        gInputBanks[i].pins[j].digital = true;
      }
      else
      {
        gInputBanks[i].pins[j].pin = analogInputPins[i * INPUT_BANKS_COUNT + j - sizeof(digitalInputPins)];
        gInputBanks[i].pins[j].digital = false;
      }
      gInputBanks[i].pins[j].previousState = LOW;
      gInputBanks[i].pins[j].state = LOW;

      // Set the input pin to input mode and enable the pullup resistor if its available
      pinMode(gInputBanks[i].pins[j].pin, (gInputBanks[i].pins[j].digital ? INPUT_PULLUP : INPUT));
    }
  }  

  // Populate the output bank global variable and set known states on the output pins
  byte digitalOutputPins[] = { DIGITAL_OUTPUT_PINS };
  for (int i = 0; i < OUTPUT_FLAG_PINS_COUNT; ++i)
  {
    // Popualte the data
    gOutputBank.flagPins[i].pin = digitalOutputPins[i];

    // Set the output pin to floating
    pinMode(gOutputBank.flagPins[i].pin, INPUT);
  }
  for (int i = 0; i < OUTPUT_STATE_PINS_COUNT; ++i)
  {
    // Populate the data
    gOutputBank.statePins[i].pin = digitalOutputPins[i + OUTPUT_FLAG_PINS_COUNT];

    // Set the output pin to floating
    pinMode(gOutputBank.statePins[i].pin, INPUT);
  }
}

// Main loop
void loop()
{
  // Loop through the input banks
  InputBank changedInputBanks[INPUT_BANKS_COUNT];
  byte changedInputBanksCount = 0;
  InputBank unchangedInputBanks[INPUT_BANKS_COUNT];
  byte unchangedInputBanksCount = 0;
  for (int i = 0; i < INPUT_BANKS_COUNT; ++i)
  {
    // Loop through the input pins in this bank
    boolean changed = false;
    for (int j = 0; j < INPUT_PINS_PER_BANK_COUNT; ++j)
    {
      // Populate the previous input pin state
      gInputBanks[i].pins[j].previousState = gInputBanks[i].pins[j].state;

      // Populate the current input pin state
      if (gInputBanks[i].pins[j].digital)
        gInputBanks[i].pins[j].state = digitalRead(gInputBanks[i].pins[j].pin);
      else
        gInputBanks[i].pins[j].state = (analogRead(gInputBanks[i].pins[j].pin) > 614/*.4*/ ? HIGH : LOW);

      // Check if the state has changed
      if (gInputBanks[i].pins[j].state != gInputBanks[i].pins[j].previousState)
        changed = true;
    }

    // Add this bank to one of the input bank arrays
    if (changed)
      memcpy(&changedInputBanks[changedInputBanksCount++], &gInputBanks[i], sizeof(InputBank));
    else
      memcpy(&unchangedInputBanks[unchangedInputBanksCount++], &gInputBanks[i], sizeof(InputBank));
  }

  // Consolidate the changed and unchanged input banks arrays
  InputBank inputBanks[INPUT_BANKS_COUNT];
  memcpy(inputBanks, changedInputBanks, sizeof(InputBank) * changedInputBanksCount);
  memcpy(&inputBanks[changedInputBanksCount], unchangedInputBanks, sizeof(InputBank) * unchangedInputBanksCount);

  // Loop through the input banks
  for (int i = 0; i < INPUT_BANKS_COUNT; ++i)
  {
    // Loop through the output flag pins
    for (int j = 0; j < OUTPUT_FLAG_PINS_COUNT; ++j)
    {
      // Check the corresponding bit value of the input bank number
      if (inputBanks[i].number & (1 << j))
      {
        // Pull down the output pin
        pinMode(gOutputBank.flagPins[j].pin, OUTPUT);
        digitalWrite(gOutputBank.flagPins[j].pin, LOW);
      }
      else
      {
        // Float the output pin
        pinMode(gOutputBank.flagPins[j].pin, INPUT);
      }
    }

    // Loop through the output state pins
    for (int j = 0; j < OUTPUT_STATE_PINS_COUNT; ++j)
    {
      if (inputBanks[i].pins[j].state)
      {
        // Pull down the output pin
        pinMode(gOutputBank.statePins[j].pin, OUTPUT);
        digitalWrite(gOutputBank.statePins[j].pin, LOW);
      }
      else
      {
        // Float the output pin
        pinMode(gOutputBank.statePins[j].pin, INPUT);
      }
    }
  }
  
  // Wait 1 second
  delay(1000);
}
