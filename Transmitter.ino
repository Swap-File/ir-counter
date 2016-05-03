/*
 Copyright (c) 2014 NicoHood
 See the readme for credit to other people.

 IRL Send Button
 Sends IR signals on any pin. This uses Bitbanging.
 
 Press the button to send data.
 Turn interrupts off to get a better result if needed
 */

#include "IRLremote.h"

// choose any pin to send IR signals
const int pinSendIR = 3;


void setup() {
  // setup for the button
pinMode(3,OUTPUT);
}

void loop() {
    // send the data, no pin setting to OUTPUT needed
    uint16_t address = 0x6361;
    uint32_t command = 0xFE01;
    IRLwrite<IR_NEC>(pinSendIR, address, command);
    delay(100);
}
