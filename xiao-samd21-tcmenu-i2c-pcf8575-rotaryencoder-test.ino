#include "xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test_menu.h"

#include <Wire.h> // For I2C communication
#include <IoAbstraction.h> // For I/O abstraction, including I2C expanders
#include <tcMenu.h> // The tcMenu library

#include "xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test_menu.h"

bool    myLEDState = true; // seeed xiao samd21 has led wired inverted. So its true for LED off.

#define SDA_PIN 4	// aaa xiao expansion board default I2C pin
#define SCL_PIN 5	// aaa xiao expansion board default I2C pin


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, myLEDState);  // turn the LED on (HIGH is the voltage level)


	Wire.begin(SDA_PIN, SCL_PIN); // aaa


    setupMenu();

    //interrupts(); // Re-enable interrupts in irq handler? From Github adafruit_PCF8575 pcf8575_buttonledirq.ino test.

}

void loop() {
    taskManager.runLoop();

}


void CALLBACK_FUNCTION onChangeTcmMyLED(int id) {
    // TODO - your menu change code
    myLEDState = !myLEDState;  // toggle myLED
    digitalWrite(LED_BUILTIN, myLEDState); // update LED hardware.
}

