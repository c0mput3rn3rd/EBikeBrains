// September 2017
// Electric Bike controller program
// Drives ESC to controll brushless motor
// With current and voltage sensor to manage power regulation to motor to prevent stalling
// Authors: Jason Armitage

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Addr, En, Rw, Rs, d4, d5, d6, d7, backlighpin, polarity

//Include Servo Library to run the ESC
#include <Servo.h>
// change REFRESH_INTERVAL in servo.h to modify default 50Hz signal

int throttlePin = A3;  // read voltage from throttle
int voltagePin = A2;  // read voltage from power module
int currentPin = A1;  // read current from power module
int throttle=40;
int escPin = 11;
int value = 0;
int motorSpeedPcnt = 0;
int servoVal;
double wheelCirc = 2.19; //  wheel circumference in meters
double bikeSpeed = 0.00;
int rampSpeed = 200;  // time in milliseconds to delay ramping of acceleration

int lastThrottle = 480;  // track the current throttle level. 174 is the no throttle val
int maxAmps = 20;  // motor cannot draw more than this
int peakAmps = 40;  // peak current the hall sensor can read
int lastAmps = 0;  
int highestAmps = 0;  // debug var to measure stall amps

double batteryVolts = 3.7;
int v=0;  // for testing
int i=0;

Servo ESC;

//####################################################################
int motorAmps() {
   // returns the number of amps the motor is drawing
   int amps;
   int sensorCurrent = 0;
   sensorCurrent = analogRead(currentPin);  // read current from sensor
//sensorCurrent = int(random(516,635));  // for debugging - replace with line above
   if (sensorCurrent > highestAmps) highestAmps = sensorCurrent;
   amps = map(sensorCurrent, 516, 778, 0, peakAmps);  // 778 is equiv to 40A

   lcd.setCursor(12, 1);
   lcd.print(map(highestAmps, 516, 1023, 0, 75));
   lcd.print("A ");
     
   lcd.setCursor(2, 1);
   lcd.print(amps);
   lcd.print("A  ");
   
   return amps;
}
//-------------------------------------------------------------
int batteryVoltage() {
   // returns the number of amps motor is drawing
   //int ratio = 10/110;
   batteryVolts  = analogRead(voltagePin);  // read voltage from sensor
   batteryVolts = map(batteryVolts, 0, 780, 0, 42);
   lcd.setCursor(11, 0);
   lcd.print(batteryVolts);
   lcd.print("V ");
   return batteryVolts;
}
//-------------------------------------------------------------
void setMotorSpeed(int throttleVal, int amps) {
    // based on the throttle value, set's the speed of the motor
    // provide throttle level and last amp reading
    // need the amp reading so we can record it along with throttle level
    int throttleLevel = 0;
    lastThrottle = throttleVal;  // set the last throttle position
    lastAmps = amps; // set the latest amp reading
    throttleLevel = map(throttleVal, 174, 870, 40, 150);
    ESC.write(throttleLevel);
    motorSpeedPcnt = map(throttleLevel, 40, 150, 0, 100);
    lcd.setCursor(4, 0);
    lcd.print(motorSpeedPcnt);
    lcd.print("% ");
}

//--------------------------------------------------------------
void accelerate(int targetThrottle) {
    // accelerate the bike to desired level
    // managing power as it ramps to prevent motor stall
    int level; // increasing throttle level as we accelerate
    int amps;
    int forwardAmp;
    int ampDiff = 0;
    int newThrottle;  // read new throttle level in case it changes
    level = lastThrottle;  // current throttle setting
    // slowly increase the power until we reach the desired level
    while (level < targetThrottle) {
         // get the amps flowing to motor
         amps = motorAmps();
         // compare the previous amp level to now
         ampDiff = abs(amps-lastAmps);
         // so estimate the value in the next increase
         forwardAmp = amps+(ampDiff*4); // look at level after next 4 periods - look further ahead
         // if the current draw is approaching maximum then back-off the throttle
         
         if (forwardAmp > (1.0*maxAmps)) {
            //slow down the acceleration
            // do this by stopping the acceleration and delay for a short while
            targetThrottle = level;
            delay(100);
            // change level very slightly
            level++;
            // after exiting this routine, if the throttle is still higher, this finction will
            // be called again and assess the amp accel rate which should have slowed after the delay
         } else {
            // increase level a bit more and send to motor
            level=level+10;  // slowly ramp up power to motor
         }
         // set motor Speed
         setMotorSpeed(level, amps);
    }
}

//--------------------------------------------------------------
float calcBikeSpeed(int pwm_value) {
     // determine the actual ground speed of the bike
     float wheelSpeed;
     wheelSpeed = wheelCirc * pwm_value * 60 * 3.6 ;  // speed in km/h
     return wheelSpeed;
}

//##################################################################
void setup() {
    Serial.begin(9600);  
    // attach ESC output
    ESC.attach(escPin);
    // mainESC.attach(escPin, 900, 2100); - sets min, max pulse width in microseconds
 
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Pwr=");
    lcd.setCursor(0, 1);
    lcd.print("I=");
    lcd.setCursor(9, 0);
    lcd.print("V=");
    lcd.setCursor(7, 1);
    lcd.print("Imax=");
    v = batteryVoltage();  // move this - call occasionally - wont change often
    setMotorSpeed(174,0);  // give ESC a low-power signal
}
//#################################################################
void loop(){
    throttle = analogRead(throttlePin);
    i = motorAmps();
    if (throttle > (5+lastThrottle)) {  // built in hysteresis
      accelerate(throttle);
    } else if (throttle < lastThrottle) {
      // not accelerating - set speed directly
      setMotorSpeed(throttle,i);
      lastAmps = 0;  // reset amp monitor
    }
}
