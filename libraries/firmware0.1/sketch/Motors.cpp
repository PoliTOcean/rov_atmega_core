#include "Arduino.h"
#include "Motors.h"

#define UR_pin  4
#define UL_pin  5
#define UB_pin  6
#define FR_pin  11
#define FL_pin  12
#define BR_pin  13
#define BL_pin  14

#define kAng 0.5
#define V_MUL 2
#define H_MUL 2
#define kDep 0.5


void Motors::configure(MS5837 psensor, IMU imu){
    cli();                                        //stop interrupts
    // attach motors
    UR.attach(UR_pin);                              
    UL.attach(UL_pin);                              
    UB.attach(UB_pin);                              
    FR.attach(FR_pin);                              
    FL.attach(FL_pin);                             
    BR.attach(BR_pin);                              
    BL.attach(BL_pin);                              
    sei();
    Motors::stop();  // do not run the motors untill `start()` is called
    brSensor = psensor; // catch the pressure sensor object
    imuSensor = imu; // catch the imu sensor object
    savePressure = false;
    reqPress = brSensor.pressure();
}
void Motors::control(){
//    if(start){                            //if the ROV is started
//      #ifdef TEST                                     //if we are in water
//          evaluateVertical(K_ANG, K_DEP, vertical);   //then evaluate vertical values for autostabilization and autoquote
//      #else                                           //else, if we aren't,
//          evaluateVertical(0, 0, vertical);           //then evaluate them just for joystick up/down
//      #endif
//          
//          evaluateHorizontal(&valLF, &valRF, &valLB, &valRB);           //evaluate values for horizontal movement
//
//    //set new motors powers
//    //setServosValues(valLF, valRF, valLB, valRB, vertical[0], vertical[1], vertical[2], vertical[3], MAX_SRV);
//  }else                                 //else, if the ROV is stopped,
//    //setServosValues(0, 0, 0, 0, 0, 0, 0, 0, 0);       //then send STOP signal to motors
}

//function for pitch power calculation
float Motors::calcPitchPower(){
  /* it takes the difference between current pitch and the requested one from the joystick
   * and multiplicates it for a multiplication constant, passed as parameter */
  return 0;//kAng*imuSensor.getPitch(); //(the angle is the orizontal due to the sensor inclination)
  // TO DO controllare per MAX IMU
}

//function for roll power calculation. Same as above, without sign inversion
float Motors::calcRollPower(){
  return 0;//kAng*imuSensor.getRoll(); //(the angle is the orizontal due to the sensor inclination)
  // TO DO controllare per MAX IMU
}

//function to evaluate vertical motors values
void Motors::evaluateVertical(){

   float pitchPower, rollPower;
   if(!started){
     UR.set_value(0);
     UL.set_value(0);
     UB.set_value(0);
   }

   //call above functions for calculations
   pitchPower = calcPitchPower();
   rollPower = calcRollPower();
   
   //value for up-down movement
   int valUD=0;            //reset valUD
   if(down || up){         //controlled up-down from joystick
     savePressure = true;                           //it has to save pressure when finished
     valUD = (up-down)*(V_MUL/* TO DO +fastV*FAST_V*/); //fixed value depending on buttons pressed
   }else if(savePressure){
     reqPress = brSensor.pressure();
     savePressure = false;
   } //else, if it is not (still) pressing up/down buttons
   
   if(!savePressure) //change value for autoquote
     valUD = (reqPress-brSensor.pressure())*kDep;
 
   //adding values for UD movement/autoquote
   UL.set_value(valUD - pitchPower - rollPower);
   UR.set_value(valUD - pitchPower + rollPower);
   UB.set_value(valUD + 2*pitchPower);
}

/* function to evaluate powers for horizontal movement.*/
// TODO we have to pass from the SPI x y and rz and set the motors values
void Motors::evaluateHorizontal() {
  if(!started){
    FL.set_value(0);
    FR.set_value(0);
    BL.set_value(0);
    BR.set_value(0);
    return;
  }
  // I puntatori si riferiscono ai motori
  FL.set_value(H_MUL * signFL * (-y+x+rz));
  FR.set_value(H_MUL * signFR * (-y-x-rz));
  BL.set_value(H_MUL * signBL * (-y-x+rz));
  BR.set_value(H_MUL * signBR * (-y+x-rz));
}

void Motors::start(){
  started = true;
}
void Motors::stop(){
  x = 0;
  y = 0;
  rz = 0;
  started = false;
}
void Motors::stopVertical(){
  up = 0;
  down = 0;
}
void Motors::goUp(){
  up = 1;
}
void Motors::goDown(){
  down = 1;
}