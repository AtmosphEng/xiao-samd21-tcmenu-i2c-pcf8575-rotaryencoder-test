//
// xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test.ino
// NOTE I2C expander changed from pcf8575 to mcp23017 for tcmenu support.

#include "xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test_menu.h"

#include <Wire.h> // For I2C communication
#include <IoAbstraction.h> // For I/O abstraction, including I2C expanders
#include <tcMenu.h> // The tcMenu library

#include "xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test_menu.h"

bool    myLEDState = true; // seeed xiao samd21 has led wired inverted. So its true for LED off.

#define PIN_XIAO_EXP_BUZZER 5 // esp32-c3 D3 is GPIO5

#define MCU_SAMD21
//#define MCU_ESP32_C3

// NOTE - I2C PINS SDA AND SCL ARE ALSO DEFINED IN TCMENUDESGNER **********************************
#if defined (MCU_SAMD21)
#define SDA_PIN 4 // also defined in tcmenuDesigner
#define SCL_PIN 5 // also defined in tcmenuDesigner
// interrupt pin 2 defined in tcmenuDesigner
#elif defined (MCU_ESP32_C3)
#define SDA_PIN 6
#define SCL_PIN 7
#else
#define SDA_PIN 4	// xiao expansion board default I2C pin
#define SCL_PIN 5	// xiao expansion board default I2C pin
#endif

//#define LED_GROVE 2 // Note for esp32-c3 - A0/D0 is GPIO2

#define LED_DEBUG 10 // for test

#define LED_BUILTIN 13 // for test compile w. XIAO_ESP32C3. NOTE XIAO_ESP32C3 does NOT have any LED_BUILTIN fitted!

void setup() {

    Serial.begin(115200);
    //while(!Serial); // this can hang the samd21 until a Serial Monitor connection is made
    delay(1000);
    Serial.println("xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test.ino");

    pinMode(LED_DEBUG, OUTPUT);
    digitalWrite(LED_DEBUG, myLEDState);  // turn the LED on (HIGH is the voltage level)
#ifdef LED_GROVE
    pinMode(LED_GROVE, OUTPUT);
    digitalWrite(LED_GROVE, myLEDState);  // turn the LED on (HIGH is the voltage level)
#endif

	//Wire.begin(SDA_PIN, SCL_PIN); // this enables the tcmenuDesigner display to render for SSD1306 128x64 SW I2C
	Wire.begin(); // this enables the tcmenuDesigner display to render for SSD1306 128x64 HW I2C. Also interrupts ok.

    setupMenu();

	taskManager.scheduleFixedRate(1000, [] { // ms

        menuTcmUpSeconds.setCurrentValue(menuTcmUpSeconds.getCurrentValue() + 1); // increment seconds to show tcmenu run.
#ifdef LED_GROVE
        digitalWrite(LED_GROVE, myLEDState); // update LED hardware.
#endif

	}); // taskManager


} // setup

void loop() {
    taskManager.runLoop();

} // loop


void CALLBACK_FUNCTION onChangeTcmMyLED(int id) {
    myLEDState = !myLEDState;  // toggle myLED
    digitalWrite(LED_DEBUG, myLEDState); // update LED hardware.
    Serial.println("in onChangeTcmMyLED()");
}



void CALLBACK_FUNCTION onChangeTcmUpSeconds(int id) {
    Serial.print("in onChangeTcmUpSeconds()     ");
    Serial.println(menuTcmUpSeconds.getCurrentValue());
}
