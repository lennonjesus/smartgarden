// ESP32S

#include <Arduino.h>
#include <WiFi.h>
#include "SSD1306Wire.h"
#include "Wire.h"
#include <DHT.h>

#define uS_TO_S_FACTOR 1000000 // conv. de microsegundos para segundos
#define TIME_TO_SLEEP 300 // tempo de hibernacao em s
#define ENA 2
#define IN1 18
#define IN2 19
#define WATER_LVL 4
#define APIN_SOIL_HUM 33
#define APIN_UV 34
#define PWR_PIN_SOIL_HUM 21

/* REDE */
String THINGSPEAK_API_KEY = "";
const char *ssid =  "";
const char *pass =  "";
const char *server = "api.thingspeak.com";
WiFiClient client;

/* OLED */
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = 22;
const int SCL_PIN = 23;
SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SCL_PIN);

/* DHT22*/
const int DHT22_PIN = 27;
DHT dht(DHT22_PIN, DHT22);

/* valores */
int soilMoisture = 0;
String soilMoistureState = "???";
bool isSoilDry = false;

float dataSensorUV = 0;
int indexUV = 0;

float humidade = 0;
float temperatura = 0;

bool hasWater = false;
bool isMotorOn = false;

void readSoilMoisture();
void readSensorUV();
void readWeather();
void readWaterLevel();

void displaySetup();
void displayMessage(String message, int milliseconds);

void displaySoilMoisture();
void displayWeatherAndUV();

void irrigacao();
void sendDataToServer();

void hibernate();

TaskHandle_t WiFiConnectTaskInstance;

void WiFiConnectTask(void *arg) {

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  for(;;) {
    
    delay(100);

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      
      Serial.print("Conectando em ");
      Serial.print(ssid);

      int timeout = 0;

      while (WiFi.status() != WL_CONNECTED && ++timeout <= 10) {
        digitalWrite(BUILTIN_LED, HIGH);
        delay(500);
        digitalWrite(BUILTIN_LED, LOW);
        delay(500);
        Serial.print("."); 
      }

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(" Conexão não estabelecida. Reiniciando...");
        // ESP.restart();
        WiFi.reconnect();
      }

      Serial.println("");
      Serial.print("Conectado em ");
      Serial.print(ssid);
      Serial.print(" com o IP ");
      Serial.println(WiFi.localIP());
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(PWR_PIN_SOIL_HUM, OUTPUT);

  pinMode(WATER_LVL, INPUT);

  dht.begin();

  WiFi.begin(ssid, pass);

  delay(100);

  xTaskCreatePinnedToCore(WiFiConnectTask, 
                      "WiFiConnectTask", 
                      2048, 
                      &WiFiConnectTaskInstance, 
                      2, 
                      NULL, 
                      PRO_CPU_NUM);

  displaySetup();
}

void loop() {

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  Serial.println("Iniciando leituras...");

  readSoilMoisture();
  delay(500);

  readSensorUV();
  delay(500);

  readWeather();
  delay(500);

  readWaterLevel();
  delay(500);

  displaySoilMoisture();
  delay(2000);

  displayWeatherAndUV();
  delay(2000);

  irrigacao();
  delay(1000);

  sendDataToServer();

  hibernate();
  delay(TIME_TO_SLEEP * 1000);
}





/* LEITURAS */
void readSoilMoisture() {

  digitalWrite(PWR_PIN_SOIL_HUM, HIGH);

  delay(500);

  soilMoisture = 100 - (analogRead(APIN_SOIL_HUM) * 100 ) / 4095;

  digitalWrite(PWR_PIN_SOIL_HUM, LOW);

  if (soilMoisture <= 30) {
    soilMoistureState = "Seco";
    isSoilDry = true;
    Serial.println(soilMoistureState);
  }
 
  if (soilMoisture > 30 && soilMoisture <= 50) {
    soilMoistureState = "Moderado";
    Serial.println(soilMoistureState); 
  }
   
  if (soilMoisture > 50 && soilMoisture <= 85) {
    soilMoistureState = "Umido";
    Serial.println(soilMoistureState);
  }

  if (soilMoisture > 85) {
    soilMoistureState = "Encharcado";
    Serial.println(soilMoistureState);
  }
}

/* Read UV Sensor in mV and calculate UV index */
void readSensorUV() {

  delay(500);

  byte numOfReadings = 10;
  dataSensorUV = 0.0;
  
  for (int idx = 0; idx < numOfReadings; idx++) {
    dataSensorUV += analogRead(APIN_UV);
    delay (200);
  }
  
  dataSensorUV /= numOfReadings;
  dataSensorUV = (dataSensorUV * (3.3 / 1023.0)) * 1000.0;
  
  Serial.println("UV: " + (String) dataSensorUV);
  
  if (dataSensorUV < 227) indexUV = 0;
  else if (227 <= dataSensorUV && dataSensorUV < 318) indexUV = 1;
  else if (318 <= dataSensorUV && dataSensorUV < 408) indexUV = 2;
  else if (408 <= dataSensorUV && dataSensorUV < 503) indexUV = 3;
  else if (503 <= dataSensorUV && dataSensorUV < 606) indexUV = 4;    
  else if (606 <= dataSensorUV && dataSensorUV < 696) indexUV = 5;
  else if (696 <= dataSensorUV && dataSensorUV < 795) indexUV = 6;
  else if (795 <= dataSensorUV && dataSensorUV < 881) indexUV = 7; 
  else if (881 <= dataSensorUV && dataSensorUV < 976) indexUV = 8;
  else if (976 <= dataSensorUV && dataSensorUV < 1079) indexUV = 9;  
  else if (1079 <= dataSensorUV && dataSensorUV < 1170) indexUV = 10;
  else indexUV = 11;
  
}

/* Get DHT data */
void readWeather() {
  
  delay(500);

  float tempIni = temperatura;
  float humIni = humidade;

  temperatura = dht.readTemperature();
  humidade = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidade) || isnan(temperatura)) {
    Serial.println("Failed to read from DHT sensor!");
    temperatura = tempIni;
    humidade = humIni;
    return;
  } 

  Serial.print("Temperatura: ");
  Serial.println(temperatura);
  Serial.print("Humidade: ");
  Serial.println(humidade);
}

void readWaterLevel() {

  hasWater = !digitalRead(WATER_LVL);

  if (hasWater) {
    Serial.println("Tem agua");
  } else {
    Serial.println("Nao tem agua");
  }
}
/* FIM LEITURAS */ 

/* ACTIONS */
void irrigacao() {

  do {

    readWaterLevel();
    readSoilMoisture();

    if (!isMotorOn) {
      Serial.println("Ligando bomba");
      
      digitalWrite(ENA, HIGH);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);

      isMotorOn = true;

      displayMessage("Bomba on", 1000);
    }

    Serial.println("Irrigando...");
  } while (hasWater && isSoilDry);

  if (isMotorOn) {
    Serial.println("Desligando bomba");

    digitalWrite(ENA, LOW);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    displayMessage("Bomba off", 1000);
    
    isMotorOn = false;
  }
}

void sendDataToServer() {
  
  if (client.connect(server, 80)) {

    Serial.println("Enviando dados...");
  
    String postStr = THINGSPEAK_API_KEY;
    
    postStr +="&field1=";
    postStr += String(temperatura);
    postStr +="&field2=";
    postStr += String(humidade);
    postStr +="&field3=";
    postStr += soilMoisture;
    postStr +="&field4=";
    postStr += indexUV;
    postStr +="&field5=";
    postStr += dataSensorUV;
    postStr +="&field6=";
    postStr += hasWater;
    postStr += "\r\n\r\n";
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + THINGSPEAK_API_KEY + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    delay(1000);
    Serial.println("Dados enviados.");
  
  } else {
    Serial.println("Falha ao enviar os dados!");
  }
  
  client.stop();
}
/* FIM ACTIONS */

/* OLED */
void displaySetup() {
  display.init();
  display.clear();
  display.display();
  
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 0, "Smart Garden");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 52, "SW Ver.:");
  display.drawString(45, 52, "0.0.1");
  display.display();

  delay(1000);
  
  for (int value = 0; value <= 100; value+=5) {
    display.clear();
    display.display();
    
    // x, y, width, height, value
    display.drawProgressBar(10, 32, 100, 10, value);
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(value) + "%");
    display.display();
    delay(20);
  }

  delay(500);
}

void displayMessage(String message, int milliseconds) {
  display.clear();
  display.display();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 0, message);
  display.display();
  delay(milliseconds); 
}

/*  Display Soil Moisture Values on local OLED*/
void displaySoilMoisture() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Solo");
  display.drawString(0, 23, "Umid: " + String(soilMoisture) + "%" );
  display.drawString(0, 48, "Est: " + soilMoistureState);
  display.display();
}

/*  Display Weather and UV Values on local OLED*/
void displayWeatherAndUV() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Temp: " + String(temperatura) + "ºC");
  display.drawString(0, 15, "Hum: " + String(humidade) + "%");
  display.drawString(0, 30, "UV (mV):" + String(dataSensorUV));
  display.drawString(0, 45, "UV Index:" + String(indexUV));
  display.display();
}
/* FIM OLED */

/* UTIL */
void hibernate() {
  displayMessage("Bye, bye!", 1000);

  Serial.println("Desligando para economizar energia..."); 
  display.displayOff();
  esp_deep_sleep_start();

  Serial.println("Jah foi dormir...");
}
/* END UTIL */