#include "inc/evse_websocket.h"
#include "inc/protocol_examples_common.h"
#include "inc/lcd20x4.h"
#include "inc/Relay.h"
#include "inc/led_strip.h"
#include "inc/GFCI.h"
#include "inc/evse_em4m.h"
#include "inc/MCP23017.h"

static const char *TAG = "ZIG TEST";
int loopCount = 0;
esp_err_t err = -1;
#if WIFI_ENABLE
void wifi_connection_chek(void *arg)
{
    while (1)
    {
        if (!isWifiConnected)
        {
            esp_wifi_connect();
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
#endif

#if PING_CHECK_ENABLE
uint8_t ping_check(void);
#endif

#if WEBSOCKET_ENABLE
#include "inc/evse_websocket.h"
#endif

#define WATCH_DOG_PIN 15

void watchdog_task(void *args)
{
    gpio_set_direction(WATCH_DOG_PIN, GPIO_MODE_OUTPUT);
    gpio_pulldown_dis(WATCH_DOG_PIN);
    gpio_pullup_dis(WATCH_DOG_PIN);
    static uint8_t state = 0;

    while (1)
    {
        gpio_set_level(WATCH_DOG_PIN, state);

        if (state == 0)
        {
            state = 1;
        }

        else if (state == 1)
        {
            state = 0;
        }
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

#define TEST_PASS (1)
#define TEST_FAIL (0)

void handle_test_value(const char *testStr);

void app_main(void)
{

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if WIFI_ENABLE
    ESP_LOGI(TAG, "Establsihing Network connection");
    connect_wifi();
    xTaskCreate(&wifi_connection_chek, "wifi connection check", 4096, "sta", 5, NULL);
    xTaskCreate(&watchdog_task, "watchdog toggle", 2048, "wtc", 5, NULL);
#endif

#if WEBSOCKET_ENABLE

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("websocket_client", ESP_LOG_DEBUG);
    esp_log_level_set("transport_ws", ESP_LOG_DEBUG);
    esp_log_level_set("trans_tcp", ESP_LOG_DEBUG);

    init_websocket();
    io_init();

    for(uint8_t i = 0; i < 16; i++)
    {
        mcp23x17_set_level(&dev, i, true);
       
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelay(6000 / portTICK_PERIOD_MS);

    for(uint8_t i = 0; i < 16; i++)
    {
        mcp23x17_set_level(&dev, i, false);
      
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    for(uint8_t i = 0; i < 16; i++)
    {
    
        mcp23x17_set_level(&dev1, i, true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    for(uint8_t i = 0; i < 16; i++)
    {
      
        mcp23x17_set_level(&dev1, i, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    // mcp23x17_set_level(&dev, 0, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 1, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 2, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 3, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 4, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 5, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 6, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 7, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 8, true);  
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 9, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 10, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 11, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 12, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 13, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 14, true);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 15, true);
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 0, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 1, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 2, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 3, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 4, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 5, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 6, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 7, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 8, false);  // 8
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 9, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 10, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 11, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 12, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 13, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 14, false);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcp23x17_set_level(&dev, 15, false);
    // // set_pin('A', 1, 1);

    err = init_energy_meter();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize energy meter: %s", esp_err_to_name(err));
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Energy meter initialized");
    }

    while (1)
    {

        if (esp_websocket_client_is_connected(client))
        {
            if (json_data_recieved)
            {
                json_data_recieved = 0;
                if (!cJSON_IsString(testValue))
                {
                    // ESP_LOGE("Web-Socket", "Invalid or missing 'Test' key");
                    cJSON_Delete(json);
                    continue;
                }

                // ESP_LOGI(TAG, "\r\nReceived Test value: %s", testValue->valuestring);

                handle_test_value(testValue->valuestring);

                // cJSON_Delete(testValue);
                cJSON_Delete(json);
                testValue = NULL;
                json = NULL;
            }
        }

        // if((loopCount++ %100) == 0)
        // {
        //     set_all_gpio(1);
        //     vTaskDelay(10000 / portTICK_PERIOD_MS);
        //     // ESP_LOGW(TAG, "Free heap size: %d bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
        // }
        // set_all_gpio(0);
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }

#endif
}

// Function to handle specific test values
void handle_test_value(const char *testStr)
{
    char *response_Json_str = NULL;
    cJSON *response_Json = NULL;
    uint8_t test_Res = -1;
    // esp_err_t err = -1;
    uint8_t test_count = 0;
    static const char *TAG = "Test";
    float read_val = 0.0;
    if (strcmp(testStr, "Energy_Meter_Relay") == 0)
    {
        ESP_LOGI(TAG, "4A Load Relay Test");
        // set_pin(POARTA, LOAD_RELAY, HIGH);
        response_Json = cJSON_CreateObject();

        // Add a string field to the JSON object
        cJSON_AddStringToObject(response_Json, "Test", "Energy_Meter_Relay");
        float v1 = 0;
        float i1 = 0;
        for (int i = 0; i < 6; i++)
        {
             v1 = read_em4m_val(VOLTAGE_PHASE_V1);
             i1 = read_em4m_val(CURRENT_I1);

            ESP_LOGI(TAG, "V1=%.1fV, I1=%.fA", v1, i1);

            vTaskDelay(pdMS_TO_TICKS(2000));

        }

        
        cJSON_AddNumberToObject(response_Json, "Voltage", v1);
        cJSON_AddNumberToObject(response_Json, "Current", i1);

        response_Json_str = cJSON_Print(response_Json);
        printf("Sending JSON: %s\n", response_Json_str);
        esp_websocket_client_send_text(client, response_Json_str, strlen(response_Json_str), portMAX_DELAY);
    }
    else if (strcmp(testStr, "Earth_Disconnect") == 0)
    {
        ESP_LOGI(TAG, "Earth_Disconnect");
       

         ESP_LOGI(TAG, "Earth_Disconnect relay on");

    }
    else if (strcmp(testStr, "Lekage_Current") == 0)
    {
      
        ESP_LOGI(TAG, "Lekage_Current");

       
        
         ESP_LOGI(TAG, "Lekage_Current relay on");
      
    }

    else if (strcmp(testStr, "Emergency_Test") == 0)
    {
       
        ESP_LOGI(TAG, "Emergency_Test");
    
         ESP_LOGI(TAG, "Emergency_Test relay on");

    }
    else if (strcmp(testStr, "Battery_Backup_Test") == 0)
    {
  
        ESP_LOGI(TAG, "Battery_Backup_Test");
        ESP_LOGI(TAG, "Battery_Backup_Test relay on");
    }
 else
    {
        ESP_LOGE(TAG, "Invalid Test");

    }
    cJSON_Delete(response_Json);    
    response_Json = NULL;
    response_Json_str = NULL;

}