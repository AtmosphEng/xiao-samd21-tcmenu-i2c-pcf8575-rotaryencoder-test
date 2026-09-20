#include <Arduino.h>
#include <Wire.h>
#include <IoAbstraction.h>
#include <tcMenu.h>
#include "generated/xiao-samd21-tcmenu-i2c-8encoder-test_menu.h"

namespace {

constexpr uint8_t M5_STACK_8ENCODER_ADDRESS = 0x41;
constexpr uint8_t M5_STACK_8ENCODER_CH1 = 0;
constexpr uint8_t M5_STACK_8ENCODER_VALUE_REGISTER = 0x00;
constexpr uint8_t M5_STACK_8ENCODER_BUTTON_REGISTER = 0x50;
constexpr int32_t M5_STACK_8ENCODER_RAW_COUNTS_PER_CLICK = 2;
constexpr uint32_t M5_STACK_8ENCODER_HOLD_TIME_MS = 400;
constexpr uint32_t M5_STACK_8ENCODER_POLL_INTERVAL_MS = 20;

class M5Stack8EncoderDevice {
public:
    bool read(uint8_t reg, uint8_t* data, size_t length) {
        Wire.beginTransmission(M5_STACK_8ENCODER_ADDRESS);
        Wire.write(reg);
        if (Wire.endTransmission() != 0) {
            return false;
        }

        if (Wire.requestFrom(M5_STACK_8ENCODER_ADDRESS, static_cast<uint8_t>(length)) != length) {
            return false;
        }

        for (size_t index = 0; index < length; ++index) {
            data[index] = Wire.read();
        }
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

    bool readButton(bool& pressed) {
        uint8_t value = 0;
        if (!read(M5_STACK_8ENCODER_BUTTON_REGISTER + M5_STACK_8ENCODER_CH1, &value, 1)) {
            return false;
        }
        pressed = value == 0;
        return true;
    }

    bool pollButton() {
        bool pressed = false;
        if (!readButton(pressed)) {
            return true;
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
};

class M5Stack8EncoderIo : public BasicIoAbstraction {
public:
    explicit M5Stack8EncoderIo(M5Stack8EncoderDevice& device) : device(device) {}

    void pinDirection(pinid_t, uint8_t) override {}

    uint8_t readValue(pinid_t) override { return LOW; }

    bool runLoop() override {
        return device.pollButton();
    }

private:
    M5Stack8EncoderDevice& device;
};

class M5Stack8EncoderRotary : public RotaryEncoder {
public:
    explicit M5Stack8EncoderRotary(M5Stack8EncoderDevice& device)
            : RotaryEncoder([](int value) { menuMgr.valueChanged(value); }), device(device), lastValue(0), pendingDelta(0) {
        setUserIntention(DIRECTION_ONLY);
    }

    bool begin() {
        if (!device.readEncoderValue(lastValue)) {
            return false;
        }
        return true;
    }

    void encoderChanged() override {
        int32_t currentValue = 0;
        if (!device.readEncoderValue(currentValue)) {
            return;
        }

        int32_t delta = currentValue - lastValue;
        lastValue = currentValue;
        pendingDelta += delta;

        if (pendingDelta >= M5_STACK_8ENCODER_RAW_COUNTS_PER_CLICK) {
            increment(1);
            pendingDelta -= M5_STACK_8ENCODER_RAW_COUNTS_PER_CLICK;
        } else if (pendingDelta <= -M5_STACK_8ENCODER_RAW_COUNTS_PER_CLICK) {
            increment(-1);
            pendingDelta += M5_STACK_8ENCODER_RAW_COUNTS_PER_CLICK;
        }
    }

private:
    M5Stack8EncoderDevice& device;
    int32_t lastValue;
    int32_t pendingDelta;
};

M5Stack8EncoderDevice encoderDevice;
M5Stack8EncoderIo encoderIo(encoderDevice);
M5Stack8EncoderRotary encoder(encoderDevice);

void processM5Stack8EncoderButtonAction() {
    if (encoderDevice.processPendingButtonAction()) {
        encoder.setUserIntention(SCROLL_THROUGH_ITEMS);
        menuMgr.setItemActive(menuMgr.getCurrentMenu());
    }
}

}

void serviceM5Stack8Encoder() {
    processM5Stack8EncoderButtonAction();
}

void setupM5Stack8Encoder() {
    if (!encoder.begin()) {
        return;
    }

    taskManager.reset();
    switches.resetAllSwitches();
    switches.init(&encoderIo, SWITCHES_NO_POLLING, true);
    switches.setEncoder(0, &encoder);
    taskManager.scheduleFixedRate(M5_STACK_8ENCODER_POLL_INTERVAL_MS, [] {
        switches.runLoop();
        encoder.encoderChanged();
    });
    renderer.initialise();
}