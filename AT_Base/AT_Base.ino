#include <SoftwareSerial.h>
#include <MPU6050_light.h>
#include "Wire.h"


//communication pins
#define rxPin 8
#define txPin 9

//push button pin
#define buttonPin 12

//left motor pins
#define enA 11
#define in1 7
#define in2 6
//right motor pins
#define enB 10
#define in3 5
#define in4 4

// Set up a new SoftwareSerial object for bluetooth
SoftwareSerial mySerial(8,9);

MPU6050 mpu(Wire);

unsigned long timer = 0;

// states the tractor could be in
enum {OFF, MOVE, TURN_L, TURN_R, TURN180};
unsigned char currentState;  // tractor state at any given moment

// states the button can be in
enum {PUSHED, RELEASED};

unsigned char buttonState; // button state at any given moment
bool buttonCommand; // boolean conversion from button input
int buttonRead; // command boolean used for directing tractor FSM logic
unsigned long debounceDelay = 50;
unsigned long debounceTime = 0;


// bluetooth char
String cmd;
// gyroscope float
float z;
float z_init;

void setup() {
  // Begins serial communication
  Serial.begin(9600);
  // Defines button pin as input
  pinMode(buttonPin, INPUT);
  pinMode(rxPin,INPUT);
  pinMode(txPin,OUTPUT);
  //left motor
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  //init direction for left motor
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  //right motor
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  //init direction for right motor
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  
  //gyro setup
  Wire.begin();
  mpu.begin();
  mpu.calcGyroOffsets();
  mpu.setFilterGyroCoef(0.98);
  mpu.calcOffsets();
  mySerial.begin(9600);
  z_init = -(mpu.getAngleZ());

  //init state
  currentState = OFF;
  buttonState = RELEASED;
  buttonCommand = false;

}

void loop() {
  mpu.update();
  float x = mpu.getAngleX();
  float y = mpu.getAngleY();
  float z = -(mpu.getAngleZ());
  if (mySerial.available()) {
    cmd = mySerial.readString();
    mySerial.print("Command: ");
    mySerial.println(cmd);
    
    if (cmd == "off") {
      mySerial.println("Turning off Robot");
    }
    else if (cmd == "on") {
      mySerial.println("Turning on Robot");
    }
    else if (cmd == "left") {
      mySerial.println("Turning left");
    }
    else if (cmd == "right") {
      mySerial.println("Turning right");
    }
    else {
      mySerial.println("Invalid command");
    }
    
  }
  mpu.update();
  //debounce button
  switch (buttonState) {
    case PUSHED:
      buttonRead = digitalRead(buttonPin);
      if (!buttonRead){
        buttonState=RELEASED;
        debounceTime = millis();
      }   
      break;
    case RELEASED:
      buttonRead = digitalRead(buttonPin);
      if (buttonRead){
        buttonState = PUSHED;
        buttonCommand = true; 
      }
      break;
  }
  mpu.update();

  switch (currentState) {
    case OFF: // Nothing happening, waiting for switchInput
      analogWrite(enA, 0);
      analogWrite(enB, 0);
      mpu.update();
      Serial.println(z);
      if (cmd == "on" || buttonCommand) {
        buttonCommand = false;
        timer = millis();
        currentState = MOVE;

        break;
      }
      else if (cmd == "left"){
        currentState = TURN_L;
        mpu.update();
        break;
      }
      else if (cmd == "right"){
        currentState = TURN_R;
        mpu.update();
        break;
      }
      break;

    case MOVE:
      if ((millis()-timer)>3000){
        currentState = OFF;
        mpu.update();
        break;
      }
      if (cmd == "off" || buttonCommand) {
        buttonCommand = false;
        currentState = OFF;
        timer = millis();
        mpu.update();
        break;
      }
      else if (cmd == "left"){
        currentState = TURN_L;
        mpu.update();
        break;
      }
      else if (cmd == "right"){
        currentState = TURN_R;
        mpu.update();
        break;
      }
      Serial.println("Moving");   
      analogWrite(enA, 120);
      analogWrite(enB, 120); 
                                                   
      if (z > z_init+5 && z < z_init+45) {
        updateZ();
        analogWrite(enA, 110);//110
        analogWrite(enB, 140);//140
      }
      else if (z > z_init+5 && z > z_init+45){
        updateZ();
        analogWrite(enA, 70);//70
        analogWrite(enB, 120);//120    
      }
      if (z < z_init-5 && z > z_init-45) {
        updateZ();
        analogWrite(enA, 140);
        analogWrite(enB, 110);
      }
      else if (z < z_init-5 && z < z_init-45){
        updateZ();
        analogWrite(enA, 120);
        analogWrite(enB, 70);        
      }
      break;      

    case TURN_R:  
      updateZ();
      //direction for left motor
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      //direction for right motor
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      if (z < z_init+90){
        updateZ();
        mySerial.print("z_init: ");
        mySerial.println(z_init);
        mySerial.print("z: ");
        mySerial.println(z);
        // Equal speeds in opposite directions
        analogWrite(enA,60);
        analogWrite(enB,60);
      }
      if (z < z_init+92 && z > z_init+88){
        updateZ();
        analogWrite(enA,0);
        analogWrite(enB,0);
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        digitalWrite(in3, LOW);
        digitalWrite(in4, HIGH);
        reinitialize();
        currentState = OFF;
        break;
      }
      
      // Resets the motor direction to initial values and establishes a new inital Z
      // Switches to MOVE state so robot continues along path
      break;

      
    case TURN_L: 
      updateZ();
      //direction for left motor
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      //direction for right motor
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      if (z > z_init-90){
        mySerial.print("z_init: ");
        mySerial.println(z_init);
        mySerial.print("z: ");
        mySerial.println(z);
        // Equal speeds in opposite directions
        analogWrite(enA,0);
        analogWrite(enB,80);
      }
      if (z > z_init-90 && z < z_init-75){
        analogWrite(enA,0);
        analogWrite(enB,0);
        currentState = OFF;
        z_init = z_init-90;
        reinitialize();
        break;
      }
      // Resets the motor direction to initial values and establishes a new inital Z
      // Switches to MOVE state so robot continues along path
      break;
    case TURN180:
      break;
    
    default:
      Serial.println("\n We hit the default");
      if (mySerial.available() > 0) {
        Serial.write(mySerial.read());
      }
      break;
  }
  // Clears the command
  cmd = "";
}

void updateZ(){
  mpu.update();
  z = -(mpu.getAngleZ());
}

void reinitialize() {
  //init direction for left motor
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  //init direction for right motor
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  
  //gyro reset
  Wire.begin();
  mpu.begin();
  mpu.begin();
  mpu.calcGyroOffsets();
  z_init = -(mpu.getAngleZ());
}
