//Bibliotecas
#include "ArduinoJson.h"
#include "EspMQTTClient.h"

//Declaração de variáveis
int ledStatus = 2;
int rele1 = 22;
int estadoRele1 = 0;

// variáveis para o botão:
int btn1 = 23;
int estadoAtual = 0;
int ultimoEstado = 1;

int limiteBounce = 30;
int ultimoTempoLimiteBounce = 0;

//-------------------------------------------------//
//configurações da conexão MQTT
EspMQTTClient client(
  "FIESC_IOT",                             //nome da sua rede Wi-Fi
  "7FuhM4@Io9",                            //senha da sua rede Wi-Fi
  "mqtt.tago.io",                          // MQTT Broker server ip padrão da tago
  "Default",                               // username
  "d16ce843-51dc-4ffa-8157-a98c05ec6641",  // Código do Token DA PLATAFORMA TAGOIO
  "senairele1",                            // Client name that uniquely identify your device
  1883                                     // The MQTT port, default to 1883. this line can be omitted
);
//-------------------------------------------------//

//Tópico
char topicoMqtt[] = "senai/esp32/001";

/*
Esta função é chamada assim que tudo estiver conectado (Wifi e MQTT)]
AVISO: VOCÊ DEVE IMPLEMENTÁ-LO SE USAR EspMQTTClient
*/
void onConnectionEstablished() {
  client.subscribe(topicoMqtt, onMessageReceived);
}

void onMessageReceived(const String& msg) {
  digitalWrite(ledStatus, LOW);
  Serial.println("Mensagem recebida:");
  Serial.println(msg);

  StaticJsonBuffer<300> JSONBuffer;                  //Memory pool
  JsonObject& parsed = JSONBuffer.parseObject(msg);  //Parse message

  if (parsed.success()) {
    int rele = parsed["led"];

    if (rele == 1) {
      ligarRele();
      Serial.println("Rele ligado por MQTT");
    } else if (rele == 2) {
      desligarRele();
      Serial.println("Rele desligado por MQTT");
    }

  } else {
    Serial.println("Falha ao realizar parsing do JSON");
  }
}

void transmitirStatusRele() {
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["statusLed"] = estadoRele1;

  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  client.publish(topicoMqtt, JSONmessageBuffer);
}

void setup() {
  //Ativando saída serial
  Serial.begin(9600);

  //Ativando principais portas

  //Botão
  pinMode(btn1, INPUT_PULLUP);

  //Relê
  pinMode(rele1, OUTPUT);
  digitalWrite(rele1, HIGH);

  //Informação de Placa ligada
  pinMode(ledStatus, OUTPUT);
  digitalWrite(ledStatus, LOW);

  //Cria a tarefa para o loop2
  Serial.printf("\nsetup() em core: %d", xPortGetCoreID());
  xTaskCreatePinnedToCore(loop2, "loop2", 8192, NULL, 1, NULL, 0);
}

void alternarEstadoRele() {

  if (estadoRele1 == 0) {
    //Liga o relê
    estadoRele1 = 1;
    Serial.println("Ligando rele");
    digitalWrite(rele1, LOW);
  } else {
    estadoRele1 = 0;
    Serial.println("Desligando");
    digitalWrite(rele1, HIGH);
  }

  transmitirStatusRele();
}

void lerBotao() {
  if (millis() - ultimoTempoLimiteBounce >= limiteBounce) {
    ultimoTempoLimiteBounce = millis();

    estadoAtual = digitalRead(btn1);

    if (ultimoEstado == HIGH && estadoAtual == LOW) {
      //Botão precionado

      alternarEstadoRele();

    } else if (ultimoEstado == LOW && estadoAtual == HIGH) {
      //Botão liberado
    }

    ultimoEstado = estadoAtual;
  }
}

void lerSerial() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') {
      alternarEstadoRele();
    }
  }
}

void ligarRele() {
  estadoRele1 = 1;
  digitalWrite(rele1, LOW);
  transmitirStatusRele();
}

void desligarRele() {
  estadoRele1 = 0;
  digitalWrite(rele1, HIGH);
  transmitirStatusRele();
}

void verificarWifi() {
  //Alternar o led "azul" do ESP32 conforme estado na conexão
  if (client.isWifiConnected()) {
    digitalWrite(ledStatus, HIGH);
  } else {
    digitalWrite(ledStatus, LOW);
  }
}

void loop() {
  lerBotao();
  lerSerial();
}

void loop2(void* z) {
  //Mostra no monitor em qual core o loop2() foi chamado
  Serial.printf("\nloop2() em core: %d", xPortGetCoreID());

  while (1)  //Pisca o led infinitamente
  {
    client.loop();
    verificarWifi();
    delay(10);
  }
}
