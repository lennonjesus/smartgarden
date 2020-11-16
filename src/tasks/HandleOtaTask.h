#include <Arduino.h>
#include <ArduinoOTA.h>

const char *OTA_HOSTNAME = "Smart Garden";
const char *OTA_PWD = "";

void configureOta();

TaskHandle_t HandleOtaTaskInstance;

void HandleOtaTask(void *args) {

  Serial.println((String) __func__ + " rodando em " + (String) xPortGetCoreID());

  configureOta();

  for (;;) {
    ArduinoOTA.handle();

    delay(1000 * 10);
  }
}

void configureOta() {

  delay(200);

  // ArduinoOTA.setPort(8266); 
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PWD);
 
  ArduinoOTA.onStart([]() {
    Serial.println("Inicio...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("nFim!");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
    
    if (error == OTA_AUTH_ERROR) Serial.println("Autenticacao Falhou");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Inicio");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na Conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na Recepcao");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim");
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA pronto");
}