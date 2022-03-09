#include <Wire.h>
#include "wiring_private.h"
#include "RTClib.h"
#include "Adafruit_MPRLS.h"
#include <rgb_lcd.h>
#include "HoneywellZephyrI2C.h"


#define RESET_PIN  -1  // set to any GPIO pin # to hard-reset on begin()
#define EOC_PIN    -1  // set to any GPIO pin to read end-of-conversion by pin
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

// FLOW RATE SENSOR SETUP
ZephyrFlowRateSensor sensor(0x49, 50, ZephyrFlowRateSensor::SCCM); // construct a 50 SCCM sensor with address 0x49

//// Real Time Clock
RTC_DS1307 rtc;

// LCD SETUP
rgb_lcd lcd;

const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

// PRESSURE SENSOR SETUP
float pressure = 0;
float KPSI = 0;
float startp = 0;
float startKPSI = 0;
float sensitivity;
float wait = 0;

// VENTILATOR MODE SETUP
bool modeAC;
bool modefull;
int prevAC = LOW;
int prevfull = LOW;

bool breath = false;

int AC;
int full;

// POTS SETUP
int potIE; // controls IE ratio, sets the E, I is always 1
int potVol; // sets target inhalation volume
int potSens; // doesn't seem to do much now
int potBPM; // controls breaths per minute


// IE RATIO CALCULATION
float In_t; // time set for the entire inhalation
int In_ratio;// ratio of airflowing vs holding within the entire inhalation time to achieve a specific tVol
float In = 1; // inhalation ratio value, currently set to 1 so that potIE modulates only Ex

float Ex_t; // exhalation time
float Ex; // exhalation ratio value

int Vol; // selected vol pot setting
int minVol; // minimum vol pot setting
int maxVol; // maximum vol pot setting


float BPM; // selected BPM pot setting
float normalize; //

// hard set variable based on flowrate meter on the wall. THIS ISN'T CURRENTLY USED

// VOLUME AND PUMP TIME CALCULATION
float flowrate; // flow detected by flowrate sensor
float v_calc; // calculated volume in mL
float v_set; // set volume in mL
float v_trans; // add to counter of calculated volume in mL
unsigned long previous; // start time for each interval while pumping in
unsigned long current; // end time for each interval while pumping in
unsigned long time_elapsed; // dt, length of time for interval
unsigned long time_breath; // initializing the breathing time variable

float breathlength_in; // time for breath in, basically In_t * 1000
float breathlength_hold; // time to hold in
float breathlength_out; // time for breath out, basically Ex_t * 1000

int multiplier;

// PINS
int pot1 = A0;
int pot2 = A1;
int pot3 = A2;
int pot4 = A3;

int red1 = 3;
int red2 = 4;
int red3 = 6;
int red4 = 5;

int green = 9;
int blue = 7;
int yell = 8;

int blueswitch = 11;
int yellswitch = 10;
int vacvalve = 13;
int airvalve = 12;

bool SystemRunning; // are there any modes selected?
bool Mode; // if true AC else Full

void setup() {
  Serial.begin(115200);

  //  //RTC SET UP
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }


  // FLOWRATE SENSOR SETUP
  Wire.begin(); // start 2-wire communication
  Wire.setClock(400000L); // set up fast-mode for flowrate sensor
  sensor.begin(); // initialize flowrate sensor

  // PRESSURE SENSOR SETUP
  Serial.println("MPRLS Simple Test"); // check if pressure sensor is working
  if (! mpr.begin()) {
    Serial.println("Failed to communicate with MPRLS sensor, check wiring?"); // can't find sensor
    while (1) {
      delay(10);
    }
  }
  Serial.println("Found MPRLS sensor"); // found sensor
  startp = mpr.readPressure(); // start pressure
  startKPSI = (startp / 6.89476); // convert from kPa to PSI

  // PIN SETUP
  pinMode(pot1, INPUT);
  pinMode(pot2, INPUT);
  pinMode(pot3, INPUT);
  pinMode(pot4, INPUT);
  pinMode(blueswitch, INPUT);
  pinMode(yellswitch, INPUT);
  pinMode(red1, OUTPUT);
  pinMode(red2, OUTPUT);
  pinMode(red3, OUTPUT);
  pinMode(red4, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(yell, OUTPUT);
  pinMode(vacvalve, OUTPUT);
  pinMode(airvalve, OUTPUT);

  // LCD SETUP
  lcd.begin(16, 2); // set up the LCD's number of columns and rows

  lcd.setRGB(123, 22, 112);

  lcd.print("Hello, I'm Eurus!"); // print a message to the LCD.

  delay(1000);
}

void loop() {
  //   //RTC CALL E.g.
  DateTime now = rtc.now();
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();


  // CHECK POTS
  potIE = analogRead(pot2);
  potVol = analogRead(pot1);
  potSens = analogRead(pot3);
  potBPM = analogRead(pot4);


  Serial.print("pot1 = ");
  Serial.println(potIE);
  Serial.print("pot2 = ");
  Serial.println(potVol);
  Serial.print("pot3 = ");
  Serial.println(potSens);
  Serial.print("pot4 = ");
  Serial.println(potBPM);

  screen();
  pressurefunction();
  flowratefunction();

  // CHECK MODE
  AC = digitalRead(blueswitch);
  full = digitalRead(yellswitch);

  if ( AC == HIGH || full == HIGH) {
    SystemRunning = true;
    if (AC == HIGH) {
      Mode = true;
    } else if (full == HIGH) {
      Mode = false;
    }
  } else {
    SystemRunning = false;
    Serial.println("off");
    digitalWrite(blue, LOW);
    digitalWrite(yell, LOW);
    digitalWrite(red1, LOW);
    digitalWrite(red2, LOW);
    digitalWrite(red3, LOW);
    digitalWrite(red4, LOW);
    digitalWrite(airvalve, LOW);
    digitalWrite(vacvalve, LOW);
  }

  if (SystemRunning == true) {

    // IE RATIO CALCULATION
    minVol = 0; // arbitrary minimum volume is 0 mL currently
    maxVol = 2000; // arbitrary maximum volume is 2000 mL currently
    Vol = map(potVol, 0, 1023, minVol, maxVol); // NOTE: minVol and maxVol have not been set yet
    BPM = map(potBPM, 0, 1023, 5, 30); // BPM scaled from 5 to 30
    Ex = map(potIE, 0, 1023, 1, 6); // IE ratio ranges from 1:1 to 1:6

    v_set = Vol; // set volume is just pot volume

    In_t = (60 / BPM) / (Ex + 1); //(60/ BPM) is time per breath in seconds / (Ex + In) is total ratio amount, output in seconds. ONLY works if In is 1.
    Ex_t = Ex * In_t; // exhalation time is the inverse IE ratio times inhalation time, output in seconds.

    breathlength_in = In_t * 1000; // convert In_t to milliseconds for arudino delay
    breathlength_out = Ex_t * 1000; // convert Ex_T to milliseconds for arduino delay
    //---------------------------------


    if (Mode == true) {
      Serial.println("AC");
      digitalWrite(blue, LOW);
      digitalWrite(yell, HIGH);

      //breathlength = map(potIE, 0, 1023, 0, 1000); // NO LONGER RELEVANT
      //wait = map(potSens, 0, 1023, 0, 2000); // NOTE: this doesn't seem necessary here

      //    sensitivity = map(potVol, 0, 1023, 2, 15);
      float sensitivity = (potVol - 0) * (15.00 - 2.00) / (1023.00 - 0.00) + 2.00;
      //  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

      Serial.println("Sense" + char (sensitivity));
      Serial.println("KPSI" + char (KPSI));
      Serial.println("Start KPSI" + char (startKPSI));


      if (KPSI < (startKPSI - sensitivity)) {
        breath = true; // reduced pressure indicates a breath
        Serial.println("breath is TRUE!");
      } else if (KPSI > startKPSI - sensitivity )  {
        breath = false; // NOTE: does this really need to be an else if and not just an else? If it does, then it should be a >=
        Serial.println("breath is false!");
      }

      breathe_AC(breath);
      delay(50);

    }  // AC MODE OUT

    else { // Full Mode
      //breathlength = map(potIE, 0, 1023, 0, 1000); // NO LONGER RELEVANT
      //wait = map(potSens, 0, 1023, 0, 2000); // NO LONGER RELEVANT
      //    if (sensor.readSensor() == 0) {
      //      normalize = sensor.flow() * -1;
      //    }
      // Mode Setting and LED color
      Serial.println("full");
      digitalWrite(blue, HIGH);
      digitalWrite(yell, LOW);

      // Inhale
      //    previous = millis (); // determine time at start of pumping air in
      digitalWrite(airvalve, HIGH);
      digitalWrite(vacvalve, LOW);
      digitalWrite(red1, HIGH);
      digitalWrite(red2, LOW);
      //delay(100);

      v_calc = 0; // start volume pumped in is 0
      time_breath = 0; // amount of time breathing is 0 so far
      multiplier = 3750; //3606
      int i = 0;
      while (v_calc <= v_set && i < 10) {
        if (sensor.readSensor() == 0) {
          flowrate = (sensor.flow() + normalize) / 60 * multiplier; // get flowrate from sensor and convert from mL/min to mL/s
          Serial.println(sensor.flow());
          Serial.println(flowrate);
          Serial.println(breathlength_in);
          Serial.println(breathlength_out);
        }
        //current = millis(); // get current time
        //time_elapsed = current - previous; // determine time in milliseconds from beginning to pump air to now
        if (flowrate < 0) {
          flowrate = 0;
        }
        v_trans = flowrate * 0.01; // determine volume pumped in this time, flow is in mL/s, time_elapsed is in ms, have to multiply by 0.01 to get s
        v_calc = v_calc + v_trans; // add to counter of total volume pumped
        //time_breath = time_breath + time_elapsed; // add to counter of how long it took to breathe in
        delay(10);
        time_breath = time_breath + 10;
        i++;
      }
      // Hold
      digitalWrite(airvalve, LOW);
      digitalWrite(vacvalve, LOW);
      digitalWrite(red1, LOW);
      digitalWrite(red2, LOW);
      breathlength_hold = breathlength_in - time_breath; // calculate how long left in inhalation to maintain proper IE ratio
      if (breathlength_hold < 0) {
        // if inhalation time too long to maintain IE ratio, remove hold time and provide warning.
        Serial.println("Not enough flow to reach set volume. Please increase airflow or decrease set volume.");
        breathlength_hold = 0;
      }
      delay(breathlength_hold);
      // Exhale
      Serial.println("Exhale");
      digitalWrite(airvalve, LOW);
      digitalWrite(vacvalve, HIGH);
      digitalWrite(red1, LOW);
      digitalWrite(red2, HIGH);
      delay(breathlength_out);

      Serial.println("B-OUT" +  char(breathlength_out));
      Serial.println("B-IN" + char(breathlength_in));
      Serial.println("B-OUT" + char(breathlength_hold));

    } //FULL MODE OUT

  }


}
