#include <FreqCounter.h>

//Pin mappings for both LEDs and photosensors
#define B0LED1_PIN 10 //Bank1 = Nitrate
#define B0LED2_PIN 12
#define B0SENSOR1_PIN 6
#define B0SENSOR2_PIN 7
#define B1LED1_PIN 11 //Bank2 = Phosphate
#define B1LED2_PIN 13
#define B1SENSOR1_PIN 5
#define B1SENSOR2_PIN 8

float bank0_blank;
float bank1_blank;
int   bank;

//Blocking wrapper function for the Frequency counter library
uint32_t frequencyCtr(int16_t len) {
  FreqCounter::start(len);
  while(FreqCounter::f_ready ==0);
  uint32_t result = FreqCounter::f_freq;
  return result;
}

//Perform boot-up initialization. Put all of the used output pins
//in the appropriate start state.
void initialize_photometer() {
  pinMode(B0LED1_PIN, OUTPUT);
  digitalWrite(B0LED1_PIN,LOW);
  pinMode(B0LED2_PIN, OUTPUT);
  digitalWrite(B0LED2_PIN,LOW);
  pinMode(B0SENSOR1_PIN, OUTPUT);
  digitalWrite(B0SENSOR1_PIN,HIGH);
  pinMode(B0SENSOR2_PIN, OUTPUT);
  digitalWrite(B0SENSOR2_PIN,HIGH);
  
  pinMode(B1LED1_PIN, OUTPUT);
  digitalWrite(B1LED1_PIN,LOW);
  pinMode(B1LED2_PIN, OUTPUT);
  digitalWrite(B1LED2_PIN,LOW);
  pinMode(B1SENSOR1_PIN, OUTPUT);
  digitalWrite(B1SENSOR1_PIN,HIGH);
  pinMode(B1SENSOR2_PIN, OUTPUT);
  digitalWrite(B1SENSOR2_PIN,HIGH);

  bank0_blank = 0.0f;
  bank1_blank = 0.0f;
  bank = 0;
}

//Select between Nitrate and Phosphate tests.
int8_t select_bank(String banka) {
  if (banka == "0")
    bank = 0;
  else if (banka == "1")
    bank = 1;
  else
    return -1;
  return 0;
}

//Get a transmittance reading for the selected bank as a ratio
//of the current itensity to a previously stored 'blank' intensity.
void get_reading(String blank_val, float* buffer) {
  if (bank == 0) {
    digitalWrite(B0LED1_PIN, HIGH);
    digitalWrite(B0LED2_PIN, HIGH);
    digitalWrite(B0SENSOR1_PIN, LOW);
    uint32_t ret = frequencyCtr(1000);
    digitalWrite(B0SENSOR1_PIN, HIGH);
    digitalWrite(B0SENSOR2_PIN, LOW);
    uint32_t ret1 = frequencyCtr(1000);
    digitalWrite(B0SENSOR2_PIN, HIGH);
    digitalWrite(B0LED1_PIN, LOW);
    digitalWrite(B0LED2_PIN, LOW);
    *buffer = (bank0_blank*((float)ret1/(float)ret));
  } else if (bank == 1) {
    digitalWrite(B1LED1_PIN, HIGH);
    digitalWrite(B1LED2_PIN, HIGH);
    digitalWrite(B1SENSOR1_PIN, LOW);
    uint32_t ret = frequencyCtr(1000);
    digitalWrite(B1SENSOR1_PIN, HIGH);
    digitalWrite(B1SENSOR2_PIN, LOW);
    uint32_t ret1 = frequencyCtr(1000);
    digitalWrite(B1SENSOR2_PIN, HIGH);
    digitalWrite(B1LED1_PIN, LOW);
    digitalWrite(B1LED2_PIN, LOW);
    *buffer = (bank1_blank*((float)ret1/(float)ret));
  }
}

void blank_bank() {
  if (bank == 0) {
    digitalWrite(B0LED1_PIN, HIGH);
    digitalWrite(B0LED2_PIN, HIGH);
    digitalWrite(B0SENSOR1_PIN, LOW);
    uint32_t ret = frequencyCtr(1000);
    digitalWrite(B0SENSOR1_PIN, HIGH);
    digitalWrite(B0SENSOR2_PIN, LOW);
    uint32_t ret1 = frequencyCtr(1000);
    digitalWrite(B0SENSOR2_PIN, HIGH);
    digitalWrite(B0LED1_PIN, LOW);
    digitalWrite(B0LED2_PIN, LOW);
    bank0_blank = ((float)ret)/((float)ret1);
  } else if (bank == 1) {
    digitalWrite(B1LED1_PIN, HIGH);
    digitalWrite(B1LED2_PIN, HIGH);
    digitalWrite(B1SENSOR1_PIN, LOW);
    uint32_t ret = frequencyCtr(1000);
    digitalWrite(B1SENSOR1_PIN, HIGH);
    digitalWrite(B1SENSOR2_PIN, LOW);
    uint32_t ret1 = frequencyCtr(1000);
    digitalWrite(B1SENSOR2_PIN, HIGH);
    digitalWrite(B1LED1_PIN, LOW);
    digitalWrite(B1LED2_PIN, LOW);
    bank1_blank = ((float)ret)/((float)ret1);
  }
}

String commandString = "";
boolean commandReceived = 0;
boolean argumentReceived = 0;
String argumentString = "";
uint8_t return_buffer[64];

#define STATUS_GREEN digitalWrite(A0,LOW);digitalWrite(A1,HIGH);
#define STATUS_RED digitalWrite(A1,LOW);digitalWrite(A0,HIGH);

void setup() {
  Serial.begin(9600);
  commandString.reserve(8);
  argumentString.reserve(16);
  commandString = "";
  argumentString = "";
  
  pinMode(A0,OUTPUT);
  pinMode(A1,OUTPUT);
  STATUS_RED
  photometer_callback(String("boot"),String(" "),return_buffer);
  photometer_callback(String("select"),String("1"),return_buffer);
  photometer_callback(String("blank"),String(" "),return_buffer);
  photometer_callback(String("select"),String("0"),return_buffer);
  photometer_callback(String("blank"),String(" "),return_buffer);
  STATUS_GREEN
}

int8_t photometer_callback(String command, String argument, void* ret_buffer) {
  command.toLowerCase();
  if (command=="boot") {
    initialize_photometer();
    return 0;
  } else if (command=="select") {
    return select_bank(argument);
  } else if (command=="test") {
    get_reading(argument, (float*)ret_buffer);
    return 4;
  } else if (command=="blank") {
    blank_bank();
    return 0;
  } else {
    //unrecognized command, report an error
    return -1;
  }
}

void loop() {
  char inChar;
  while (Serial.available()) {
    inChar = Serial.read();
    if (!commandReceived) {
      if (inChar == '\n') argumentReceived = 1;
      else if (inChar != ' ') commandString+=inChar;
      else commandReceived = 1;
    } else {
      if (inChar != '\n') argumentString+=inChar;
      else argumentReceived = 1;
    }
    if (argumentReceived) {
      commandReceived = 0;
      argumentReceived = 0;
        int8_t res = photometer_callback(commandString, argumentString, return_buffer);
        if (res == -1)
          Serial.println("NACK");
        else if (res == 1)
          Serial.println((char*)return_buffer);
        else if (res == 4)
          Serial.println(*((float*)return_buffer));
        else if (res == 0)
          Serial.println("ACK");
      commandString = "";
      argumentString = "";
    }
  }
}
