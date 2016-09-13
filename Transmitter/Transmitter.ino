//NOTE DOUBLE CHECK HZ WITH SCOPE
//CURRENT 8MHZ TIMING HAS NOT BEEN VERFIED!

#include <avr/power.h>

const byte LED = 9;  // Timer 1 "A" output: OC1A

ISR (TIMER4_COMPA_vect)
{
  TCCR1A ^= _BV (COM1A0) ;  // Toggle OC1A on Compare Match

  if ((TCCR1A & _BV (COM1A0)) == 0)
    digitalWrite (LED, LOW);  // ensure off

}  // end of TIMER2_COMPA_vect

void setup() {
  
  delay(10000);  //IMPORTANT!!! Allow time for changing firmware, or otherwise if we disable the USB too soon we are stuck and only a re-flash of the bootloader may help

  power_adc_disable();
  power_usart0_disable();
  power_spi_disable();
  power_twi_disable();
  power_timer0_disable();
  power_timer2_disable();
  power_timer3_disable();
  power_usart1_disable();
  power_usb_disable();
  USBCON |= (1 << FRZCLK);// Freeze the USB Clock
  PLLCSR &= ~(1 << PLLE);// Disable the USB Clock (PPL)
  USBCON &= ~(1 << USBE);// Disable the USB

  clock_prescale_set(clock_div_2);

  pinMode (LED, OUTPUT);

  // set up Timer 1 - gives us 38.095 MHz (correction: 38.095 KHz)
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV (CS10);   // CTC, No prescaler
  //OCR1A =  209;          // compare A register value (210 * clock speed)
  OCR1A =  104;

  //  = 13.125 nS , so frequency is 1 / (2 * 13.125) = 38095

  // Timer 2 - gives us our 1 mS counting interval
  // 16 MHz clock (62.5 nS per tick) - prescaled by 128
  //  counter increments every 8 uS.
  // So we count 125 of them, giving exactly 1000 uS (1 mS)
  TCCR4A = _BV (WGM41) ;   // CTC mode
  // OCR4A  = 124;            // count up to 125  (zero relative!!!!)
  OCR4A  = 62;            // count up to 125  (zero relative!!!!)
  TIMSK4 = _BV (OCIE4A);   // enable Timer2 Interrupt
  TCCR4B =  _BV (CS40) | _BV (CS42) ;  // prescaler of 128

}  // end of setup

void loop()
{
  // all done by interrupts
}