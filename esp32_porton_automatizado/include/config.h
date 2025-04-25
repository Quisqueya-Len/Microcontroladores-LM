#ifndef CONFIG_H
#define CONFIG_H

// Pines sensores
#define PIN_LSA  12  // Límite Superior Abierto
#define PIN_LSC  14  // Límite Superior Cerrado

// Pines botones
#define PIN_BTN_PP  32
#define PIN_BTN_A   33
#define PIN_BTN_C   25

// Pines motores y LEDs
#define PIN_MOTOR_A 26
#define PIN_MOTOR_C 27
#define PIN_LED_A   2
#define PIN_LED_C   4
#define PIN_LED_ERR 13

// MQTT
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT   1883
#define MQTT_TOPIC_COMMAND "porton/control"
#define MQTT_TOPIC_STATUS  "porton/estado"

// Tiempo máximo operación
#define TIEMPO_MAXIMO 10000

#endif