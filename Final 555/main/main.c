#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "web_page.h"

#define PWM_PIN         18
#define BUTTON_PIN      0

static const char *TAG = "NE555_EMULATOR";
httpd_handle_t server = NULL;

extern void wifi_init_softap(void);
extern void pwm_init(void);
extern void pwm_set_frequency(uint32_t freq, uint8_t duty);
extern void gpio_button_init(void);
extern float calc_astable_freq(float R1, float R2, float C);
extern float calc_duty_cycle(float R1, float R2);
extern float calc_monostable_pulse(float R, float C);

static esp_err_t start_handler(httpd_req_t *req) {
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    float R1 = atof(strstr(buf, "r1=") + 3);
    float R2 = atof(strstr(buf, "r2=") + 3);
    float C = atof(strstr(buf, "c=") + 2);
    bool astable = strstr(buf, "mode=astable") != NULL;
    if (astable) {
        float freq = calc_astable_freq(R1, R2, C);
        float duty = calc_duty_cycle(R1, R2);
        pwm_set_frequency((uint32_t)freq, (uint8_t)((duty/100)*255));
    } else {
        float duration = calc_monostable_pulse(R1, C);
        pwm_set_frequency(1, 255);
        vTaskDelay(pdMS_TO_TICKS(duration * 1000));
        pwm_set_frequency(1, 0);
    }
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t stop_handler(httpd_req_t *req) {
    pwm_set_frequency(1, 0);
    httpd_resp_sendstr(req, "Stopped");
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);

    httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t start_uri = { .uri = "/start", .method = HTTP_POST, .handler = start_handler };
    httpd_register_uri_handler(server, &start_uri);

    httpd_uri_t stop_uri = { .uri = "/stop", .method = HTTP_GET, .handler = stop_handler };
    httpd_register_uri_handler(server, &stop_uri);
}

void app_main(void) {
    ESP_LOGI(TAG, "Inicializando NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init_softap();
    pwm_init();
    gpio_button_init();
    start_webserver();
}