#include "generated/xiao-samd21-tcmenu-i2c-8encoder-test_menu.h"
#include "i2cM5Stack8encoder.h"

#define DEF_USE_8ENCODER true

//
// xiao-samd21-tcmenu-i2c-pcf8575-rotaryencoder-test.ino
// NOTE I2C expander changed from pcf8575 to mcp23017 for tcmenu support.
// I2C uses SCL=5, SDA=4, MSC23017 on i2c address 0x20 and using interrupt 2. Defined in tcmDesigner.


#include <Wire.h> // For I2C communication
#include <IoAbstraction.h> // For I/O abstraction, including I2C expanders
#include <tcMenu.h> // The tcMenu library


bool    myLEDState = true; // seeed xiao samd21 has led wired inverted. So its true for LED off.

#define PIN_XIAO_EXP_BUZZER 3

// CHOOSE ONE MCU FROM THE LIST BELOW
#define MCU_SAMD21
//#define MCU_ESP32_C3

// NOTE - I2C PINS SDA AND SCL ARE ALSO DEFINED IN TCMENUDESGNER **********************************
#if defined (MCU_SAMD21)
#define SDA_PIN 4 // also defined in tcmenuDesigner
#define SCL_PIN 5 // also defined in tcmenuDesigner
// NOTE - interrupt pin 2 and MSP23018 i2c addr of 0x20 defined in tcmenuDesigner
//#define LED_GROVE 0 // Note for samd21 - A0/D0 is GPIO0
#elif defined (MCU_ESP32_C3)
#define SDA_PIN 6
#define SCL_PIN 7
//#define LED_GROVE 2 // Note for esp32-c3 - A0/D0 is GPIO2
#else
#define SDA_PIN 4	// xiao expansion board default I2C pin
#define SCL_PIN 5	// xiao expansion board default I2C pin
#endif


#define LED_DEBUG 0 // for test
//aaa #define LED_DEBUG 10 // for test

#define LED_BUILTIN 13 // for test compile w. XIAO_ESP32C3. NOTE XIAO_ESP32C3 does NOT have any LED_BUILTIN fitted!

void setup() {

    Serial.begin(115200);
    //while(!Serial); // this can hang the samd21 until a Serial Monitor connection is made
    delay(1000);
    Serial.println("xiao-samd21-tcmenu-i2c-rotaryencoder-test.ino");

    pinMode(PIN_XIAO_EXP_BUZZER, OUTPUT);
    pinMode(LED_DEBUG, OUTPUT);

    digitalWrite(LED_DEBUG, myLEDState);
    delay(500);
    digitalWrite(LED_DEBUG, !myLEDState);
    delay(500);
    digitalWrite(LED_DEBUG, myLEDState);
    delay(500);
    digitalWrite(LED_DEBUG, !myLEDState);

#ifdef LED_GROVE
    pinMode(LED_GROVE, OUTPUT);
    digitalWrite(LED_GROVE, myLEDState);  // turn the LED on (HIGH is the voltage level)
#endif

	//Wire.begin(SDA_PIN, SCL_PIN); // this enables the tcmenuDesigner display to render for SSD1306 128x64 SW I2C
	Wire.begin(); // this enables the tcmenuDesigner display to render for SSD1306 128x64 HW I2C. Also interrupts ok.

    setupMenu();

#if DEF_USE_8ENCODER
    setupM5Stack8Encoder();
#endif

	taskManager.scheduleFixedRate(1000, [] { // ms

        menuTcmUpSeconds.setCurrentValue(menuTcmUpSeconds.getCurrentValue() + 1); // increment seconds to show tcmenu run.
#ifdef LED_GROVE
        digitalWrite(LED_GROVE, myLEDState); // update LED hardware.
#endif

	}); // taskManager


} // setup

void loop() {
    taskManager.runLoop();
#if DEF_USE_8ENCODER
    serviceM5Stack8Encoder();
#endif

} // loop


void CALLBACK_FUNCTION onChangeTcmMyLED(int id) {
    digitalWrite(LED_DEBUG, myLEDState); // update LED hardware.
    Serial.println("in onChangeTcmMyLED()");
    myLEDState = !myLEDState;  // toggle myLED
   
    if (menuTcmMyLED.getCurrentValue()) {
        tone(PIN_XIAO_EXP_BUZZER, 1000, 500); // Generate a tone (frequency of 1000 Hz) for 500 milliseconds
    }
}



void CALLBACK_FUNCTION onChangeTcmUpSeconds(int id) {
    Serial.print("in onChangeTcmUpSeconds()     ");
    Serial.println(menuTcmUpSeconds.getCurrentValue());
}
