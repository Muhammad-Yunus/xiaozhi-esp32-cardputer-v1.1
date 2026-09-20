#ifndef HC138_KEYBOARD_H
#define HC138_KEYBOARD_H

#include <functional>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// HID-compatible key codes (same as TCA8418 keyboard)
enum KeyCode {
    KC_NONE = 0x00,
    KC_A = 0x04,
    KC_B = 0x05,
    KC_C = 0x06,
    KC_D = 0x07,
    KC_E = 0x08,
    KC_F = 0x09,
    KC_G = 0x0A,
    KC_H = 0x0B,
    KC_I = 0x0C,
    KC_J = 0x0D,
    KC_K = 0x0E,
    KC_L = 0x0F,
    KC_M = 0x10,
    KC_N = 0x11,
    KC_O = 0x12,
    KC_P = 0x13,
    KC_Q = 0x14,
    KC_R = 0x15,
    KC_S = 0x16,
    KC_T = 0x17,
    KC_U = 0x18,
    KC_V = 0x19,
    KC_W = 0x1A,
    KC_X = 0x1B,
    KC_Y = 0x1C,
    KC_Z = 0x1D,
    KC_1 = 0x1E,
    KC_2 = 0x1F,
    KC_3 = 0x20,
    KC_4 = 0x21,
    KC_5 = 0x22,
    KC_6 = 0x23,
    KC_7 = 0x24,
    KC_8 = 0x25,
    KC_9 = 0x26,
    KC_0 = 0x27,
    KC_ENTER = 0x28,
    KC_ESC = 0x29,
    KC_BACKSPACE = 0x2A,
    KC_TAB = 0x2B,
    KC_SPACE = 0x2C,
    KC_MINUS = 0x2D,
    KC_EQUAL = 0x2E,
    KC_LBRACKET = 0x2F,
    KC_RBRACKET = 0x30,
    KC_BACKSLASH = 0x31,
    KC_SEMICOLON = 0x33,
    KC_APOSTROPHE = 0x34,
    KC_GRAVE = 0x35,
    KC_COMMA = 0x36,
    KC_DOT = 0x37,
    KC_SLASH = 0x38,
    KC_CAPSLOCK = 0x39,
    KC_RIGHT = 0x4F,
    KC_LEFT = 0x50,
    KC_DOWN = 0x51,
    KC_UP = 0x52,
    KC_LSHIFT = 0xE1,
    KC_LCTRL = 0xE0,
    KC_LALT = 0xE2,
    KC_LOPT = 0xE3,
};

// Key event structure
struct KeyEvent {
    bool pressed;
    bool is_modifier;
    uint8_t key_code;
    const char* key_char;
};

// Legacy key codes for backward compatibility
enum LegacyKeyCode {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_OTHER
};

// 74HC138 keyboard scanner for Cardputer v1.1
class Hc138Keyboard {
public:
    using KeyCallback = std::function<void(LegacyKeyCode key)>;
    using KeyEventCallback = std::function<void(const KeyEvent& event)>;

    Hc138Keyboard(gpio_num_t a0, gpio_num_t a1, gpio_num_t a2);
    ~Hc138Keyboard();

    void Initialize();
    void SetKeyCallback(KeyCallback callback) { key_callback_ = callback; }
    void SetKeyEventCallback(KeyEventCallback callback) { key_event_callback_ = callback; }

private:
    gpio_num_t a0_pin_;
    gpio_num_t a1_pin_;
    gpio_num_t a2_pin_;
    KeyCallback key_callback_;
    KeyEventCallback key_event_callback_;
    TaskHandle_t task_handle_ = nullptr;

    void SetDecoderOutput(uint8_t col);
    uint8_t ReadInputRow();

    static void ScanTask(void* arg);
};

#endif // HC138_KEYBOARD_H
