#include "evse_websocket.h"

#if WIFI_ENABLE
static const char *TAG1 = "WiFi-Network";

uint8_t isWifiConnected = 0;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
cJSON *testValue = NULL;
cJSON *json = NULL;
volatile uint8_t json_data_recieved = 0;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;
#endif

#if WEBSOCKET_ENABLE
static const char *TAG = "Web-Socket";

esp_websocket_client_handle_t client;
static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;

volatile uint8_t sus_websocket = 0;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if WEBSOCKET_ENABLE
    static void log_error_if_nonzero(const char *message, int error_code)
    {
        if (error_code != 0)
        {
            ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
        }
    }

    static void shutdown_signaler(TimerHandle_t xTimer)
    {
        ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
        xSemaphoreGive(shutdown_sema);
    }

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
    static void get_string(char *line, size_t size)
    {
        int count = 0;
        while (count < size)
        {
            int c = fgetc(stdin);
            if (c == '\n')
            {
                line[count] = '\0';
                break;
            }
            else if (c > 0 && c < 127)
            {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

#endif /* CONFIG_WEBSOCKET_URI_FROM_STDIN */

    static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
    {
        esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
        switch (event_id)
        {
        case WEBSOCKET_EVENT_BEGIN:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
            break;
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            log_error_if_nonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
            if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
            {
                log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
            }
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
            ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
            if (data->op_code == 0x2)
            { // Opcode 0x2 indicates binary data
                ESP_LOG_BUFFER_HEX("Received binary data", data->data_ptr, data->data_len);
            }
            else if (data->op_code == 0x0A)
            {
                ESP_LOGI(TAG, "Received ping message");
            }
            else if (data->op_code == 0x08 && data->data_len == 2)
            {
                ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
            }
            else
            {
                ESP_LOGW(TAG, "Received=%.*s\n\n", data->data_len, (char *)data->data_ptr);
            }
            // ESP_LOGI("Web-Socket", "Processing WebSocket event");

            // JSON parsing example
            json = cJSON_Parse(data->data_ptr);
            if (!json)
            {
                ESP_LOGE("Web-Socket", "Failed to parse JSON");
                return;
            }
            else
            {
                json_data_recieved = 1;
            }

            // Validate JSON content
            testValue = cJSON_GetObjectItem(json, "Test");
            
            ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

            xTimerReset(shutdown_signal_timer, portMAX_DELAY);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
            log_error_if_nonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
            if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
            {
                log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
            }
            break;
        case WEBSOCKET_EVENT_FINISH:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
            break;
        }
    }

    // To suspend the esp_websocket_register_events function
    void suspend_websocket_events(void)
    {
        xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    }

    // To resume the esp_websocket_register_events function
    void resume_websocket_events(void)
    {
        xSemaphoreGive(shutdown_sema);
    }

    void deinit_websocket(void)
    {
        xSemaphoreTake(shutdown_sema, portMAX_DELAY);
        esp_websocket_client_close(client, portMAX_DELAY);
        ESP_LOGI(TAG, "Websocket Stopped");
        esp_websocket_client_destroy(client);
    }

    void init_websocket(void)
    {
        esp_websocket_client_config_t websocket_cfg = {};

        shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
                                             pdFALSE, NULL, shutdown_signaler);
        shutdown_sema = xSemaphoreCreateBinary();

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
        char line[128];

        ESP_LOGI(TAG, "Please enter uri of websocket endpoint");
        get_string(line, sizeof(line));

        websocket_cfg.uri = line;
        ESP_LOGI(TAG, "Endpoint uri: %s\n", line);

#else
        websocket_cfg.uri = CONFIG_WEBSOCKET_URI;
#endif /* CONFIG_WEBSOCKET_URI_FROM_STDIN */

#if CONFIG_WS_OVER_TLS_MUTUAL_AUTH
        /* Configuring client certificates for mutual authentification */
        extern const char cacert_start[] asm("_binary_ca_cert_pem_start");   // CA certificate
        extern const char cert_start[] asm("_binary_client_cert_pem_start"); // Client certificate
        extern const char cert_end[] asm("_binary_client_cert_pem_end");
        extern const char key_start[] asm("_binary_client_key_pem_start"); // Client private key
        extern const char key_end[] asm("_binary_client_key_pem_end");

        websocket_cfg.cert_pem = cacert_start;
        websocket_cfg.client_cert = cert_start;
        websocket_cfg.client_cert_len = cert_end - cert_start;
        websocket_cfg.client_key = key_start;
        websocket_cfg.client_key_len = key_end - key_start;
#elif CONFIG_WS_OVER_TLS_SERVER_AUTH
        // Using certificate bundle as default server certificate source
        websocket_cfg.crt_bundle_attach = esp_crt_bundle_attach;
        // If using a custom certificate it could be added to certificate bundle, added to the build similar to client certificates in this examples,
        // or read from NVS.
        /* extern const char cacert_start[] asm("ADDED_CERTIFICATE"); */
        /* websocket_cfg.cert_pem = cacert_start; */
#endif

#if CONFIG_WS_OVER_TLS_SKIP_COMMON_NAME_CHECK
        websocket_cfg.skip_cert_common_name_check = true;
#endif

        ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

        client = esp_websocket_client_init(&websocket_cfg);
        esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

        esp_websocket_client_start(client);
        xTimerStart(shutdown_signal_timer, portMAX_DELAY);

#if 0
        char data[32];
        int i = 0;
        while (i < 5)
        {
            if (esp_websocket_client_is_connected(client))
            {
                int len = sprintf(data, "hello %04d", i++);
                ESP_LOGI(TAG, "Sending %s", data);
                esp_websocket_client_send_text(client, data, len, portMAX_DELAY);
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // Sending text data
        ESP_LOGI(TAG, "Sending fragmented text message");
        memset(data, 'a', sizeof(data));
        esp_websocket_client_send_text_partial(client, data, sizeof(data), portMAX_DELAY);
        memset(data, 'b', sizeof(data));
        esp_websocket_client_send_cont_msg(client, data, sizeof(data), portMAX_DELAY);
        esp_websocket_client_send_fin(client, portMAX_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Sending binary data
        ESP_LOGI(TAG, "Sending fragmented binary message");
        char binary_data[5];
        memset(binary_data, 0, sizeof(binary_data));
        esp_websocket_client_send_bin_partial(client, binary_data, sizeof(binary_data), portMAX_DELAY);
        memset(binary_data, 1, sizeof(binary_data));
        esp_websocket_client_send_cont_msg(client, binary_data, sizeof(binary_data), portMAX_DELAY);
        esp_websocket_client_send_fin(client, portMAX_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Sending text data longer than ws buffer (default 1024)
        ESP_LOGI(TAG, "Sending text longer than ws buffer (default 1024)");
        const int size = 2000;
        char *long_data = malloc(size);
        memset(long_data, 'a', size);
        esp_websocket_client_send_text(client, long_data, size, portMAX_DELAY);
        free(long_data);

        deinit_websocket();
#endif
    }
#endif

#if WIFI_ENABLE
    static void event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            // if(!sus_websocket)
            // {
            //     sus_websocket = 1;
            //     suspend_websocket_events();
            // }
            isWifiConnected = 0;
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
            {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG1, "retry to connect to the AP");
            }
            else
            {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            ESP_LOGI(TAG1, "connect to the AP fail");
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG1, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            isWifiConnected = 1;

            // if (sus_websocket)
            // {
            //     sus_websocket = 0;
            //     resume_websocket_events();
            // }
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }

    void connect_wifi(void)
    {
        ESP_LOGI(TAG1, "Wi-Fi Initializing.");

        s_wifi_event_group = xEventGroupCreate();

        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_got_ip));

        wifi_config_t wifi_config = {
            .sta = {
                .ssid = EXAMPLE_ESP_WIFI_SSID,
                .password = EXAMPLE_ESP_WIFI_PASS,
                /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
                 * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
                 * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
                 * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
                 */
                .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
                .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
                .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG1,"wifi_init_sta finished.");

        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
         * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
         * happened. */
        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG1, "connected to ap SSID:%s password:%s",
                     EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            ESP_LOGI(TAG1, "Failed to connect to SSID:%s, password:%s",
                     EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        }
        else
        {
            ESP_LOGE(TAG1, "UNEXPECTED EVENT");
        }
    }
#endif

#ifdef __cplusplus
}
#endif