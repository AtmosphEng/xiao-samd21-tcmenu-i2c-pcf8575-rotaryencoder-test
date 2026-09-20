#include <Arduino.h>
#include <Wire.h>
#include <IoAbstraction.h>
#include <tcMenu.h>
#include "i2cM5Stack8encoder.h"
#include "generated/xiao-samd21-tcmenu-i2c-8encoder-test_menu.h"

namespace {

constexpr uint8_t M5_STACK_8ENCODER_ADDRESS = 0x41;
constexpr uint8_t M5_STACK_8ENCODER_CH1 = 0;
constexpr uint8_t M5_STACK_8ENCODER_VALUE_REGISTER = 0x00;
constexpr uint8_t M5_STACK_8ENCODER_INCREMENT_REGISTER = 0x20;
constexpr uint8_t M5_STACK_8ENCODER_BUTTON_REGISTER = 0x50;
constexpr int32_t M5_STACK_8ENCODER_SUBSTEPS_PER_DETENT = 2;
constexpr uint32_t M5_STACK_8ENCODER_HOLD_TIME_MS = 400;
constexpr uint32_t M5_STACK_8ENCODER_POLL_INTERVAL_MS = 100;

class M5Stack8EncoderDevice {
public:
    bool read(uint8_t reg, uint8_t* data, size_t length) {
        Wire.beginTransmission(M5_STACK_8ENCODER_ADDRESS);
        Wire.write(reg);
        if (Wire.endTransmission() != 0) {
            return false;
        }

        Wire.requestFrom(M5_STACK_8ENCODER_ADDRESS, static_cast<uint8_t>(length));
        if (Wire.available() < static_cast<int>(length)) {
            return false;
        }

        for (size_t index = 0; index < length; ++index) {
            data[index] = Wire.read();
        }
        return true;
    }

    bool readEncoderIncrement(int32_t& value) {
        uint8_t data[4];
        if (!read(M5_STACK_8ENCODER_INCREMENT_REGISTER + M5_STACK_8ENCODER_CH1 * 4, data, sizeof(data))) {
            return false;
        }

        value = static_cast<int32_t>(static_cast<uint32_t>(data[0])
                | (static_cast<uint32_t>(data[1]) << 8)
                | (static_cast<uint32_t>(data[2]) << 16)
                | (static_cast<uint32_t>(data[3]) << 24));
        return true;
    }

    bool readEncoderValue(int32_t& value) {
        uint8_t data[4];
        if (!read(M5_STACK_8ENCODER_VALUE_REGISTER + M5_STACK_8ENCODER_CH1 * 4, data, sizeof(data))) {
            return false;
        }

        value = static_cast<int32_t>(static_cast<uint32_t>(data[0])
                | (static_cast<uint32_t>(data[1]) << 8)
                | (static_cast<uint32_t>(data[2]) << 16)
                | (static_cast<uint32_t>(data[3]) << 24));
        return true;
    }

    bool pollCh1() {
        uint8_t value = 0;
        int32_t encoderValue = 0;
        bool pressed = false;
        if (!read(M5_STACK_8ENCODER_BUTTON_REGISTER + M5_STACK_8ENCODER_CH1, &value, 1)
                || !readEncoderValue(encoderValue)) {
            return false;
        }
        pressed = value == 0;
        if (!haveEncoderValue) {
            lastEncoderValue = encoderValue;
            haveEncoderValue = true;
        } else {
            pendingIncrement += encoderValue - lastEncoderValue;
            lastEncoderValue = encoderValue;
        }

        if (pressed && !buttonDown) {
            buttonDown = true;
            buttonHeld = false;
            buttonStartedAt = millis();
        } else if (pressed && !buttonHeld
                   && millis() - buttonStartedAt >= M5_STACK_8ENCODER_HOLD_TIME_MS) {
            buttonHeld = true;
        } else if (!pressed && buttonDown) {
            if (buttonHeld) {
                pendingBack = true;
            } else {
                pendingSelect = true;
            }
            buttonDown = false;
            buttonHeld = false;
        }
        return true;
    }

    int32_t takeIncrement() {
        int32_t value = pendingIncrement;
        pendingIncrement = 0;
        return value;
    }

    bool processPendingButtonAction() {
        if (pendingBack) {
            pendingBack = false;
            if (menuMgr.getCurrentEditor() != nullptr
                    || menuMgr.getCurrentMenu() != menuMgr.getRoot()) {
                menuMgr.performDirectionMove(true);
            }
            return true;
        } else if (pendingSelect) {
            pendingSelect = false;
            menuMgr.onMenuSelect(false);
        }
        return false;
    }

private:
    bool buttonDown = false;
    bool buttonHeld = false;
    bool pendingBack = false;
    bool pendingSelect = false;
    uint32_t buttonStartedAt = 0;
    int32_t pendingIncrement = 0;
    int32_t lastEncoderValue = 0;
    bool haveEncoderValue = false;
};

class M5Stack8EncoderRotary : public RotaryEncoder {
public:
    explicit M5Stack8EncoderRotary(M5Stack8EncoderDevice& device)
            : RotaryEncoder([](int value) { menuMgr.valueChanged(value); }), device(device) {
    }

    bool begin() {
        return true;
    }

    void encoderChanged() override {
        pendingSubsteps += device.takeIncrement();
        if (pendingSubsteps >= M5_STACK_8ENCODER_SUBSTEPS_PER_DETENT) {
            increment(1);
            pendingSubsteps -= M5_STACK_8ENCODER_SUBSTEPS_PER_DETENT;
        } else if (pendingSubsteps <= -M5_STACK_8ENCODER_SUBSTEPS_PER_DETENT) {
            increment(-1);
            pendingSubsteps += M5_STACK_8ENCODER_SUBSTEPS_PER_DETENT;
        }
    }

private:
    M5Stack8EncoderDevice& device;
    int32_t pendingSubsteps = 0;
};

M5Stack8EncoderDevice encoderDevice;
M5Stack8EncoderRotary encoder(encoderDevice);

void processM5Stack8EncoderButtonAction() {
    if (encoderDevice.processPendingButtonAction()) {
        encoder.setUserIntention(SCROLL_THROUGH_ITEMS);
        menuMgr.setItemActive(menuMgr.getCurrentMenu());
    }
}

}

void serviceM5Stack8Encoder() {
    static uint32_t nextPoll = 0;
    uint32_t now = millis();
    if (static_cast<int32_t>(now - nextPoll) >= 0) {
        nextPoll = now + M5_STACK_8ENCODER_POLL_INTERVAL_MS;
        encoderDevice.pollCh1();
        encoder.encoderChanged();
    }
    processM5Stack8EncoderButtonAction();
}

void setupM5Stack8Encoder() {
    if (!encoder.begin()) {
        return;
    }

    switches.resetAllSwitches();
    switches.init(ioUsingArduino(), SWITCHES_NO_POLLING, true);
    switches.setEncoder(0, &encoder);
    menuMgr.changeMenu();
}