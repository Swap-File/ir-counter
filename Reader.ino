#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7, 8);

uint8_t payload[9];

void setup() {
  Serial.begin(115200);
  printf_begin();

  // Setup and configure rf radio
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(0);
  radio.setPayloadSize(9);    //StationID  In1 In2 In3 In4  Out1 Out2 Out3 Out4
  radio.setDataRate(RF24_250KBPS);
  uint8_t address[] = { 0xCC, 0xCE, 0xCC, 0xCE, 0xCC };

  radio.openReadingPipe(0, address);

  radio.startListening();                 // Start listening
  radio.printDetails();
}

void loop(void) {

  byte pipeNo = 99;
  byte gotByte;                                       // Dump the payloads until we've gotten everything
  while ( radio.available(&pipeNo)) {
    radio.read( &payload, 9 );
    uint32_t in_count =  payload[1] << 24 |   payload[2] << 16 | payload[3] << 8 | payload[4];
    uint32_t out_count =  payload[5] << 24 |   payload[6] << 16 | payload[7] << 8 | payload[8] ;
    Serial.print(in_count);
    Serial.print(" ");
    Serial.println(out_count);
  }
}
