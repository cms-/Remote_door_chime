#include <VirtualWire.h>
/*
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
*/
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // F Malpartida's NewLiquidCrystal library

//const int txPwrPin = 1
#define switchPin 4
//const int battPin 5;
#define rxPin 6
#define battBridge 7
#define ampPwrPin 8
#define spkrPin 3
const float lowBatt = 5.0;

static String logFile = "door.csv";
#define SEP         ","
#define chipSelect  2

struct data_t {
  byte stn_id;
  byte msg_type;
  unsigned short msg_value;
  unsigned long ts;
};
data_t data;
unsigned long spkrLast = 0;
unsigned long battLast = 0;
const long spkrInterval = 1000;
const long battInterval = 3000;

const float battDiv = 0.494;
struct batt_t {
  float value = 99.9;
  unsigned long ts = 0;
  unsigned long local = 0;
};
batt_t battRemote;
batt_t battLocal;

void soundBuzzer(int type);
void logWrite(String logString);

void setup()
{
  Serial.begin(9600);	// Debugging only
  Serial.println("setup");
  pinMode(13, OUTPUT);
  pinMode(switchPin, INPUT);
  pinMode(battBridge, OUTPUT);
  pinMode(ampPwrPin, OUTPUT);
  if (digitalRead(switchPin) == LOW) {
    Serial.println("Mute switch enabled");
  }
  setupRadio();
  Serial.println("radio init");
  setupSD();

  delay(1000); // delay for serial
}

void setupSD(void) {
  if (SD.begin(chipSelect)) {
    Serial.println("SD init success");
  } else {
    Serial.println("SD init failed");
  }
}

void setupRadio(void) {
  vw_set_rx_pin(rxPin);
  // Initialise the IO and ISR
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);	 // Bits per sec
  vw_rx_start();       // Start the receiver PLL running
}

void loop() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;

  while (digitalRead(switchPin) == LOW) {
     
    
    /*
     *  Check battery values on interval and soundBuzzer if voltage
     *  values have dropped below above-specified variables.
     */
    if (millis() - battLast > battInterval) {
      
      if (battRemote.value < lowBatt) {
        String logString = String(0) + SEP + String(millis()) + SEP;
        logString += String(battRemote.ts) + SEP + String(battRemote.value);
        logWrite(logString);
        soundBuzzer(2);
        Serial.println(logString);
        lcd.setCursor(13,0);
        lcd.print("BAT LOW");
      }
      
      battLast = millis();
    }
    
    for (int i=0; i<5; i++) {
      if (vw_get_message(buf, &buflen)) {
        // Got good message, process if it matches struct data_t
        if (buflen == sizeof(data_t)) {
          memcpy(&data, buf, sizeof(data));
          processData(data);
        }
      }
    }
  }
  //setWdt();
}

void setWdt() {
  cli(); // disable global interrupts
  // prep the watchdog register
  WDTCSR |= ((1<<WDCE) | (1<<WDE));
  // set interrupt mode with a timeout of 1 sec
  WDTCSR = ((1<<WDIE) | (1<<WDP2));
  //WDTCR = ((1<<WDIE));
  sei(); // enable global interrupts
}

void processData(struct data_t data) {
  // takes action based on data packet received
  // set up log string
  String logString = String(data.stn_id) + SEP + String(data.msg_type) + SEP;
  // msg_type 1: door open
  if ((data.msg_type == 1) && (millis() - spkrLast > spkrInterval)) {
    logString += String(millis()) + SEP + String(data.ts) + SEP;
    logWrite(logString);
    Serial.println(logString);
    lcd.setCursor(2,9);
    lcd.print("DOOR OPEN");
    soundBuzzer(data.msg_type);
    spkrLast = millis();
  }
  // msg_type 2: remote battery voltage
  if (data.msg_type == 2) {
    battRemote.value = (data.msg_value / 1023.0 * 5.0) / battDiv;
    if (battRemote.ts != data.ts) {
      int result = 0;
      battRemote.ts = data.ts;
      logString += String(millis()) + SEP + String(battRemote.ts) + SEP;
      logString += String(battRemote.value);
      logWrite(logString);
      Serial.println(logString);
      lcd.setCursor(0,9);
      lcd.print(battRemote.value);
    }
  }
}

void soundBuzzer(int type) {
  // sounds a buzzer based on supplied type
  // type 1) door opened
  // type 2) battery low
  // type 3) alarm
  digitalWrite(ampPwrPin, LOW); // turn on amp
  delay(200);
  int i;
  if (type == 1) {
    // door opened      
    for(int i = 440; i<480; i += 5) {
      tone(3, i, 185);
      delay(i-280);
      noTone(3);
      tone(3, i, 150);
      delay(i-240);
      noTone(3);
    }
  }
  if (type == 2) {
    // batt low
    for (int i = 0; i<2; i++) {
      tone(spkrPin, 2500, 35);
      delay(120);  
    }
  }
  if (type == 3) {
    // alarm
    // not yet implemented
  }
  digitalWrite(ampPwrPin, HIGH); // turn off amp
}

void readBattery(int pin, int ratio) {
  // reads supplied pin and returns an int representing voltage times 10
  // divides by ratio to obtain the actual voltage
  ADCSRA |= (1<<ADEN); // turn on ADC
  digitalWrite(battBridge, HIGH); // Turn on battery bridge     
  // read the battery here
  ADCSRA &= ~(1<<ADEN); // disable ADC
  digitalWrite(battBridge, LOW);
  // return the reading
}

void logWrite(String logString) {
  File dataFile = SD.open(logFile, FILE_WRITE);
  if (dataFile) {
    dataFile.println(logString);
    dataFile.close();
  }
  else {
    Serial.println("error opening " + String(logFile));
  }
}

ISR(WDT_vect) {
}
