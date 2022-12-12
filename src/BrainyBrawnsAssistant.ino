#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

/* This driver uses the Adafruit unified sensor library (Adafruit_Sensor),
   which provides a common 'type' for sensor data and some helper functions.

   To use this driver you will also need to download the Adafruit_Sensor
   library and include it in your libraries folder.

   You should also assign a unique ID to this sensor for use with
   the Adafruit Sensor API so that you can identify this particular
   sensor in any data logs, etc.  To assign a unique ID, simply
   provide an appropriate value in the constructor below (12345
   is used by default in this example).

   Connections
   ===========
   Connect SCL to analog 5
   Connect SDA to analog 4
   Connect VDD to 3.3-5V DC
   Connect GROUND to common ground
*/

uint16_t BNO_SAMPLE_RATE_DELAY_MS = 5; // delay between fresh samples

// Check I2C device address and correct line below (by default address is 0x29 or 0x28)
//                                    id, address
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

class AccelerometerEventBuffer {
  int bufferSize;
  sensors_event_t events[];

  AccelerometerEventBuffer(int bufferSize) {
    this.bufferSize = bufferSize;
    this.events = new sensors_event_t[bufferSize];
  }

  void addEvent(sensors_event_t *event) {
    for (int i = 0; i < this.bufferSize-1; i++) {
      this.events[i] = this.events[i+1];
    }
    this.events[this.bufferSize-1] = &event;
  }

  double getAverageX() {
    double sum = 0;
    for (int i = 0; i < this.bufferSize; i++) {
      sum += this.events[i]->acceleration.x;
    }
    return sum / this.bufferSize;
  }

  double getAverageY() {
    double sum = 0;
    for (int i = 0; i < this.bufferSize; i++) {
      sum += this.events[i]->acceleration.y;
    }
    return sum / this.bufferSize;
  }

  double getAverageZ() {
    double sum = 0;
    for (int i = 0; i < this.bufferSize; i++) {
      sum += this.events[i]->acceleration.z;
    }
    return sum / this.bufferSize;
  }
};

AccelerometerEventBuffer buffer = new AccelerometerEventBuffer(10);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Orientation Sensor Test");
  Serial.println("");

  // Initialise the sensor
  if (!bno.begin()) {
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }

  delay(100);
}

void loop(void) {
  sensors_event_t accelerometerData;
  bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  processAccelerometerEvent(&accelerometerData);

  delay(BNO_SAMPLE_RATE_DELAY_MS);
}

void processAccelerometerEvent(sensors_event_t *event) {
  buffer.addEvent(&event);
  double averageX = buffer.getAverageX();
  double averageY = buffer.getAverageY();
  double averageZ = buffer.getAverageZ();

  Serial.println(averageX + ", " + averageY + ", " + averageZ);
}
