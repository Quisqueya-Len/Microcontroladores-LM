#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// Estados
enum Estado {INIT, ABIERTA, CERRADA, ABRIENDO, CERRANDO, DETENIDA, ERROR};
Estado estado_actual = INIT;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long tiempoInicio = 0;

void setup_pines() {
  pinMode(PIN_LSA, INPUT_PULLUP);
  pinMode(PIN_LSC, INPUT_PULLUP);
  pinMode(PIN_BTN_PP, INPUT_PULLUP);
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_C, INPUT_PULLUP);

  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_C, OUTPUT);
  pinMode(PIN_LED_A, OUTPUT);
  pinMode(PIN_LED_C, OUTPUT);
  pinMode(PIN_LED_ERR, OUTPUT);
}

void detener_motor() {
  digitalWrite(PIN_MOTOR_A, LOW);
  digitalWrite(PIN_MOTOR_C, LOW);
}

void abrir_porton() {
  detener_motor();
  digitalWrite(PIN_MOTOR_A, HIGH);
  tiempoInicio = millis();
  estado_actual = ABRIENDO;
}

void cerrar_porton() {
  detener_motor();
  digitalWrite(PIN_MOTOR_C, HIGH);
  tiempoInicio = millis();
  estado_actual = CERRANDO;
}

void cambiar_estado(Estado nuevo_estado) {
  estado_actual = nuevo_estado;
}

void reportar_estado() {
  const char* estados[] = {"INIT", "ABIERTA", "CERRADA", "ABRIENDO", "CERRANDO", "DETENIDA", "ERROR"};
  client.publish(MQTT_TOPIC_STATUS, estados[estado_actual]);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (msg == "ABRIR" && estado_actual == CERRADA) abrir_porton();
  else if (msg == "CERRAR" && estado_actual == ABIERTA) cerrar_porton();
  else if (msg == "DETENER") {
    detener_motor();
    cambiar_estado(DETENIDA);
  }
}

void conectar_wifi_y_mqtt() {
  WiFi.begin("Emily.", "Emilio0507122028");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  while (!client.connected()) client.connect("ESP32Porton");
  client.subscribe(MQTT_TOPIC_COMMAND);
}

void setup() {
  Serial.begin(115200);
  setup_pines();
  conectar_wifi_y_mqtt();
  reportar_estado();
}

void loop() {
  client.loop();

  bool LSA = !digitalRead(PIN_LSA);
  bool LSC = !digitalRead(PIN_LSC);

  if (estado_actual == ABRIENDO && LSA) {
    detener_motor();
    digitalWrite(PIN_LED_A, HIGH);
    cambiar_estado(ABIERTA);
    reportar_estado();
  }

  if (estado_actual == CERRANDO && LSC) {
    detener_motor();
    digitalWrite(PIN_LED_C, HIGH);
    cambiar_estado(CERRADA);
    reportar_estado();
  }

  if ((estado_actual == ABRIENDO || estado_actual == CERRANDO) && (millis() - tiempoInicio > TIEMPO_MAXIMO)) {
    detener_motor();
    digitalWrite(PIN_LED_ERR, HIGH);
    cambiar_estado(ERROR);
    reportar_estado();
  }
}