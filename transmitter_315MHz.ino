#include <VirtualWire.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

//const int txPwrPin = 1;
const int doorPin = 0;
const int battPin = A2;
const int txPin = 3;
const int battBridge = 1;

struct data_t {
  byte stn_id;
  byte msg_type;
  unsigned short msg_value;
  unsigned long ts;
};

data_t data;
short readBattery(int pin);

volatile long counter = 0;
const long battInterval = 800;

void setup() {
  MCUSR = 0;
  wdt_disable();
  data.stn_id = 1;
  //pinMode(txPwrPin, OUTPUT);
  pinMode(battBridge, OUTPUT);
  // changed below to enable internal pullup resistor
  pinMode(doorPin, INPUT);
  setupRadio();
  setWdt();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA &= ~(1<<ADEN); // disable ADC
  digitalWrite(battBridge, LOW); // Turn off battery bridge (NPN)
}

void setupRadio() {
  vw_set_rx_pin(2);
  vw_set_ptt_pin(2);
  vw_set_tx_pin(txPin);
  vw_set_ptt_inverted(true);
  vw_setup(2000);
}

void loop() {
  sleep_mode();
  /*
   *  Run readBattery every interval
   */
  if (counter >= battInterval) {
    counter = 0;
    wdt_disable();
    data.msg_type = 2;
    data.msg_value = readBattery(battPin);
    data.ts = millis();
    
    vw_send((uint8_t*) &data, sizeof(data));
    vw_wait_tx(); // Wait until the whole message is gone 
  }
    
  // If the door is open, send a message.
  while (digitalRead(doorPin) == LOW) {
      wdt_disable();
      // compose message
      
      data.msg_type = 1;
      data.msg_value = 1;
      data.ts = millis();
      /*
      data.msg_type = 2;
      data.msg_value = readBattery(battPin);
      */
      // send message
      vw_send((uint8_t*) &data, sizeof(data));
      vw_wait_tx(); // Wait until the whole message is gone 
  }
  setWdt(); // enable watchdog
}

void setWdt() {
  cli(); // disable global interrupts
  // prep the watchdog register
  WDTCR |= ((1<<WDCE) | (1<<WDE));
  // set interrupt mode with a timeout of 1 sec
  WDTCR = ((1<<WDIE) | (1<<WDP1) | (1<<WDP2));
  //WDTCR = ((1<<WDIE));
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei(); // enable global interrupts
}

short readBattery(int pin) {
  // reads supplied pin and returns value proportional to voltage / 0.494
  ADCSRA |= (1<<ADEN); // turn on ADC
  digitalWrite(battBridge, HIGH); // Turn on battery bridge (NPN)    
  delay(250);
  short value = analogRead(pin);
  ADCSRA &= ~(1<<ADEN); // disable ADC
  digitalWrite(battBridge, LOW); // Turn off battery bridge
  return value;
}

ISR(WDT_vect) {
  counter++;
}
