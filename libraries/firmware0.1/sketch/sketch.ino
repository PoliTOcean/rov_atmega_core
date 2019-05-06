
#include <Array.h>
#include <SPI.h>
#include "IMU.h"
#include "Sensor.h"
#include "MS5837.h"
#include "Motors.h"
#include "Commands.h"

#define SENSORS_SIZE static_cast<int>(sensor_t::Last)+1

volatile sensor_t s;  // sensor counter
volatile Array<Sensor<byte>, SENSORS_SIZE> sensors; // array of sensors

volatile bool process;
volatile byte c;
volatile bool nextIsButton = false;
volatile int receivedDataSelector = 0;

IMU imu;  // imu sensor

MS5837 brSensor;  // pressure sensor
Motors motors;  // motors manager


void setup() {
    Serial.begin(9600);                   // initialize comunication via the serial port
    imu.configure();                      // initialize IMU sensor
    brSensor.init();                      // initialize pressure sensor
    brSensor.setModel(MS5837::MS5837_30BA);
    brSensor.setFluidDensity(997);        // kg/m^3 (freshwater, 1029 for seawater)
    motors.configure(brSensor,imu);       // initialize motors
    delay(1000);                          // delay of 1 second to make actions complete
    pinMode(MISO, OUTPUT);                // SPI setup
    SPCR |= _BV(SPE);
    SPDR = 0xFF;                          // set the SPI data register to 0xFF before sending sensors data
    SPI.attachInterrupt();                // enable SPI
    
    for (auto sensor_type : sensor_t()) // create sensors array
        sensors.push_back(Sensor<byte>(sensor_type, 0));
    s = sensor_t::First;              // set the sensor counter
}

void loop() {
  // prepare data to send back via spi
  unsigned long now = micros();
  
  brSensor.read();
  imu.imuRead();
  imu.complementaryFilter();
  sensors[static_cast<int>(sensor_t::TEMPERATURE)].setValue(brSensor.temperature());
  sensors[static_cast<int>(sensor_t::PRESSURE)].setValue(brSensor.pressure());
  sensors[static_cast<int>(sensor_t::PITCH)].setValue(imu.pitch);
  sensors[static_cast<int>(sensor_t::ROLL)].setValue(imu.roll);
  motors.evaluateHorizontal();
  motors.evaluateVertical();
  
  now = micros()-now;
  Serial.println((float)now/1000);
}

ISR (SPI_STC_vect)
{
    c = SPDR;
    
    // Prepare the next sensor's value to send through SPI
    SPDR = sensors[static_cast<int>(s)].getValue();

    if(c == 0x00){
      //the next incoming data is a button
      nextIsButton=true;
      return;
    }

    if(nextIsButton){
      // process the nextIsButton
      switch(c){
        case MOTORS_ON:
          motors.start();
        break;
        case MOTORS_OFF:
          motors.stop();
        break;
        case VDOWN:
          motors.goDown();
        break;
        case VDOWN_STOP:
          motors.stopVertical();
        break;
        case VUP:
          motors.goUp();
        break;
        case VUP_STOP:
          motors.stopVertical();
        break;
        case FAST:
          motors.velocity = 3;
        break;
        case NORMAL:
          motors.velocity = 2;
        break;
        case SLOW:
          motors.velocity = 1;
        break;
       }
      nextIsButton = false; // last command
      receivedDataSelector = 0; // restart from x
    }else{
      switch(receivedDataSelector){
       case 0:         //  read x
        motors.x = c-127;
       break;
      
       case 1:        // read y
       motors.y = c-127;
       break;
      
       case 2:  //  read rz
       motors.rz = c-127;
       break;
      }
      
      if (++receivedDataSelector >= 3)
        receivedDataSelector = 0;
    }
      
    
  
    // if I sent the last sensor, reset current sensor to first one.
    if (++s > sensor_t::Last)
      s = sensor_t::First;

    process = true;
}