/*
 * main.c
 *
 *  Created on: 6 nov 2023
 *      Author: Mattia Bonfanti
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "freertos/event_groups.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs.h"
static const char *TAG = "MQTT_EXAMPLE";

#define  EXAMPLE_ESP_WIFI_SSID ""
#define  EXAMPLE_ESP_WIFI_PASS ""

TaskHandle_t task;



uint32_t MQTT_CONNEECTED = 0;

static void mqtt_app_start(void);

//gestione eventi Wi-Fi
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "Connessione in corso alla rete WiFi...\n");


        break;

    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Wi-Fi connesso\n");

        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Inizializzazione ottenuto IP\n");

        mqtt_app_start();
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnesso: tentativo di riconnessione Wi-Fi\n");


        esp_wifi_connect();
        break;

    default:
        break;
    }
    return ESP_OK;
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));


    wifi_config_t wifi_config = {
        .sta = {
        	.ssid=EXAMPLE_ESP_WIFI_SSID,
			.password=EXAMPLE_ESP_WIFI_PASS,
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
}



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNESSO");
        MQTT_CONNEECTED=1;

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test1", 0);
        ESP_LOGI(TAG, "sottoscrizione avvenuta con successo, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "sottoscrizione avvenuta con successo, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test3", 1);
                ESP_LOGI(TAG, "sottoscrizione avvenuta con successo, msg_id=%d", msg_id);


        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNESSO");
        MQTT_CONNEECTED=0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SOTTOSCRIZIONE msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_ANNULLA_SOTTOSCRIZIONE, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBBLICATO, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERRORE");
        break;
    default:
        ESP_LOGI(TAG, "Altro evento id:%d", event->event_id);
        break;
    }
}

esp_mqtt_client_handle_t client = NULL;


void Publisher_Task(void *params)
{
  while (true)
  {
    if(MQTT_CONNEECTED)
    {
        esp_mqtt_client_publish(client, "/topic/test3", "Hello World", 0,1,0);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
  }
}


static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .uri = "mqtt://broker.emqx.io:1883"};
    	

    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    xTaskCreate(Publisher_Task,"Publisher Task", 2048 ,NULL,1,NULL);
}


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);



    wifi_init();

}
