#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

const char *SSID =  "";
const char *PASS =  "";

TaskHandle_t WiFiConnectTaskInstance;

void WiFiConnectTask(void *arg) {

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  WiFi.begin(SSID, PASS);

  delay(500);

  for(;;) {
    
    delay(100);

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      
      Serial.print("Conectando na rede ");
      Serial.println(SSID);

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
      Serial.print(WiFi.SSID());
      Serial.print(" com o IP ");
      Serial.println(WiFi.localIP());

      ArduinoOTA.begin();
    }
  }
}