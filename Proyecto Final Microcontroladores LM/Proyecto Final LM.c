#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "mqtt_client.h"

// Configuración de pines
#define LED_PIN 2
#define BUTTON_PIN 13

// Estados del LED
#define STATE_OFF 1
#define STATE_BLINK_05 2
#define STATE_BLINK_01 3
#define STATE_BLINK_1 4
#define STATE_PROGRESSIVE 5

// Variables globales
int current_state = STATE_OFF;  // Estado actual
int progressive_interval = 100; // Intervalo inicial del estado progresivo
bool button_last_state = false;

// MQTT
#define MQTT_BROKER "mqtt://broker.hivemq.com"
#define MQTT_TOPIC_COMMANDS "esp32/commands"
#define MQTT_TOPIC_STATUS "esp32/status"

esp_mqtt_client_handle_t mqtt_client;

// Temporizador
TimerHandle_t led_timer;
bool led_state = false;

// Prototipos de funciones
void configure_gpio(void);
void set_timer(int interval);
void stop_timer(void);
void timer_callback(TimerHandle_t pxTimer);
void handle_state_change(void);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

// Configuración GPIO
void configure_gpio(void) {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_set_level(LED_PIN, false);
}

// Configuración del temporizador
void set_timer(int interval) {
    if (led_timer == NULL) {
        led_timer = xTimerCreate("LED Timer", pdMS_TO_TICKS(interval), pdTRUE, NULL, timer_callback);
        if (led_timer != NULL) {
            xTimerStart(led_timer, 0);
        }
    } else {
        xTimerChangePeriod(led_timer, pdMS_TO_TICKS(interval), 0);
    }
}

void stop_timer(void) {
    if (led_timer != NULL) {
        xTimerStop(led_timer, 0);
        gpio_set_level(LED_PIN, false);
    }
}

// Callback del temporizador
void timer_callback(TimerHandle_t pxTimer) {
    led_state = !led_state;
    gpio_set_level(LED_PIN, led_state);
}

// Incrementa el estado y reinicia si es necesario
void increment_state(void) {
    current_state++;
    if (current_state > STATE_PROGRESSIVE) {
        current_state = STATE_OFF;
    }
    handle_state_change();
}

// Cambiar estado basado en botón o MQTT
void handle_state_change(void) {
    // Aplica acción basada en el estado
    switch (current_state) {
        case STATE_OFF:
            stop_timer();
            break;
        case STATE_BLINK_05:
            set_timer(500);
            break;
        case STATE_BLINK_01:
            set_timer(100);
            break;
        case STATE_BLINK_1:
            set_timer(1000);
            break;
        case STATE_PROGRESSIVE:
            progressive_interval = 100;  // Reinicia intervalo progresivo
            set_timer(progressive_interval);
            break;
    }

    // Publica el estado actual mediante MQTT
    char state_message[50];
    snprintf(state_message, sizeof(state_message), "Current state: %d", current_state);
    esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS, state_message, 0, 1, 0);
}

// Control del temporizador en estado progresivo
void control_progressive_state(void) {
    if (current_state == STATE_PROGRESSIVE) {
        progressive_interval += 250;
        if (progressive_interval > 1000) {
            progressive_interval = 100;
        }
        set_timer(progressive_interval);
    }
}

// Manejo de eventos MQTT
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_DATA:
            increment_state(); // Actúa como pulsador
            break;
        default:
            break;
    }
}

// Configuración de MQTT
void setup_mqtt(void) {
    esp_mqtt_client_config_t mqtt_config = {
        .uri = MQTT_BROKER,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// Tarea principal
void app_main(void) {
    configure_gpio();
    setup_mqtt();

    while (1) {
        bool button_state = !gpio_get_level(BUTTON_PIN);

        // Detecta cambio en el botón
        if (button_state && !button_last_state) {
            increment_state(); // Cambia estado al presionar el botón
        }
        button_last_state = button_state;

        control_progressive_state(); // Controla estado progresivo
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
