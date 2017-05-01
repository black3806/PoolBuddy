#include "DS18B20.h"
#include "SparkFunMAX17043.h"

#define TEMP_SENSOR D6
#define PH_ADDRESS 99
#define ORP_ADDRESS 98

DS18B20 ds18b20(TEMP_SENSOR);
double celsius;
unsigned int Metric_Publish_Rate = 30000;
unsigned int MetricnextPublishTime;
int nextSampleTime, nextSleepTime;
int SAMPLE_INTERVAL = 2500;
int SLEEP_INTERVAL = 60000 * 5;
int WAKE_INTERVAL = 60000;

double pH = 0;
double ORP = 0;
double soc = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(TEMP_SENSOR, INPUT);

  lipo.begin();
  lipo.quickStart();

  Particle.variable("temp", celsius);
  Particle.variable("pH", pH);
  Particle.variable("ORP", ORP);
  Particle.variable("soc", soc);
  Particle.function("calibratePh",calibrate_ph);
  Particle.function("calibrateORP",calibrate_orp);
  Particle.function("check",check_calibrated);
  Particle.function("slope",check_slope);

  nextSleepTime = millis() + WAKE_INTERVAL;
}

void loop() {
  if (millis() > nextSampleTime){
    getTemp();
    getPh();
    getORP();
    nextSampleTime = millis() + SAMPLE_INTERVAL;
    soc = lipo.getSOC();
    }
  if(millis() > nextSleepTime) {
    Particle.publish("waterdata", water_data(), PRIVATE);
    nextSleepTime = millis() + SLEEP_INTERVAL + WAKE_INTERVAL;
    executeRequest(PH_ADDRESS,"Sleep");
    executeRequest(ORP_ADDRESS,"Sleep");
    System.sleep(SLEEP_MODE_DEEP,60*5);
  }
}

String water_data() {
  return String::format("{temperature:%f,ph:%f,orp:%f,soc:%f}",celsius,pH,ORP,soc);
}

void getPh() {
  String pH_data = executeRequest(PH_ADDRESS,"R");
  Serial.print("Ph:");
  Serial.println(pH_data);
  if (isdigit(pH_data[0]))
    pH = String(pH_data).toFloat();
 }

void getORP() {
  String orp_data = executeRequest(ORP_ADDRESS,"R");
  Serial.print("ORP:");
  Serial.println(orp_data);
  if (isdigit(orp_data[0]))
    ORP = orp_data.toFloat();
}

int calibrate_ph(String arg) {
  String commandString = String("Cal");
  commandString.concat(",");
  commandString.concat(arg);
  commandString.concat(",");
  if(arg=="mid")
    commandString.concat("7.00");
  if(arg=="low")
    commandString.concat("4.00");
  if(arg=="high")
    commandString.concat("10.00");
  executeRequest(PH_ADDRESS,commandString);
  return 0;
}

int calibrate_orp(String arg) {
  String commandString = String("Cal");
  commandString.concat(",");
  commandString.concat(arg);
  executeRequest(ORP_ADDRESS,commandString);
  return 0;
}

int check_calibrated(String args) {
  executeRequest(PH_ADDRESS,"Cal,?");
  executeRequest(ORP_ADDRESS,"Cal,?");
  return 0;
}

int check_slope(String args) {
  executeRequest(PH_ADDRESS,"Slope,?");
  return 0;
}

void getTemp() {
  int dsAttempts = 0;

  if(!ds18b20.search()){
    ds18b20.resetsearch();
    celsius = ds18b20.getTemperature();
    Serial.printlnf("%f celsius",celsius);
    while (!ds18b20.crcCheck() && dsAttempts < 4){
      Serial.println("Caught bad value.");
      dsAttempts++;
      Serial.print("Attempts to Read: ");
      Serial.println(dsAttempts);
      if (dsAttempts == 3){
        delay(1000);
      }
      ds18b20.resetsearch();
      celsius = ds18b20.getTemperature();
      continue;
    }
  }
}

String executeRequest(int address, String cmd) {
  char data[20];
  int i=0;

  Wire.beginTransmission(address);
  Wire.write(cmd);
  Wire.endTransmission();
  if(cmd=="R")
    delay(800);
  else
    delay(300);
  Wire.requestFrom(address, 20);
  switch (Wire.read()) {
    case 1:
      Serial.println("Success");
      break;
    case 2:
      Serial.println("Failed");
      return String("Failed");
      break;
    case 254:
      Serial.println("Pending");
      break;
    case 255:
      Serial.println("No Data");
      break;
  }

  while(Wire.available()) {
    data[i]= Wire.read();
    i+=1;
  }
  Wire.endTransmission();
  return String(data);
}
