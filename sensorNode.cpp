#include <Arduino.h>
#include <LoRa.h>
#include "DHT.h"
#include <PMSerial.h>
#include <Wire.h>
#include <chrono>
#include <thread>
#include <ctime> //Used only for localtime and mktime
#ifndef ESP32
#include <SoftwareSerial.h>
#endif
using namespace std;

//Geolocation attributes (Fixed)
string SN_name = "Udine";
string SN_lat = "46.08092343193739";
string SN_lon = "13.211915210647616";
string SN_alt = "500";

//MQ135 setup
#define MQ_PIN 34 

//DHT sensor library setup
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

//PMS5003 Setup - using 16/17 as RX/TX
#if !defined(PMS_RX) && !defined(PMS_TX)
constexpr auto PMS_RX = 16;
constexpr auto PMS_TX = 17;
#endif

#ifndef ESP32
SerialPM pms(PMS5003, PMS_RX, PMS_TX); // PMSx003, RX, TX
// Alternative:
//SoftwareSerial SoftSerial1(PMS_RX, PMS_TX);
//SerialPM pms(PMS5003, SoftSerial1);
#else
SerialPM pms(PMS5003, PMS_RX, PMS_TX); // PMSx003, RX, TX
#endif

string DHT11Measurement(){
  string result = "";
  float temp_c = dht.readTemperature(); // read temperature in celsius
  float temp_f = dht.readTemperature(true); // read temperature in fahrenheit (true = fahrenheit, false or default = celsius)
  float humidity = dht.readHumidity(); // read humidity
  if(isnan(temp_c) || isnan(temp_f) || isnan(humidity)){ // if one or more values is invalid measurement is considered unrealiable
    // send error message as result
    Serial.println("Error in getting DHT11 readings, readings are not available or invalid");
    result = "#DHT11/ERROR:Could_not_get_a_valid_measurement";
  }else{ // measurement is valid
    //return data to be attached to message
    float heat_index = dht.computeHeatIndex(temp_c, humidity, false);
    result = "#DHT11/Temperature_Celsius:" + to_string(temp_c);
    result = result + "/Temperature_Fahrenheit:" + to_string(temp_f);
    result = result + "/Temperature_Feels_like:" + to_string(heat_index);
    result = result + "/Humidity:" + to_string(humidity);
    Serial.println("Readings from DHT11 have been successful");
  }
  return result;
}

string MQ135Measurement(){
  string result;
  //Read mixed measurement from pin
  float measurement = analogRead(MQ_PIN);
  if(isnan(measurement)){ // if results are invalid
    //send error message as result
    result = "#MQ135/ERROR:Could_not_get_a_valid_measurement";
  }else{ //measurements are valid
    //return data to be attached to message
    Serial.println("Measurement from MQ135 has been successful");
    Serial.println(measurement);
    result = "#MQ135/MQ:" + to_string(measurement);
  }
  return result;
}

string PMS5003Measurement(){
  string result = "";
  pms.read();
  if(pms){ //Successfull read
    //return data to be attached to message
    Serial.println("Measurement from PMS has been successful");
    result = "#PMS5003";
    result = result + "/PM1.0:" + to_string(pms.pm01);
    result = result + "/PM2.5:" + to_string(pms.pm25);
    result = result + "/PM10:" + to_string(pms.pm10);
    if (pms.has_number_concentration()){ //to attach additional readings uncomment
      Serial.println("Number concentration readings have been successful too");
      /*result = result + "/N0.3:" + to_string(pms.n0p3);
      result = result + "/N0.5:" + to_string(pms.n0p5);
      result = result + "/N1.0:" + to_string(pms.n1p0);
      result = result + "/N2.5:" + to_string(pms.n2p5);
      result = result + "/N5.0:" + to_string(pms.n5p0);
      result = result + "/N10:" + to_string(pms.n10p0);*/
    }
  }else{ //Read not succesfull
    result = "#PMS5003/ERROR:";
    //attach the error that occurred to be sent as a result
    Serial.println("An error occurred with the PMS5003 sensor, the error: ");
    switch (pms.status){
      case pms.OK: //Should never come here, Included to compile with no warnings
        break;     
      case pms.ERROR_TIMEOUT:
        result = result + "ERROR_TIMEOUT";
        Serial.println(F(PMS_ERROR_TIMEOUT));
        break;
      case pms.ERROR_MSG_UNKNOWN:
        result = result + "ERROR_MSG_UNKNOWN";
        Serial.println(F(PMS_ERROR_MSG_UNKNOWN));
        break;
      case pms.ERROR_MSG_HEADER:
        result = result + "ERROR_MSG_HEADER";
        Serial.println(F(PMS_ERROR_MSG_HEADER));
        break;
      case pms.ERROR_MSG_BODY:
        result = result + "ERROR_MSG_BODY";
        Serial.println(F(PMS_ERROR_MSG_BODY));
        break;
      case pms.ERROR_MSG_START:
        result = result + "ERROR_MSG_START";
        Serial.println(F(PMS_ERROR_MSG_START));
        break;
      case pms.ERROR_MSG_LENGTH:
        result = result + "ERROR_MSG_LENGTH";
        Serial.println(F(PMS_ERROR_MSG_LENGTH));
        break;
      case pms.ERROR_MSG_CKSUM:
        result = result + "ERROR_MSG_CKSUM";
        Serial.println(F(PMS_ERROR_MSG_CKSUM));
        break;
      case pms.ERROR_PMS_TYPE:
        result = result + "ERROR_PMS_TYPE";
        Serial.println(F(PMS_ERROR_PMS_TYPE));
        break;
    }
  }
  return result;
}

void SendMessage(string message){
  //transmission sequence: start -> print message -> end
  //if stuck in end phase check wiring
  LoRa.beginPacket();
  Serial.println("Starting transmission sequence...");
  LoRa.print(message.c_str());
  Serial.println("Ending transmission sequence...");
  LoRa.endPacket();
  Serial.println("Message should be on it's way");
  Serial.println("\n-----\n");
}

void WaitNextMinute(){
  //current time in UTC
  auto now = chrono::system_clock::now();
  time_t now_c = chrono::system_clock::to_time_t(now);
  tm now_tm = *gmtime(&now_c);
  //calculate next minute mark
  now_tm.tm_sec = 0;
  now_tm.tm_min += 1;
  time_t next_minute_c = mktime(&now_tm);
  auto next_minute = chrono::system_clock::from_time_t(next_minute_c);
  auto duration = next_minute - now;
  //wait for the next minute mark
  this_thread::sleep_for(duration);
}

void WaitNextHour(){
  //current time in UTC
  auto now = chrono::system_clock::now();
  time_t now_c = chrono::system_clock::to_time_t(now);
  tm now_tm = *gmtime(&now_c);
  //calculate next hour mark
  now_tm.tm_sec = 0;
  now_tm.tm_min = 0;
  now_tm.tm_hour += 1;
  time_t next_hour_c = mktime(&now_tm);
  auto next_hour = chrono::system_clock::from_time_t(next_hour_c);
  auto duration = next_hour - now;
  //wait for the next hour mark
  this_thread::sleep_for(duration);
}

void setup() { 
  //initiate serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial communication should be functional now...");
  //Startup DHT 11
  try{
    dht.begin();
    Serial.println("DHT sensor has been setup correctly");
  }catch(const exception e){
    Serial.println("ERROR: There was a problem setting up the DHT11 sensor...");
  }
  delay(1000);

  //NOTE: MQ135 sensor DOES NOT require additional setup actions, it only REQUIRES heating time

  //Setup PMS5003
  try{
    pms.init();
  }catch(exception e){
    Serial.println("ERROR: There was a problem setting up the PMS5003 sensor...");
  }
  //Setup Lora Communication
  try{
    //use default settings for coding rate and bandwidth
    LoRa.setPins(5, 14, 2);
    LoRa.setTxPower(17);
    //Use default settings for coding rate, bandwidth and spreading factor
    //LoRa.setSyncWord(0x12);
    Serial.println("Lora pins defined");
    Serial.println("Lora startup");
  }catch(exception e){ // lora communication couldn't be setup
    Serial.println("ERROR: There was a problem setting up the LoRa communication");
  }
  while(!LoRa.begin(868E6)){ // is lora setup failed the program won't proceed 
    Serial.print(".");
    delay(500);
  }
}

void loop() {
  //Used for testing:
  WaitNextMinute();
  //waits for the start of the next minute (i.e: 12:00, 12:01, 12:02, etc...)

  //Ideally use:
  //WaitNextHour();
  //to measure once at the start of every hour (i.e: 12:00, 13:00, 14:00, etc...)
  //to not create too many measurements

  Serial.println("\n\n\n");
  string message = "SNGeo/Name:" + SN_name + "/Lat:" + SN_lat + "/Lon:" + SN_lon + "/Alt:" + SN_alt; 
  //create a initial section of the message containing sensor node name, coordinates and altitude
  //defined at the start of the program

  string tmp; //auxiliary string variable
  //Elaborate the data from the sensors
  //if measurements are valid add them to the message, in the event of an error a brief description will be added
  //DHT11
  try{
    Serial.println("Attempting to get DHT11 measurement");
    tmp = DHT11Measurement();
    Serial.println("DHT11 has sent results:\n");    
    Serial.println(tmp.c_str());
    Serial.println("Result added to package");
    message = message + tmp;
  }catch(exception e){ // if results (measurement or error message) could not be received then attach error
    Serial.println("ERROR: Could not reach DHT11 Sensor");
    message = message + "#DHT11/ERROR:Could_not_reach_sensor";
  }
  //MQ135
  try{
    Serial.println("Attempting to get MQ135 measurement");
    tmp = MQ135Measurement();
    Serial.println("MQ has sent results:\n");
    Serial.println(tmp.c_str());
    Serial.println("Result added to package");
    message = message + tmp;
  }catch(exception e){ // if results (measurement or error message) could not be received then attach error
    Serial.println("ERROR: Could not reach MQ135 Sensor");
    message = message + "#MQ135/ERROR:Could_not_reach_sensor";
  }
  //PMS5003
  try{
    Serial.println("Attempting to get PMS measurement");
    tmp = PMS5003Measurement();
    Serial.println("PMS has sent results:\n");
    Serial.println(tmp.c_str());
    Serial.println("Result added to package");
    message = message + tmp;
  }catch(exception e){ // if results (measurement or error message) could not be received then attach error
    Serial.println("ERROR:Could not reach PMS Sensor");
    message = message + "#PMS5003/ERROR:Could_not_reach_sensor";
  }
  //Start communication sequence
  SendMessage(message);
}
