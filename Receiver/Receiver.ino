#include <EEPROMWearLeveler.h>
#include "RF24.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <FlexiTimer2.h> //must use timer2 since it runs in power save mode

EEPROMWearLeveler eepromwl(1024, 128); // 32x more read/write cycles, should be good for 3.2 million per direction
RF24 radio(7, 8);

#define STATION_ID 2  //CHANGE ON EVERY UNIT!

#define IN_OUT_READY 0
#define IN_STARTING 1
#define OUT_STARTING 2
#define IN_ENDING 3
#define OUT_ENDING 4

#define LEFT_SENSOR_PIN 2
#define RIGHT_SENSOR_PIN 3

volatile uint32_t in_count = 0;
volatile uint32_t out_count = 0;
volatile uint16_t time_since_output = 0;
volatile uint8_t isr_state = 0;

void setup() {

  delay(10000);  //IMPORTANT!!! Allow time for changing firmware, or otherwise if we disable the USB too soon we are stuck and only a re-flash of the bootloader may help

  radio.begin();
  radio.setPALevel(RF24_PA_LOW); //lowest power mode
  radio.setAutoAck(0);
  radio.setDataRate(RF24_250KBPS); //slowest speed
  radio.setPayloadSize(9);  //StationID In1 In2 In3 In4  Out1 Out2 Out3 Out4
  uint8_t address[] = { 0xCC, 0xCE, 0xCC, 0xCE, 0xCC };
  radio.openWritingPipe(address);

  pinMode(LEFT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(RIGHT_SENSOR_PIN, INPUT_PULLUP);

  in_count = ReadLongEEPROM(0);
  out_count = ReadLongEEPROM(4);

  attachInterrupt(digitalPinToInterrupt(LEFT_SENSOR_PIN), left_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RIGHT_SENSOR_PIN), right_isr, CHANGE);

  power_adc_disable();
  power_usart0_disable();
  power_twi_disable();

  //power_usart0_disable();
  power_timer0_disable();
  power_timer1_disable();

  power_spi_disable();

  //set_sleep_mode (SLEEP_MODE_PWR_SAVE);
  //sleep_enable();

  FlexiTimer2::set(2, 1.0 / 1000, check_for_motion); // call every ~2ms
  FlexiTimer2::start();
}

void loop() {
  sleep_cpu();
}

void left_isr() {
  FlexiTimer2::start();  //reset deadman's switch
  if (isr_state == IN_OUT_READY) {
    isr_state = IN_STARTING ;
  }
  else if (isr_state == OUT_STARTING) {
    isr_state = OUT_ENDING;
  }
}

void right_isr() {
  FlexiTimer2::start();  //reset deadman's switch
  if (isr_state == IN_OUT_READY) {
    isr_state = OUT_STARTING;
  }
  else if (isr_state == IN_STARTING) {
    isr_state = IN_ENDING;
  }
}

void check_for_motion() {
  FlexiTimer2::stop();
  time_since_output = time_since_output + FlexiTimer2::time_units;

  if (isr_state == IN_ENDING) {
    WriteLongEEPROM(0, ++in_count); //this may take ~10ms but thats OK since we should to wait to reset anyway
  }
  else if (isr_state == OUT_ENDING) {
    WriteLongEEPROM(4, ++out_count); //this may take ~10ms but thats OK since we should to wait to reset anyway
  }

  isr_state = IN_OUT_READY;

  if (time_since_output > 2000) {
    time_since_output = 0;
    output_data();
  }

  FlexiTimer2::start();
}

void WriteLongEEPROM(byte addr, unsigned long data) {
  eepromwl.write(addr + 0, (byte)((data >> 24) & 0xff));
  eepromwl.write(addr + 1, (byte)((data >> 16) & 0xff));
  eepromwl.write(addr + 2, (byte)((data >> 8) & 0xff));
  eepromwl.write(addr + 3, (byte)(data & 0xff));
}

uint32_t ReadLongEEPROM(byte addr) {
  uint32_t result;
  result = ((uint32_t)eepromwl.read(addr + 0)) << 24;
  result |= ((uint32_t)eepromwl.read(addr + 1)) << 16;
  result |= ((uint32_t)eepromwl.read(addr + 2)) << 8;
  result |= ((uint32_t)eepromwl.read(addr + 3));
  return result;
}

void output_data() {

  uint8_t payload[9];
  payload[0] = STATION_ID;

  Serial.print(in_count);
  Serial.print(' ');
  Serial.println(out_count);

  //snapshot data
  payload[1] = (in_count >> 24) & 0xFF;
  payload[2] = (in_count >> 16) & 0xFF;
  payload[3] = (in_count >> 8) & 0xFF;
  payload[4] = (in_count >> 0) & 0xFF;
  payload[5] = (out_count >> 24) & 0xFF;
  payload[6] = (out_count >> 16) & 0xFF;
  payload[7] = (out_count >> 8) & 0xFF;
  payload[8] = (out_count >> 0) & 0xFF;

  interrupts();

  power_timer0_enable();  //radio library needs timer0 for delays
  power_spi_enable();
  
  radio.powerUp();
  radio.write( &payload, 9 );
  radio.powerDown();
  
  power_spi_disable();
  power_timer0_disable();

}