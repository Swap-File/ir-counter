#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <LiquidCrystal.h>
// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(2, 3);

#define MAX_STATIONS 16  //max number of stations to fit in ram at a time

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

bool button_handled = false;
struct counter_data {
  uint8_t station_id;
  uint32_t in_count;
  uint32_t out_count;
};

struct counter_data counters[MAX_STATIONS];

uint8_t counter_index_previous = 255;
uint8_t counters_active_previous = 255;
uint8_t counter_index = 0;
uint8_t counters_active = 0;

uint32_t display_update = 0;

uint32_t animation_time = 0;
bool animation_frame = false;
bool animation_frame_previous = true;

bool update_screen = true;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
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

      if (location == counter_index) update_screen = true;

      counters[location].station_id = payload[0];
      counters[location].in_count = payload[1] << 24 | payload[2] << 16 | payload[3] << 8 | payload[4];
      counters[location].out_count = payload[5] << 24 | payload[6] << 16 | payload[7] << 8 | payload[8];

      animation_frame = !animation_frame;

    }
  }

  int button = read_LCD_buttons();
  if (button == btnNONE && button_handled == true) button_handled = false;

  if (button_handled == false) {
    if (button == btnDOWN) {
      update_screen = true;
      if (counters_active > 0)  counters_active--;
      counter_index = (counter_index + 1) % MAX_STATIONS;
    }
  }


  //only redraw screen if data changes
  if (update_screen) {
    if (counters_active > 0) {

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(counters[counter_index].station_id);

      lcd.setCursor(2, 1);
      lcd.print("O: ");
      lcd.print(counters[counter_index].out_count);

      lcd.setCursor(2, 0);
      lcd.print("I: ");
      lcd.print(counters[counter_index].in_count);

    } else {

      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Watching for");
      lcd.setCursor(2, 1);
      lcd.print("Counters...");
      update_screen = false;

    }
    update_screen = false;
  }

  if (counters_active > 0) {
    lcd.setCursor(0, 1);
  }

  else {
    if (millis() - animation_time > 300)  {
      animation_frame = !animation_frame;
      animation_time = millis();
    }
    lcd.setCursor(13, 1);
  }


  if (animation_frame == 0) lcd.print('|');
  else lcd.print('-');
 
}



uint16_t button_state_previous = btnNONE;
uint8_t button_count = 0;

int read_LCD_buttons() {              // read the buttons

  uint16_t adc_key_in = analogRead(0);

  uint8_t button_state = btnNONE;

  if (adc_key_in < 850)  button_state = btnSELECT;
  if (adc_key_in < 650)  button_state = btnLEFT;
  if (adc_key_in < 450)  button_state = btnDOWN;
  if (adc_key_in < 250)  button_state = btnUP;
  if (adc_key_in < 50)   button_state = btnRIGHT;

  if (button_state_previous == button_state)  button_count++;
  else                                        button_count = 0;

  button_state_previous = button_state;

  //debounce input, 5 reads must agree or will return none.
  if (button_count > 5) return button_state;
  else                  return btnNONE;

}

