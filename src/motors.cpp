#include <Arduino.h>
#include "common_types.h"

extern void zeroLinearRailEncoder();

void setMotorTorque(computer_commands_t commands);

void motorSetup(){
    pinMode(LINEAR_MOTOR_PWM, OUTPUT);
    pinMode(ELBOW_MOTOR_PWM, OUTPUT);
    pinMode(LINEAR_MOTOR_ENABLE, OUTPUT);
    pinMode(ELBOW_MOTOR_ENABLE, OUTPUT);

    pinMode(ENDSTOP, INPUT_PULLUP);
}

void enableMotors(){
    digitalWrite(LINEAR_MOTOR_ENABLE, HIGH);
    digitalWrite(ELBOW_MOTOR_ENABLE, HIGH);
}

void disableMotors(){
    digitalWrite(LINEAR_MOTOR_ENABLE, LOW);
    digitalWrite(ELBOW_MOTOR_ENABLE, LOW);
}

bool readEndstop(){
  return !digitalRead(ENDSTOP);
}

void homingRoutine(){
    computer_commands_t c;
    c.elbowTorque = 0;

    // Fast home
    while(readEndstop() == false){
        Serial.println("looking for home position");
        c.linearTorque = -HOMING_INITIAL_TORQUE;
        setMotorTorque(c);
        #ifdef VESC_DEBUG
            Serial.print("VESC: ");
            Serial.println(Serial1.readStringUntil('\n'));
        #endif
    }
    Serial.println("found home position");
    c.linearTorque = 0;
    setMotorTorque(c);
    delay(100);

    // backoff
    c.linearTorque = HOMING_INITIAL_TORQUE;
    Serial.println("Backing off");
    setMotorTorque(c);
    delay(500);
    c.linearTorque = 0;
    setMotorTorque(c);
    delay(100);

    // slow home
    while(readEndstop() == false){
        Serial.println("refining home position");
        c.linearTorque = -HOMING_SLOW_TORQUE;
        setMotorTorque(c);
        #ifdef VESC_DEBUG
            Serial.print("VESC: ");
            Serial.println(Serial1.readStringUntil('\n'));
        #endif
    }
    c.linearTorque = 0;
    setMotorTorque(c);
    delay(100);

    zeroLinearRailEncoder();

    // final backoff to center rail
    c.linearTorque = HOMING_INITIAL_TORQUE;
    setMotorTorque(c);
    delay(2000);

    c.linearTorque = 0;
    setMotorTorque(c);
    delay(100);
}

void setMotorTorque(computer_commands_t commands){

    // do a manual map. MAP() arduino built in function is messed up
    float linear_voltage = 255 * ((commands.linearTorque - MIN_TORQUE) / (MAX_TORQUE - MIN_TORQUE));
    float elbow_voltage = 255 * ((commands.elbowTorque - MIN_TORQUE) / (MAX_TORQUE - MIN_TORQUE));

    Serial.print("linear_voltage: ");
    Serial.println(linear_voltage);

    dacWrite(LINEAR_MOTOR_PWM, (int) linear_voltage);
    dacWrite(ELBOW_MOTOR_PWM, (int) elbow_voltage);
}

computer_commands_t safety_check(computer_commands_t commands, encoderPositions_t positions){
    // TODO: State machine integration

    if (
        positions.linearRailPosition > MAX_LINEAR_POSITION || 
        positions.linearRailPosition < MIN_LINEAR_POSITION ||
        positions.elbowPosition > MAX_ANGLE_ELBOW ||
        positions.elbowPosition < MIN_ANGLE_ELBOW ||
        positions.shoulderPosition > MAX_ANGLE_SHOULDER ||
        positions.shoulderPosition < MIN_ANGLE_SHOULDER
        ){
            commands.linearTorque = 0;
            commands.elbowTorque = 0;
    }

    return commands;
}