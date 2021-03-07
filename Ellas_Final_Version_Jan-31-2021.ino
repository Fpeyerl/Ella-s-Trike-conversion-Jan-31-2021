
/*** NEW DEFINES FOR HIGH/LOW SPEED AND STEERING LIMIT ***/

#define STEERING_OFFSET 300  //make this 0 if not offsets will be used
#define SLOWSPEED  1.5      //this value is the divisor for the calculation of slow speed  speed=speed/SLOWSPEED eq. is located in MainDriveActuator()

#define DEBUG 1
#define CALIBRATION 0

#define STMIN 300
#define STMAX 700
#define DRVMIN 700
#define BRMAX 400

int brMoveToLimit(int);            //this function is used to calibrate the brake min & max values
void brDriveActuator(int, int);    //this function is used to drive the brake actuator to the setpoint
int stMoveToLimit(int);            //this function is used to calibrate the steering min and max values
void stDriveActuator(int, int);    //this function is used to control the steering actuator to the setpoint

void MainDriveActuator(int, int); //this function is used to control main drive motor
void YjoystickCalibration(void);
void XjoystickCalibration(void);
void brakeCalibration(void);
void steeringCalibration(void);  //this function is used to calibrate the steering actuator min & max values

#define FORWARD 1
#define EXTEND 1
#define RETRACT -1
#define STOP 0
#define MARGIN 10 //allowable margin of err between actual position and setpoint


   //steering actuator variables, if doing a calibration stMinReading & stMaxReading will change, you can change these to what works without doing a calibration
int stMinReading = 49;       //actual maximum fb value 0-1023
int stMaxReading = 949;       //actual minimum fb value 0-1023

int stSpeed = 200;           //this is the steering actuator speed value, can be changed
int stSetpoint = 50;         //dummy  setpoint value for steering actuator

//speed button
const uint8_t slowSpeedBtn = 12;

//E-Brake
const uint8_t eBrake = 2;    //locks the brake, disengages drive

//brake actuator variables - uses DROK dual H-bridge driver board
const uint8_t brIN3 = 7;     //in3  for direction
const uint8_t brIN4 = 8;     //in4  for direction
const uint8_t brENB = 6;     //enable pin: pwm this pin to change speed

const uint8_t brSensorPin = A1;  //brake feedback
int brSpeed = 240;           //brake actuator default speed, can be changed
int brSetpoint = 50;         //dummy brake setpoint value
int brSensorVal = 0;         //brake fb value 0 - 1023


  //brake actuator variables, if doing a calibration brMinReading & brMaxReading will change, you can change these to what works without doing a calibration
int brMinReading = 70;        //actual min. feedback value from brake (retracted)
int brMaxReading = 925;       //actual max. feedback value from brake (extended)


//drive motor-use sngl H-bridge driver board
const uint8_t drvENA1 = 3;
const uint8_t drvIN1 = 4;
const uint8_t drvIN2 = 5;
int drvSetpoint = 0;

//uses DROK dual H-bridge driver board (for testing)
//const uint8_t stIN1 = 7;         //in1 for direction
//const uint8_t stIN2 = 8;         //in2 for direction
//const uint8_t stENA1 = 6;        //enable pin (pwm)

const uint8_t stIN1 = 10;         //in1 for direction
const uint8_t stIN2 = 11;         //in2 for direction
const uint8_t stENA1 = 9;         //enable pin (pwm)

const uint8_t stSensorPin = A0;   //brake feedback pin
int stSensorVal = 0;              //brake fb value 0 - 1023

const uint8_t y_JoyPin = A2;      //this is the drive/brake axis of the joystick (Y axis)
const uint8_t x_JoyPin = A3;      //this is the steering axis of the joystick    (X axis)
int y_JoyVal, x_JoyVal = 199;         //value of joystick values in counts (0-1023)

int y_JoyVal_max = 1023, y_JoyVal_min = 0, y_JoyVal_center = 504; //default calibration values of brake/drive joystick
int x_JoyVal_max = 1023, x_JoyVal_min = 0, x_JoyVal_center = 512; //default calibration values of steering joystick


long unsigned prev = 0, prev1 = 0;
int skip = 0;


//finds a number that is linear deposed between two other numbers based on a ratio
int interpolate(int value, int in_min, int in_max, int out_min, int out_max) {
  int y = value - in_min;
  int x = in_max - in_min;
  int z = out_max - out_min;

  long out = ((long)y * (long)z) / x + out_min;

  return out;
}

void setup() {

  Serial.begin(9600);
  while (!Serial);               //wait until serial port is ready

  pinMode(slowSpeedBtn, INPUT_PULLUP); //

  pinMode(eBrake, INPUT_PULLUP);  //engages brake fully, disengages drive

  pinMode(brIN3, OUTPUT);        //these two pins control direction of actuator
  pinMode(brIN4, OUTPUT);        //for the brake actuator
  pinMode(brENB, OUTPUT);
  pinMode(brSensorPin, INPUT);   //fb pin from brake actuator
  digitalWrite(brENB, LOW);      //initially turn off brake actuator

  pinMode(stIN1, OUTPUT);        //these two pins control direction of actuator
  pinMode(stIN2, OUTPUT);        //for the steering actuator
  pinMode(stENA1, OUTPUT);
  pinMode(stSensorPin, INPUT);   //fb pin from steering actuator
  digitalWrite(stENA1, LOW);     //initially turn off steering actuator

  pinMode(drvENA1, OUTPUT);
  pinMode(drvIN1, OUTPUT);
  pinMode(drvIN2, OUTPUT);
  digitalWrite(drvENA1, LOW);     //initially turn off steering actuator

  //perform all the calibrations
#if CALIBRATION
  steeringCalibration();
  brakeCalibration();
  YjoystickCalibration(); //for brake & main drive
  XjoystickCalibration(); //for steering
#endif

  prev = millis();          //intialize prev for the 2 second display of setpoint values

}

void loop() {

  // display values
#if DEBUG
  if ((millis() - prev) > 3000) { //every two seconds print out the Brake setpoint and its actual value

    //if (y_JoyVal < 505)
    //stSetpoint=interpolate(x_JoyVal, x_JoyVal_min, x_JoyVal_max, stMinReading, stMaxReading);//calc setpoint
    Serial.print("Steering_SP = "); Serial.println(stSetpoint);
    //stSensorVal=analogRead(stSensorPin);
    Serial.print("steering Value = "); Serial.println(stSensorVal);
    Serial.print("y_JoyVal value = "); Serial.println(y_JoyVal);
    Serial.print("brake value = ");   Serial.println(brSensorVal);
    //Serial.print("stMinReadinge = "); Serial.println(stMinReading);
    //Serial.print("stMaxReading = "); Serial.println(stMaxReading);
    Serial.println();

    prev = millis();
  }
#endif

  //if eBrake is on stay in this loop-
  while (digitalRead(eBrake) == LOW) {
    drvSetpoint = 0;
    MainDriveActuator(FORWARD, drvSetpoint);  //disengage drive

    brSetpoint =  brMaxReading;
    brDriveActuator(RETRACT, brSpeed);   //apply brake
  }



  y_JoyVal = analogRead(y_JoyPin);     //y axis joystick controls the brake (joystick position 0-1023  ) & the main drive

  //BRAKE AND MAIN DRIVE CODE


  if (y_JoyVal < BRMAX)
    brSetpoint = interpolate(y_JoyVal, y_JoyVal_min, BRMAX, brMinReading, brMaxReading);

  else
    brSetpoint =  brMaxReading;


  brSensorVal = analogRead(brSensorPin);   //read brake feedback Sensor

  if ( (abs(brSetpoint - brSensorVal)) < MARGIN ) {
    brDriveActuator(STOP, brSpeed);
  }
  else if (brSetpoint < brSensorVal) { //retract brake actuator
    brDriveActuator(RETRACT, brSpeed);
  }
  else if (brSetpoint > brSensorVal) {  //extend brake actuator
    brDriveActuator(EXTEND, brSpeed);
  }



  //APPLY MOTOR DRIVE
  if (y_JoyVal > DRVMIN) {

    brDriveActuator(EXTEND, 255); //brMoveToLimit(EXTEND);  //turn brake off

    while ( (brSensorVal = analogRead(brSensorPin)) < 900); //while brake is engaged do not energize drive motor

    drvSetpoint = interpolate(y_JoyVal, DRVMIN, x_JoyVal_max, 0, 255);

  }
  else {
    drvSetpoint = 0 ;
  }

  MainDriveActuator(FORWARD, drvSetpoint);

  //STEERING CODE
  x_JoyVal = analogRead(x_JoyPin);     //joystick position 0-1023

  /*
       test code
    if((millis() - prev) > 3000){
        if (x_JoyVal ==199)
                x_JoyVal= 800;
        else if (x_JoyVal == 800)
                x_JoyVal = 199;
        prev=millis();
    }
  */

  //turn right
  if (x_JoyVal < STMIN)
    stSetpoint = interpolate(x_JoyVal, x_JoyVal_min, STMIN, stMinReading, (stMinReading + stMaxReading) / 2);

  //turn left
  else if (x_JoyVal > STMAX)
    stSetpoint = interpolate(x_JoyVal, STMAX, x_JoyVal_max, (stMinReading + stMaxReading) / 2, stMaxReading);

  else
    stSetpoint = (stMinReading + stMaxReading) / 2 ;

  stSensorVal = analogRead(stSensorPin);  //read brSensor;

  if ( (abs(stSetpoint - stSensorVal)) < MARGIN ) {
    stDriveActuator(STOP, stSpeed);
  }
  else if (stSetpoint < stSensorVal) {
    stDriveActuator(RETRACT, stSpeed);
  }
  else if (stSetpoint > stSensorVal) {
    stDriveActuator(EXTEND, stSpeed);
  }


}//loop

//controls the main drive motor.
//two states: forward ,  off
void MainDriveActuator(int Direction, int Speed) {  //this is for the steering acuator only


  if (digitalRead(slowSpeedBtn) == true)          //this is where the speed change is made if in slow speed mode
    Speed = float(Speed / SLOWSPEED);


  switch (Direction) {

    case 1:       //forward

      digitalWrite(drvIN2, 1);
      digitalWrite(drvIN1, 0);
      analogWrite(drvENA1, Speed);
      break;

    case 0:       //extend
      digitalWrite(drvIN2, 0);
      digitalWrite(drvIN1, 0);
      analogWrite(drvENA1, 0);
      break;

  }
}


//drives the steering actuator to the setpoint value
void stDriveActuator(int Direction, int Speed) {  //this is for the steering acuator only
  switch (Direction) {
    case 1:       //extension

      digitalWrite(stIN2, 1);
      digitalWrite(stIN1, 0);
      analogWrite(stENA1, Speed);
      break;

    case 0:       //stopping
      digitalWrite(stIN2, 0);
      digitalWrite(stIN1, 0);
      analogWrite(stENA1,  0);
      break;

    case -1:      //retraction
      digitalWrite(stIN2, 0);
      digitalWrite(stIN1, 1);
      analogWrite(stENA1, Speed);
      break;
  }
}

//this function sends proper signal to the driver board in order to reach the setpoint
void brDriveActuator(int Direction, int Speed) {
  switch (Direction) {
    case 1:       //extension

      digitalWrite(brIN4, 1);
      digitalWrite(brIN3, 0);
      analogWrite(brENB, Speed);
      break;

    case 0:       //stopping
      digitalWrite(brIN4, 0);
      digitalWrite(brIN3, 0);
      analogWrite(brENB,  0);
      break;

    case -1:      //retraction
      digitalWrite(brIN4, 0);
      digitalWrite(brIN3, 1);
      analogWrite(brENB, Speed);
      break;
  }
}


// finds the limit of the steering actuator feedback signal
// inputs: Direction values: EXTEND -1   RETRACT  1   */
int stMoveToLimit(int Direction) {

  int prevReading = 0;
  int currReading = 0;

  do {
    prevReading = currReading;
    stDriveActuator(Direction, 200);
    delay(100);                             //may have to change this value
    currReading = analogRead(stSensorPin);

    //if extening and limiting travel, then we want to end this routine when we are at 1023 -counts; counts is the user controlled variable
    if (!RETRACT)
      if (currReading >= 1023 - STEERING_OFFSET)
        break;
    //if retracting and limiting travel, then we want to end this routine when we are at 0 + counts; counts is the user controlled variable
    if (RETRACT)
      if (currReading <= STEERING_OFFSET)
        break;


  } while (prevReading != currReading);

  return currReading;
}


// finds the limit of the actuator feedback signal
// inputs: Direction values: EXTEND -1   RETRACT  1 */
int brMoveToLimit(int Direction) {

  int prevReading = 0;
  int currReading = 0;

  do {
    prevReading = currReading;
    brDriveActuator(Direction, 200);
    delay(200);
    currReading = analogRead(brSensorPin);
  } while (prevReading != currReading);

  return currReading;
}


// brake calibration function
void brakeCalibration() {

  Serial.print("Extending Brake Actuator to maximum ... ");
  delay(1000);
  brMoveToLimit(EXTEND);          //get brake drive actuator extend limit
  delay(5000);                    //give it time to get there
  brMaxReading = analogRead(brSensorPin);
  Serial.print("brake Max Reading is "); Serial.println(brMaxReading);
  delay(2000);

  Serial.print("Retracting Brake Actuator to minimum ... ");
  delay(1000);
  brMoveToLimit(RETRACT);
  delay(4000);
  brMinReading = analogRead(brSensorPin); //get brake drive actuator retract limit
  Serial.print("brake Min Reading is "); Serial.println(brMinReading);
  delay(2000);

  Serial.println("Extending Actuator to maximum (brake off)");
  delay(1000);
  brMoveToLimit(EXTEND);          //get brake drive actuator extend limit
  delay(8000);                    //give it time to get there
}


//steering calibration function
void steeringCalibration() {
  Serial.print("Extending Actuator to maximum ... ");
  delay(1000);
  stMoveToLimit(EXTEND);  //get brake drive actuator extend limit
  delay(4000);            //give it time to get there
  stMaxReading = analogRead(stSensorPin);
  Serial.print("stMaxReading is "); Serial.println(stMaxReading);

  Serial.print("Retracting Actuator to minimum ... ");
  delay(1000);
  stMoveToLimit(RETRACT);
  delay(4000);
  stMinReading = analogRead(stSensorPin); //get brake drive actuator retract limit
  Serial.print("stMinReading is "); Serial.println(stMinReading);
}

//joystick calibration routine Y AXIS (brake and drive motor)
void YjoystickCalibration(void) {

  int y_JoyVal_maxA = 0, y_JoyVal_minA = 1000;
  Serial.println("Lets Test brake/drive joystick");
  //delay(8000);
  Serial.println("Move joystick all the way forward");
  delay(5000);
  y_JoyVal_max = analogRead(y_JoyPin);
  Serial.print("Joystick max = "); Serial.println(y_JoyVal_max);
  Serial.println();
  Serial.println("Move joystick all the way back ");
  delay(5000);
  y_JoyVal_min = analogRead(y_JoyPin);     //joystick position 0-1023
  Serial.print("Joystick min = "); Serial.println(y_JoyVal_min);

  Serial.println();
  Serial.println("Release joystick (center joystick)");
  delay(3000);
  uint32_t prev = millis();

  //this loop tests the stability of the joystick by taking readings while the joy stick is centered
  //and not moving. The test lasts 3 seconds. We expect the counts to be close to the same from max to min
  while (millis() - prev < 3000) {  //for 3 seconds

    y_JoyVal_center = analogRead(y_JoyPin);     //read the Y axis position of the joystick
    delay(50);

    if (y_JoyVal_center < y_JoyVal_minA) {     //if joy < min value store new min value
      y_JoyVal_minA = y_JoyVal_center;
    }
    else if (y_JoyVal_center > y_JoyVal_maxA)  //if joy > max value store new max value
      y_JoyVal_maxA = y_JoyVal_center;
  }//while

  y_JoyVal_center = (y_JoyVal_maxA + y_JoyVal_minA) / 2; //this sets the joystick center position in counts which is later mapped to actuator max counts (brake off)

  Serial.println("The Counts below should about the same if not then something is not right with the joystick");
  Serial.print("Joystick min = "); Serial.println(y_JoyVal_minA);
  Serial.print("Joystick max = "); Serial.println(y_JoyVal_maxA);

  //lets display an error msg if the joystick is not stable
  if ( (abs(y_JoyVal_maxA - y_JoyVal_minA)) > 5)
    Serial.println("Error: Joystick is not stable");

}

//joystick calibration routine X AXIS (for steering)
void XjoystickCalibration(void) {
  int x_JoyVal_maxA = 0, x_JoyVal_minA = 1000;
  Serial.println("Lets Test steering joystick");
  //delay(8000);
  Serial.println("Move joystick all the way left");
  delay(5000);
  x_JoyVal_max = analogRead(x_JoyPin);
  Serial.print("Joystick max = "); Serial.println(x_JoyVal_max);
  Serial.println();
  Serial.println("Move joystick all the way right ");
  delay(5000);
  x_JoyVal_min = analogRead(x_JoyPin);     //joystick position 0-1023
  Serial.print("Joystick min = "); Serial.println(x_JoyVal_min);

  Serial.println();
  Serial.println("Release joystick (center joystick)");
  delay(3000);
  uint32_t prev = millis();

  //this loop tests the stability of the joystick by taking readings while the joy stick is centered
  //and not moving. The test lasts 3 seconds. We expect the counts to be close to the same from max to min
  while (millis() - prev < 2000) {  //for 3 seconds

    x_JoyVal_center = analogRead(x_JoyPin);     //read the Y axis position of the joystick
    delay(50);

    if (x_JoyVal_center < x_JoyVal_minA) {     //if joy < min value store new min value
      x_JoyVal_minA = x_JoyVal_center;
    }
    else if (x_JoyVal_center > x_JoyVal_maxA)  //if joy > max value store new max value
      x_JoyVal_maxA = x_JoyVal_center;
  }//while

  x_JoyVal_center = (x_JoyVal_maxA + x_JoyVal_minA) / 2; //this sets the joystick center position in counts which is later mapped to actuator max counts (brake off)

  Serial.println("The Counts below should about the same if not then something is not right with the joystick");
  Serial.print("Joystick min = "); Serial.println(x_JoyVal_minA);
  Serial.print("Joystick max = "); Serial.println(x_JoyVal_maxA);

  //lets display an error msg if the joystick is not stable
  if ( (abs(x_JoyVal_maxA - x_JoyVal_minA)) > 5)
    Serial.println("Error: Joystick is not stable");

}
