//This code wite for Kidbright that use ESP32 
//Please make sure these libraries are in folder of arduino. #\Documents\Arduino\libraries 
#include <WiFi.h> //ESP32 libraries
#include <WiFiUdp.h> 

#include <NTPClient.h> //NTP libraries

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h" //LED matrix 16*8 libraries

#include <IOXhop_FirebaseESP32.h> //Firebase libraries

#define LM73_ADDR 0x4D
#define FIREBASE_HOST "YOUR_FIREBASE_HOST_URL.firebaseio.com" //please paste in this form not include https://
#define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_SECRETS"

int analog_value = 0;
double temp=0;

float readTemperature() { //Temperature sensor on kidbright board
  Wire1.beginTransmission(LM73_ADDR);
  Wire1.write(0x00); // Temperature Data Register
  Wire1.endTransmission();
 
  uint8_t count = Wire1.requestFrom(LM73_ADDR, 2);
  float temp = 0.0;
  if (count == 2) {
    byte buff[2];
    buff[0] = Wire1.read();
    buff[1] = Wire1.read();
    temp += (int)(buff[0]<<1);
    if (buff[1]&0b10000000) temp += 1.0;
    if (buff[1]&0b01000000) temp += 0.5;
    if (buff[1]&0b00100000) temp += 0.25;
    if (buff[0]&0b10000000) temp *= -1.0;
  }
  return temp;
}

// Replace with your network credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

Adafruit_8x16minimatrix matrix = Adafruit_8x16minimatrix();

void setup() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  matrix.begin(0x70);  // pass in the address
  matrix.setTextSize(1);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setRotation(1);
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  Wire1.begin(4, 5);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  
  Serial.println("");
  Serial.println("WiFi connected."); //If wifi connected print connected in LED matrix 16*8
  for (int x=17; x>=-90; x--) {
    matrix.clear();
    matrix.setCursor(x,0);
    matrix.print("WiFi Connected!");
    matrix.writeDisplay();
    delay(60);
  }
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(25200);  //GMT +7 = 25200
}
void loop() {
  
  
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitGap = formattedDate.indexOf(" ");
  dayStamp = formattedDate.substring(0, splitGap);
  Serial.print("Date: ");
  Serial.println(dayStamp);
  
  //Extract time
  timeStamp = formattedDate.substring(splitGap+1, formattedDate.length());
  Serial.print("Time: ");
  Serial.println(timeStamp);

  //Extract temperature
  temp = readTemperature();
  Serial.print("Temp: ");
  Serial.println(temp);

  //Extract LDR on kidbright board
  analog_value = analogRead(36);
  if(analog_value > 1000){
   analog_value = 1000;
  }
  int LDR = (1000 - analog_value)*100/1000;
  
  //Form data packet before send to firebase
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = temp;
  root["light"] = LDR;
  root["time"] = formattedDate;

  Firebase.push("Sensor2", root); //In this case child name in firebase is Sensor2
  
  // handle error
  if (Firebase.failed()) {
      Serial.print("pushing to firebase failed:");
      Serial.println(Firebase.error());  
      return;
  }
  Serial.print("pushed to firebase!");
  
  for (int x=17; x>=-90; x--) { //Print day in LED matrix 16*8
    matrix.clear();
    matrix.setCursor(x,0);
    matrix.print("Date:");
    matrix.println(dayStamp);
    matrix.writeDisplay();
    delay(60);
  }
  
  for (int x=17; x>=-77; x--) { //Print time in LED matrix 16*8
    matrix.clear();
    matrix.setCursor(x,0);
    matrix.print("Time:");
    matrix.println(timeStamp);
    matrix.writeDisplay();
    delay(60);
  }
  
  for (int x=17; x>=-50; x--) { //Print temperature in LED matrix 16*8
    matrix.clear();
    matrix.setCursor(x,0);
    matrix.print("*C ");
    matrix.println(temp);
    matrix.writeDisplay();
    delay(60);
  }

  for (int x=17; x>=-50; x--) { //Print LDR in LED matrix 16*8
    matrix.clear();
    matrix.setCursor(x,0);
    matrix.print("Light:");
    matrix.println(LDR);
    matrix.writeDisplay();
    delay(60);
  }
}
