// M5Stack Cardputer v1.1 - 74HC138 keyboard scanner
// Based on M5Stack M5Cardputer-UserDemo keyboard implementation

#include "hc138_keyboard.h"
#include <esp_log.h>
#include <driver/gpio.h>

#define TAG "Hc138Keyboard"

struct KeyValue {
    const char* normal;
    uint8_t normal_code;
    const char* shifted;
    uint8_t shifted_code;
};

static const KeyValue KEY_MAP[4][14] = {
    // Row 0: `  1  2  3  4  5  6  7  8  9  0  -  =  Del
    {{"`", KC_GRAVE, "~", KC_GRAVE},
     {"1", KC_1, "!", KC_1},
     {"2", KC_2, "@", KC_2},
     {"3", KC_3, "#", KC_3},
     {"4", KC_4, "$", KC_4},
     {"5", KC_5, "%", KC_5},
     {"6", KC_6, "^", KC_6},
     {"7", KC_7, "&", KC_7},
     {"8", KC_8, "*", KC_8},
     {"9", KC_9, "(", KC_9},
     {"0", KC_0, ")", KC_0},
     {"-", KC_MINUS, "_", KC_MINUS},
     {"=", KC_EQUAL, "+", KC_EQUAL},
     {"", KC_BACKSPACE, "", KC_BACKSPACE}},
    // Row 1: Tab Q  W  E  R  T  Y  U  I  O  P  [  ]  Backslash
    {{"", KC_TAB, "", KC_TAB},
     {"q", KC_Q, "Q", KC_Q},
     {"w", KC_W, "W", KC_W},
     {"e", KC_E, "E", KC_E},
     {"r", KC_R, "R", KC_R},
     {"t", KC_T, "T", KC_T},
     {"y", KC_Y, "Y", KC_Y},
     {"u", KC_U, "U", KC_U},
     {"i", KC_I, "I", KC_I},
     {"o", KC_O, "O", KC_O},
     {"p", KC_P, "P", KC_P},
     {"[", KC_LBRACKET, "{", KC_LBRACKET},
     {"]", KC_RBRACKET, "}", KC_RBRACKET},
     {"\\", KC_BACKSLASH, "|", KC_BACKSLASH}},
    // Row 2: Shift CapsLk A  S  D  F  G  H  J  K  L  ;  '  Enter
    {{"", KC_LSHIFT, "", KC_LSHIFT},
     {"", KC_CAPSLOCK, "", KC_CAPSLOCK},
     {"a", KC_A, "A", KC_A},
     {"s", KC_S, "S", KC_S},
     {"d", KC_D, "D", KC_D},
     {"f", KC_F, "F", KC_F},
     {"g", KC_G, "G", KC_G},
     {"h", KC_H, "H", KC_H},
     {"j", KC_J, "J", KC_J},
     {"k", KC_K, "K", KC_K},
     {"l", KC_L, "L", KC_L},
     {";", KC_SEMICOLON, ":", KC_SEMICOLON},
     {"'", KC_APOSTROPHE, "\"", KC_APOSTROPHE},
     {"", KC_ENTER, "", KC_ENTER}},
    // Row 3: Ctrl Opt Alt Z  X  C  V  B  N  M  ,  .  /  Space
    {{"", KC_LCTRL, "", KC_LCTRL},
     {"", KC_LOPT, "", KC_LOPT},
     {"", KC_LALT, "", KC_LALT},
     {"z", KC_Z, "Z", KC_Z},
     {"x", KC_X, "X", KC_X},
     {"c", KC_C, "C", KC_C},
     {"v", KC_V, "V", KC_V},
     {"b", KC_B, "B", KC_B},
     {"n", KC_N, "N", KC_N},
     {"m", KC_M, "M", KC_M},
     {",", KC_COMMA, "<", KC_COMMA},
     {".", KC_DOT, ">", KC_DOT},
     {"/", KC_SLASH, "?", KC_SLASH},
     {" ", KC_SPACE, " ", KC_SPACE}}
};

struct ScanEntry {
    int decoder_col;
    int input_bit;
    int logical_row;
    int logical_col;
};

// Key scan mapping: (decoder_output, input_bit) -> (logical_row, logical_col)
// Based on M5Stack M5Cardputer-UserDemo keyboard implementation
static const ScanEntry KEY_SCAN_MAP[] = {
    // Decoder output 0 (column 0)
    {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 2, 2, 0}, {0, 3, 3, 0},
    // Decoder output 1 (column 1)
    {1, 0, 0, 1}, {1, 1, 1, 1}, {1, 2, 2, 1}, {1, 3, 3, 1},
    // Decoder output 2 (column 2)
    {2, 0, 0, 2}, {2, 1, 1, 2}, {2, 2, 2, 2}, {2, 3, 3, 2},
    // Decoder output 3 (column 3)
    {3, 0, 0, 3}, {3, 1, 1, 3}, {3, 2, 2, 3}, {3, 3, 3, 3},
    // Decoder output 4 (column 4)
    {4, 0, 0, 4}, {4, 1, 1, 4}, {4, 2, 2, 4}, {4, 3, 3, 4},
    // Decoder output 5 (column 5)
    {5, 0, 0, 5}, {5, 1, 1, 5}, {5, 2, 2, 5}, {5, 3, 3, 5},
    // Decoder output 6 (column 6)
    {6, 0, 0, 6}, {6, 1, 1, 6}, {6, 2, 2, 6}, {6, 3, 3, 6},
    // Decoder output 7 (column 7)
    {7, 0, 0, 7}, {7, 1, 1, 7}, {7, 2, 2, 7}, {7, 3, 3, 7},
};

// Right half mapping for columns 8-13
static const ScanEntry KEY_SCAN_MAP_RIGHT[] = {
    // Column 8
    {0, 4, 0, 8}, {0, 5, 0, 9}, {0, 6, 0, 10},
    // Column 9
    {1, 4, 0, 11}, {1, 5, 0, 12}, {1, 6, 0, 13},
    // Column 10
    {2, 4, 1, 8}, {2, 5, 1, 9}, {2, 6, 1, 10}, {2, 7, 1, 11},
    // Column 11
    {3, 4, 1, 12}, {3, 5, 1, 13},
    // Column 12
    {4, 4, 2, 8}, {4, 5, 2, 9}, {4, 6, 2, 10}, {4, 7, 2, 11},
    // Column 13
    {5, 4, 2, 12}, {5, 5, 2, 13},
    // Additional mappings
    {6, 4, 3, 8}, {6, 5, 3, 9}, {6, 6, 3, 10}, {6, 7, 3, 11},
    {7, 4, 3, 12},
};

Hc138Keyboard::Hc138Keyboard(gpio_num_t a0, gpio_num_t a1, gpio_num_t a2)
    : a0_pin_(a0), a1_pin_(a1), a2_pin_(a2) {}

Hc138Keyboard::~Hc138Keyboard() {
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

void Hc138Keyboard::Initialize() {
    ESP_LOGI(TAG, "Initializing 74HC138 keyboard");

    // Configure 74HC138 decoder output pins as outputs
    gpio_config_t out_conf = {};
    out_conf.mode = GPIO_MODE_OUTPUT;
    out_conf.pin_bit_mask = (1ULL << a0_pin_) | (1ULL << a1_pin_) | (1ULL << a2_pin_);
    gpio_config(&out_conf);
    SetDecoderOutput(0);

    // Configure input pins with internal pull-ups
    gpio_config_t in_conf = {};
    in_conf.mode = GPIO_MODE_INPUT;
    in_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    in_conf.pin_bit_mask = (1ULL << GPIO_NUM_7) | (1ULL << GPIO_NUM_6) | (1ULL << GPIO_NUM_5) |
                           (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_3) | (1ULL << GPIO_NUM_15) |
                           (1ULL << GPIO_NUM_13);
    gpio_config(&in_conf);

    // Create keyboard scan task
    xTaskCreate(ScanTask, "kb_scan", 4096, this, 5, &task_handle_);

    ESP_LOGI(TAG, "74HC138 keyboard initialized");
}

void Hc138Keyboard::SetDecoderOutput(uint8_t col) {
    col &= 0x07;
    gpio_set_level(a0_pin_, col & 0x01);
    gpio_set_level(a1_pin_, col & 0x02);
    gpio_set_level(a2_pin_, col & 0x04);
}

uint8_t Hc138Keyboard::ReadInputRow() {
    uint8_t value = 0;
    if (gpio_get_level(GPIO_NUM_7) == 0) value |= (1 << 0);
    if (gpio_get_level(GPIO_NUM_6) == 0) value |= (1 << 1);
    if (gpio_get_level(GPIO_NUM_5) == 0) value |= (1 << 2);
    if (gpio_get_level(GPIO_NUM_4) == 0) value |= (1 << 3);
    if (gpio_get_level(GPIO_NUM_3) == 0) value |= (1 << 4);
    if (gpio_get_level(GPIO_NUM_15) == 0) value |= (1 << 5);
    if (gpio_get_level(GPIO_NUM_13) == 0) value |= (1 << 6);
    return value;
}

void Hc138Keyboard::ScanTask(void* arg) {
    Hc138Keyboard* kb = static_cast<Hc138Keyboard*>(arg);

    constexpr int LEFT_COUNT = sizeof(KEY_SCAN_MAP) / sizeof(KEY_SCAN_MAP[0]);
    constexpr int RIGHT_COUNT = sizeof(KEY_SCAN_MAP_RIGHT) / sizeof(KEY_SCAN_MAP_RIGHT[0]);
    constexpr int TOTAL_COUNT = LEFT_COUNT + RIGHT_COUNT;

    ScanEntry all_entries[TOTAL_COUNT];
    memcpy(all_entries, KEY_SCAN_MAP, LEFT_COUNT * sizeof(ScanEntry));
    memcpy(all_entries + LEFT_COUNT, KEY_SCAN_MAP_RIGHT, RIGHT_COUNT * sizeof(ScanEntry));

    uint64_t prev_state = 0;

    while (true) {
        uint64_t current_state = 0;

        // Scan all 8 decoder outputs
        for (int col = 0; col < 8; col++) {
            kb->SetDecoderOutput(col);
            vTaskDelay(pdMS_TO_TICKS(2));

            uint8_t row_value = kb->ReadInputRow();

            // Check which input rows are active
            for (int bit = 0; bit < 7; bit++) {
                if (row_value & (1 << bit)) {
                    // Find matching entry and set the key state
                    for (int i = 0; i < TOTAL_COUNT; i++) {
                        if (all_entries[i].decoder_col == col && all_entries[i].input_bit == bit) {
                            uint8_t idx = (uint8_t)(all_entries[i].logical_row * 14 + all_entries[i].logical_col);
                            current_state |= (1ULL << idx);
                            break;
                        }
                    }
                }
            }
        }

        // Detect pressed and released keys
        uint64_t pressed = current_state & ~prev_state;
        uint64_t released = ~current_state & prev_state;

        // Process pressed keys
        auto process_event = [&](uint64_t state_mask, bool is_pressed) {
            uint64_t mask = 1ULL;
            for (int idx = 0; idx < 56; idx++) {
                if (state_mask & mask) {
                    int row = idx / 14;
                    int col = idx % 14;

                    // Fire key event callback
                    if (is_pressed && kb->key_event_callback_) {
                        KeyEvent event;
                        event.pressed = true;
                        event.is_modifier = false;
                        event.key_code = KC_NONE;
                        event.key_char = "";

                        if (row >= 0 && row < 4 && col >= 0 && col < 14) {
                            const KeyValue& kv = KEY_MAP[row][col];
                            event.key_code = kv.normal_code;
                            event.key_char = kv.normal;
                        }
                        kb->key_event_callback_(event);
                    }

                    // Fire legacy key callback for navigation keys
                    if (is_pressed && kb->key_callback_) {
                        LegacyKeyCode legacy = KEY_OTHER;
                        // Arrow keys and Enter
                        if (row == 2 && col == 11) legacy = KEY_UP;       // ; key
                        else if (row == 3 && col == 11) legacy = KEY_DOWN; // . key
                        else if (row == 3 && col == 10) legacy = KEY_LEFT; // , key
                        else if (row == 3 && col == 12) legacy = KEY_RIGHT; // / key
                        else if (row == 2 && col == 13) legacy = KEY_ENTER; // Enter key

                        if (legacy != KEY_OTHER && legacy != KEY_NONE) {
                            kb->key_callback_(legacy);
                        }
                    }
                }
                mask <<= 1;
            }
        };

        process_event(pressed, true);
        process_event(released, false);

        prev_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
