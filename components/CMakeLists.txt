idf_component_register(SRCS 
                            "src/wifi_connect.c"
                            "src/ppp_connect.c"
                            "src/connect.c"
                            "src/check_connection.c"
                            "src/esp_websocket_client.c"
                            "src/evse_websocket.c"
                            "src/lcd20x4.c"
                            "src/Relay.c"
                            "src/led_strip_encoder.c"4 
                            "src/led_strip.c"
                            "src/evse_em4m.c"
                            "src/MCP23017.c"
                            "src/GFCI.c"
                        
                    INCLUDE_DIRS "."
                    "inc"
                #     "inc/rc522"
                    "src"
                #     "src/rc522"
                # mcp23x17
                    REQUIRES
                    mcp23x17
                    "esp_netif"
                    "esp_system"
                    "esp_wifi"
                    "esp_event"
                    "nvs_flash"
                    "esp_eth"
                    "esp_driver_gpio"
                    "esp_driver_uart"
                    "mbedtls"
                    "wpa_supplicant"
                    "tcp_transport"
                    "http_parser"
                    "json"
                    "driver"   
)
