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

uint16_t BNO_SAMPLE_RATE_DELAY_MS = 200; // delay between fresh samples

// Check I2C device address and correct line below (by default address is 0x29 or 0x28)
//                                    id, address
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

double max = 9.8;
double min = -9.8;

double upperBound = max * .7;
double lowerBound = min * .7;

const int BOTTOM = 0;
const int MIDDLE = 1;
const int TOP = 2;

const int RESTING_BOTTOM = 3;
const int LIFTING = 5;
const int RESTING_TOP = 6;
const int LOWERING = 4;

int currentPosition = BOTTOM;
int currentAction = RESTING_BOTTOM;

long flexStartTime;
long flexDurations[100];
int flexCount = 0;
const int minFlexCount = 3; // number of flexes needed to establish flex duration baseline

long averageFlexDuration = 2000;
long flexTimeToFailure = 5000;

bool isFailureAchieved = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Orientation Sensor Test");
  Serial.println("");

  // Initialise the sensor
  if (!bno.begin()) {
    Serial.print("Oops, no BNO055 detected... Check your wiring or I2C ADDR!");
    while (1);
  }

  delay(100);
}

void loop() {
  if (isFailureAchieved) return;

  sensors_event_t accelerometerData;
  bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  double x = accelerometerData.acceleration.x;

  int previousPosition = currentPosition;
  if (x > upperBound) {
    currentPosition = TOP;
  } else if (x < lowerBound) {
    currentPosition = BOTTOM;
  } else {
    currentPosition = MIDDLE;
  }

  int previousAction = currentAction;
  if (currentPosition == previousPosition) {
    currentAction = previousAction;
    // no action state transition to report
    if (currentAction == LIFTING) {
      // check for stall during lifting (timeout)
      long flexTimeElapsed = millis() - flexStartTime;
      checkFlexDuration(flexTimeElapsed);
    }
  } else {
    if (previousPosition == BOTTOM && currentPosition == MIDDLE) {
      currentAction = LIFTING;
    } else if (previousPosition == MIDDLE && currentPosition == TOP) {
      currentAction = RESTING_TOP;
    } else if (previousPosition == TOP && currentPosition == MIDDLE) {
      currentAction = LOWERING;
    } else if (previousPosition == MIDDLE && currentPosition == BOTTOM) {
      currentAction = RESTING_BOTTOM;
    } else {
      currentAction = 12;  // something went wrong?
    }
    // report action state transition
    handleActionTransition();
  }

  Serial.print(x);
  Serial.print(", ");
  Serial.print(max);
  Serial.print(", ");
  Serial.print(min);
  Serial.print(", ");
  Serial.print(upperBound);
  Serial.print(", ");
  Serial.print(lowerBound);
  Serial.print(", ");
  Serial.print(currentPosition);
  Serial.print(", ");
  Serial.print(currentAction);

  Serial.println();

  delay(BNO_SAMPLE_RATE_DELAY_MS);
}

void handleActionTransition() {
  switch (currentAction) {
    case LIFTING: // started lifting from bottom
      flexStartTime = millis();
      break;
    case RESTING_TOP: // reached top
      long flexStopTime = millis();
      long flexDuration = flexStopTime - flexStartTime;
      flexDurations[flexCount++] = flexDuration;
      if (flexCount >= minFlexCount) {
        setFlexPace();
      }

      checkFlexDuration(flexDuration);
      break;
  }
}

void setFlexPace() {
  long sumFlexDurations = 0;
  for (int i = 0; i < flexCount; i++) {
    sumFlexDurations += flexDurations[i];
  }
  long averageFlexDuration = sumFlexDurations / flexCount;
  flexTimeToFailure = (long) (averageFlexDuration * 2.5);
}

void checkFlexDuration(long flexDuration) {
  if (flexDuration > flexTimeToFailure) {
    isFailureAchieved = true;
  }
}
