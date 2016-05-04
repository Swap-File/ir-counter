#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7, 8);

#define MAX_STATIONS 32  //max number of stations to fit in ram at a time

struct counter_data {
  uint8_t station_id;
  uint32_t in_count;
  uint32_t out_count;
};

struct counter_data counters[MAX_STATIONS];
uint8_t counter_index = 0;
uint8_t counters_active = 0;

uint32_t display_update = 0;

void setup() {
  Serial.begin(115200);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(0);
  radio.setPayloadSize(9);    //StationID  In1 In2 In3 In4  Out1 Out2 Out3 Out4
  radio.setDataRate(RF24_250KBPS);
  uint8_t address[] = { 0xCC, 0xCE, 0xCC, 0xCE, 0xCC };
  radio.openReadingPipe(0, address);
  radio.startListening();
}

void loop(void) {

  uint8_t pipeNo;
  uint8_t payload[9];
  while ( radio.available(&pipeNo)) {

    radio.read( &payload, 9 );

    uint8_t location;
    boolean found_match = false;
    for (uint8_t i = 0; i < counters_active; i++) {  //check active counters
      location = (counter_index + i ) % MAX_STATIONS; //calc actual array location
      if (counters[location].station_id == payload[0] ) {  //check if match
        found_match = true; //set found flag
        break; //stop checking rest of array
      }
    }
    if (found_match == false) { //add new entry
      if (counters_active + 1 < MAX_STATIONS) { //ensure room in array
        location = (counter_index + counters_active) % MAX_STATIONS; //calc actual array location
        counters_active++;
        found_match = true;
      } else {
        Serial.print(F("Too Full! Dropping incoming data!"));
      }
    }
    if (found_match == true) {
      counters[location].station_id = payload[0];
      counters[location].in_count = payload[1] << 24 | payload[2] << 16 | payload[3] << 8 | payload[4];
      counters[location].out_count = payload[5] << 24 | payload[6] << 16 | payload[7] << 8 | payload[8];
    }
  }

  if (Serial.available()) {
    uint8_t character = Serial.read();

    if (character == 'n') { //next
      if (counters_active > 0) {
        counters_active--;
        Serial.print(F("Going to next Counter!"));
      } else {
        Serial.print(F("No other counters available!"));
      }
      counter_index = (counter_index + 1) % MAX_STATIONS;
      update_display();
    }

    if (millis() - display_update > 1000) {
      update_display();
    }
  }
}

void update_display() {
  Serial.print(F("-----------------------"));
  Serial.print(F("Counters Seen: "));
  Serial.println(counters_active);
  if (counters_active > 0) {
    Serial.print(F("Current Counter: "));
    Serial.println(counters[counter_index].station_id);
    Serial.print(F("In: "));
    Serial.println(counters[counter_index].in_count);
    Serial.print(F("Out: "));
    Serial.println(counters[counter_index].out_count);
  }
  if (counters_active + 1 == MAX_STATIONS) {
    Serial.print(F("Warning: Memory is Full!"));
  }
}

