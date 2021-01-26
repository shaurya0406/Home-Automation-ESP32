/**********************************************************************
* - Description:		esp32-homekit
* - File:				led.c
* - Compiler:			xtensa-esp32
* - Debugger:			USB2USART
* - Author:				Shaurya Chandra
* - Target:				ESP32
* - Created:			23-11-2019
* - Last changed:		26-01-2021
*
**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/gpio.h>

#include <driver/gpio.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "wifi.h"
#include "iot_button.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

static const char *TAG = "app";

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;


#define SERV_NAME_PREFIX    "PROV_"
const int led_gpio = 23;
const int relay0_gpio = 2;
const int relay1_gpio = 14;
const int relay2_gpio = 27;
const int button_gpio0 = 0;
const int button_gpio1 = 5;
const int button_gpio2 = 12;
bool led_on = false;
bool relay0_on = false;
bool relay1_on = false;
bool relay2_on = false;
static bool g_output_state;

void on_wifi_ready();

void led_write(bool on) {
    gpio_set_level(led_gpio, on ? 1 : 0);
}
void relay_write(int relay, bool on) {
    gpio_set_level(relay, on ? 1 : 0);
}

void led_init() {
    gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(relay0_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(relay1_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(relay2_gpio, GPIO_MODE_OUTPUT);
    led_write(led_on);
    relay_write(relay0_gpio, relay0_on);
    relay_write(relay1_gpio, relay1_on);
    relay_write(relay2_gpio, relay2_on);
}



void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_write(led_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(led_identify_task, "LED identify", 512, NULL, 2, NULL);
}
homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}
homekit_value_t relay0_on_get() {
    return HOMEKIT_BOOL(relay0_on);
}
homekit_value_t relay1_on_get() {
    return HOMEKIT_BOOL(relay1_on);
}
homekit_value_t relay2_on_get() {
    return HOMEKIT_BOOL(relay2_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_on = value.bool_value;
    led_write(led_on);
}
void relay0_on_set(homekit_value_t value){
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }
    printf("relay0\n");
    printf("%d",value.bool_value);
    relay0_on=value.bool_value;
    relay_write(relay0_gpio,relay0_on);
}
void relay1_on_set(homekit_value_t value){
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }
    printf("relay1\n");
    //printf("%d",value.bool_value);
    relay1_on=value.bool_value;
    relay_write(relay1_gpio,relay1_on);
}
void relay2_on_set(homekit_value_t value){
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }
    printf("relay2\n");
    //printf("%d",value.bool_value);
    relay2_on=value.bool_value;
    relay_write(relay2_gpio,relay2_on);
}
homekit_characteristic_t relay0_state = HOMEKIT_CHARACTERISTIC_(
    ON, false, .getter=relay0_on_get,
                .setter=relay0_on_set
);
homekit_characteristic_t relay1_state = HOMEKIT_CHARACTERISTIC_(
    ON, false, .getter=relay1_on_get,
                .setter=relay1_on_set
);
homekit_characteristic_t relay2_state = HOMEKIT_CHARACTERISTIC_(
    ON, false, .getter=relay2_on_get,
                .setter=relay2_on_set
);

void reset_configuration_task() {
    //Flash the LED first before we start the reset
    // for (int i=0; i<3; i++) {
    //     led_write(true);
    //     vTaskDelay(100 / portTICK_PERIOD_MS);
    //     led_write(false);
    //     vTaskDelay(100 / portTICK_PERIOD_MS);
    // }
    
    printf("Resetting Wifi Config\n");
//    esp_err_t retrn = nvs_flash_erase();
//    if(retrn != ESP_OK)
//    {
//        printf("nvs flash not erased");
//        ESP_ERROR_CHECK(nvs_flash_erase());
//    }
    ESP_ERROR_CHECK(nvs_flash_deinit());
    
    ESP_ERROR_CHECK(nvs_flash_erase());
    
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    printf("Resetting HomeKit Config\n");
    
    homekit_server_reset();
     
    
    printf("Restarting\n");
    
    esp_restart();

}

void push_btn0_cb(void* arg)
{
    printf("Toggling relay\n");
    homekit_value_t value;
    value=relay0_on_get();
    value.bool_value = !value.bool_value;
    relay0_on_set(value);
    homekit_characteristic_notify(&relay0_state,value);
}
void push_btn1_cb(void* arg)
{
    printf("Toggling relay\n");
    homekit_value_t value;
    value=relay1_on_get();
    value.bool_value = !value.bool_value;
    relay1_on_set(value);
    homekit_characteristic_notify(&relay1_state,value);
}
void push_btn2_cb(void* arg)
{
    // app_driver_set_state(!g_output_state);
    printf("Toggling relay\n");
    homekit_value_t value;
    value=relay2_on_get();
    value.bool_value = !value.bool_value;
    relay2_on_set(value);
    homekit_characteristic_notify(&relay2_state,value);
}
void button_press_3sec_cb(void *arg)
{
    nvs_flash_erase();
    reset_configuration_task();
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    wifi_prov_mgr_event_handler(ctx, event);
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            printf("STA start\n");
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            printf("WiFI ready\n");
            on_wifi_ready();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            printf("STA disconnected\n");
            led_write(0);
            esp_wifi_connect();
            break;
        default:
            break;
    }
    return ESP_OK;
}



/* Event handler for catching system events */
static void prov_event_handler(void *user_data, wifi_prov_cb_event_t event, void *event_data)
{
    switch (event) {
           case WIFI_PROV_START:
               ESP_LOGI(TAG, "Provisioning started");
               break;
           case WIFI_PROV_CRED_RECV: {
               wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
               /* If SSID length is exactly 32 bytes, null termination
                * will not be present, so explicitly obtain the length */
               size_t ssid_len = strnlen((const char *)wifi_sta_cfg->ssid, sizeof(wifi_sta_cfg->ssid));
               ESP_LOGI(TAG, "Received Wi-Fi credentials"
                        "\n\tSSID     : %.*s\n\tPassword : %s",
                        ssid_len, (const char *) wifi_sta_cfg->ssid,
                        (const char *) wifi_sta_cfg->password);
               break;
           }
           case WIFI_PROV_CRED_FAIL: {
               wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
               ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                        "\n\tPlease reset to factory and retry provisioning",
                        (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                        "Wi-Fi AP password incorrect" : "Wi-Fi AP not found");
               break;
           }
           case WIFI_PROV_CRED_SUCCESS:
               ESP_LOGI(TAG, "Provisioning successful");
               break;
           case WIFI_PROV_END:
               /* De-initialize manager once provisioning is finished */
               wifi_prov_mgr_deinit();
               break;
           default:
               break;
       }
}

static void wifi_init_sta(void)
{
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                          uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf) {
        ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

static void wifi_init() {
    
//    /* Initialize NVS partition */
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            /* NVS partition was truncated
             * and needs to be erased */
            ESP_ERROR_CHECK(nvs_flash_erase());

            /* Retry nvs_flash_init */
            ESP_ERROR_CHECK(nvs_flash_init());
        }

        /* Initialize TCP/IP */
        ESP_ERROR_CHECK(esp_netif_init());

        /* Initialize the event loop */
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        wifi_event_group = xEventGroupCreate();

        /* Register our event handler for Wi-Fi, IP and Provisioning related events */
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

        /* Initialize Wi-Fi including netif with default config */
        esp_netif_create_default_wifi_sta();
        esp_netif_create_default_wifi_ap();
    
        
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        /* Configuration for the provisioning manager */
        wifi_prov_mgr_config_t config = {

            .scheme = wifi_prov_scheme_softap,

            .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
            
            .app_event_handler = {
                .event_cb = prov_event_handler,
                .user_data = NULL
            }
        };

            
        /* Initialize provisioning manager with the
         * configuration parameters set above */
        ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

//        bool provisioned = false;
//        /* Let's find out if the device is provisioned */
//        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

        bool provisioned;
        if (wifi_prov_mgr_is_provisioned(&provisioned) != ESP_OK) {
            ESP_LOGE(TAG, "Error getting device provisioning state");
            return;
        }
        /* If device is not yet provisioned start provisioning service */
        if (!provisioned) {
            ESP_LOGI(TAG, "Starting provisioning");

            /* What is the Device Service Name that we want
             * This translates to :
             *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
             *     - device name when scheme is wifi_prov_scheme_ble
             */
            char service_name[12];
            get_device_service_name(service_name, sizeof(service_name));

            /* What is the security level that we want (0 or 1):
             *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
             *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
             *          using X25519 key exchange and proof of possession (pop) and AES-CTR
             *          for encryption/decryption of messages.
             */
            wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

            /* Do we want a proof-of-possession (ignored if Security 0 is selected):
             *      - this should be a string with length > 0
             *      - NULL if not used
             */
            const char *pop = "abcd1234";

            /* What is the service key (could be NULL)
             * This translates to :
             *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
             *     - simply ignored when scheme is wifi_prov_scheme_ble
             */
            const char *service_key = NULL;

    

            /* An optional endpoint that applications can create if they expect to
             * get some additional custom data during provisioning workflow.
             * The endpoint name can be anything of your choice.
             * This call must be made before starting the provisioning.
             */
            wifi_prov_mgr_endpoint_create("custom-data");
            /* Start provisioning service */
            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));

            /* The handler for the optional endpoint created above.
             * This call must be made after starting the provisioning, and only if the endpoint
             * has already been created above.
             */
            wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);

        } else {
            ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

            /* We don't need the manager as device is already provisioned,
             * so let's release it's resources */
            wifi_prov_mgr_deinit();

            /* Start Wi-Fi station */
            wifi_init_sta();
        }

        /* Wait for Wi-Fi connection */
        //xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);

}
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "SHAURDUINO LED"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Shaurya Chandra"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP32"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Sample LED"),
            &relay0_state,
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Tubelight"),
            &relay1_state,
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Sodium Lamp"),
            &relay2_state,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "123-45-678",
    .setupId="1QJ8"
};

void on_wifi_ready() {
    led_write(1);
    homekit_server_init(&config);
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_init();
    led_init();
    button_handle_t button0=iot_button_create(0,0);
    iot_button_set_evt_cb(button0, BUTTON_CB_RELEASE,push_btn0_cb, "RELEASE");
    iot_button_add_on_press_cb(button0, 3, button_press_3sec_cb, NULL);
    button_handle_t button1=iot_button_create(5,0);
    iot_button_set_evt_cb(button1, BUTTON_CB_RELEASE,push_btn1_cb, "RELEASE");
    button_handle_t button2=iot_button_create(12,0);
    iot_button_set_evt_cb(button2, BUTTON_CB_RELEASE,push_btn2_cb, "RELEASE");
    
}
