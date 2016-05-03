# ir-counter
This is a bidirectional people counter built using an Arduino Micro with an IR LED as a Transmitter and another Arduino Micro with two VS1838B IR Sensors as a Receiver.

The Receiver stores all counts in EEPROM with wear leveling, and also has a nRF24L01 radio to broadcast it's counts to the Reader.

Expected battery life is ~4 days for the Transmitter and Receiver.  The Reader will have much shorter life because it will need to power a screen and continually power it's radio to listen for broadcasts.
