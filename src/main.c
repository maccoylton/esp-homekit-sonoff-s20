/*
 * Example of using esp-homekit library to control
 * a a  Sonoff S20 using HomeKit.
 * The esp-wifi-config library is also used in this
 * example. This makes us of an OAT mechanis created by HomeACessoryKid
 * In order to flash the sonoff s20 you will have to
 * have a 3,3v (logic level) FTDI adapter.
 *
 * To flash this example connect 3,3v, TX, RX, GND
 * in this order, beginning in the (square) pin header
 * next to the button.
 * Next hold down the button and connect the FTDI adapter
 * to your computer. The sonoff is now in flash mode and
 * you can flash the custom firmware.
 *
 * WARNING: Do not connect the sonoff to AC while it's
 * connected to the FTDI adapter! This may fry your
 * computer and sonoff.
 *
 */

#define DEVICE_MANUFACTURER "itead"
#define DEVICE_NAME "Sonoff"
#define DEVICE_MODEL "S20"
#define DEVICE_SERIAL "12345678"
#define FW_VERSION "1.0"

#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include <adv_button.h>
#include <led_codes.h>
#include <udplogger.h>
#include <custom_characteristics.h>
#include <shared_functions.h>

// add this section to make your device OTA capable
// create the extra characteristic &ota_trigger, at the end of the primary service (before the NULL)
// it can be used in Eve, which will show it, where Home does not
// and apply the four other parameters in the accessories_information section

#include <ota-api.h>

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);

homekit_characteristic_t wifi_reset   = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_RESET, false, .setter=wifi_reset_set);
homekit_characteristic_t ota_trigger  = API_OTA_TRIGGER;
homekit_characteristic_t name         = HOMEKIT_CHARACTERISTIC_(NAME, DEVICE_NAME);
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER,  DEVICE_MANUFACTURER);
homekit_characteristic_t serial       = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, DEVICE_SERIAL);
homekit_characteristic_t model        = HOMEKIT_CHARACTERISTIC_(MODEL,         DEVICE_MODEL);
homekit_characteristic_t revision     = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION,  FW_VERSION);
homekit_characteristic_t switch_on = HOMEKIT_CHARACTERISTIC_(
                                                             ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)
                                                             );


// The GPIO pin that is connected to the relay and the blue LED on the Sonoff S20.
const int relay_gpio = 12;
// The GPIO pin that is connected to the green LED on the Sonoff S20.
const int LED_GPIO = 13;
// The GPIO pin that is oconnected to the button on the Sonoff S20.
const int button_gpio = 0;

int led_off_value=1; /* global varibale to support LEDs set to 0 where the LED is connected to GND, 1 where +3.3v */
const int status_led_gpio = 13; /*set the gloabl variable for the led to be sued for showing status */



void button_single_press_callback(uint8_t gpio, void* args, const uint8_t param) {
    
    printf("button_single_press_callback:Button %d, Toggling relay on gpio %d\n", gpio, relay_gpio);
    switch_on.value.bool_value = !switch_on.value.bool_value;
    relay_write(switch_on.value.bool_value, relay_gpio);
    
}


void gpio_init() {

    adv_button_set_evaluate_delay(10);
    printf("Initialising buttons\n");
    adv_button_create(button_gpio, true, false);
    adv_button_register_callback_fn(button_gpio, button_single_press_callback, SINGLEPRESS_TYPE, NULL, 0);
    adv_button_register_callback_fn(button_gpio, reset_button_callback, VERYLONGPRESS_TYPE, NULL, 0);
    
    gpio_enable(LED_GPIO, GPIO_OUTPUT);
    led_write(false, LED_GPIO);
    gpio_enable(relay_gpio, GPIO_OUTPUT);
    relay_write(switch_on.value.bool_value, relay_gpio);
}

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    printf("switch_on_callback: Homekit etting Relay to %s\n", switch_on.value.bool_value ? "true" : "false");
    relay_write(switch_on.value.bool_value, relay_gpio);
}



homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            &manufacturer,
            &serial,
            &model,
            &revision,
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Switch"),
            &switch_on,
            &ota_trigger,
            &wifi_reset,
            NULL
        }),
        NULL
    }),
    NULL
};

void accessory_init (void ){
    /* initalise anything you don't want started until wifi and pairing is confirmed */
    
}


homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};


void user_init(void) {

    standard_init (&name, &manufacturer, &model, &serial, &revision);
   
    gpio_init();

    wifi_config_init("SonoffS20", NULL, on_wifi_ready);

 
}
