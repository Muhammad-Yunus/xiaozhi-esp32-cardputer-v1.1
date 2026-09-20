#ifndef WIFI_CONFIG_UI_H
#define WIFI_CONFIG_UI_H

#include <string>
#include <vector>
#include <functional>
#include "hc138_keyboard.h"
#include <lvgl.h>

class LcdDisplay;

enum class WifiConfigResult {
    Connected,
    Cancelled,
    None
};

struct WifiScanResult {
    std::string ssid;
    int8_t rssi;
    bool is_encrypted;
};

enum class WifiConfigState {
    Scanning,
    SelectWifi,
    InputPassword,
    SavedList,
    Connecting,
    Success,
    Failed
};

class WifiConfigUI {
public:
    explicit WifiConfigUI(LcdDisplay* display);
    ~WifiConfigUI();

    WifiConfigResult HandleKeyEvent(const KeyEvent& event);
    void Start();
    void StartWithSavedList();
    void SetConnectCallback(std::function<void(const std::string&, const std::string&)> cb) {
        connect_callback_ = cb;
    }
    void OnConnectResult(bool connected);

private:
    LcdDisplay* display_;
    std::function<void(const std::string&, const std::string&)> connect_callback_;

    WifiConfigState state_;
    bool is_active_;
    int selected_index_;
    int scroll_offset_;
    int saved_selected_index_;
    int saved_scroll_offset_;
    bool input_focus_on_password_;
    bool cursor_visible_;
    uint32_t last_cursor_toggle_;

    std::string input_ssid_;
    std::string input_password_;
    std::string selected_ssid_;

    std::vector<WifiScanResult> scan_results_;
    std::vector<std::pair<std::string, std::string>> saved_wifi_list_;

    static constexpr int MAX_VISIBLE_ITEMS = 6;
    static constexpr int MAX_INPUT_LENGTH = 64;
    static constexpr uint32_t CURSOR_BLINK_MS = 500;

    void StartScanning();
    void DoWifiScan();
    void ShowScanResults();
    void ShowPasswordInput();
    void RedrawPasswordInput();
    void ShowManualInput();
    void RedrawManualInput();
    void ShowSavedList();
    void DrawSavedWifiList();
    void ShowConnecting();
    void ShowSuccess();
    void ShowFailed();

    void DrawHeader(const char* title);
    void DrawFooter(const char* hint);
    void DrawWifiList(const std::vector<WifiScanResult>& list, int selected, int scroll);
    std::string GetSignalBars(int8_t rssi);

    void LoadSavedWifiList();
    void SaveWifiCredentials(const std::string& ssid, const std::string& password);
    void DeleteSavedWifi(int index);
    void AttemptConnection();

    void HandleScanningKey(const KeyEvent& event);
    void HandleSelectWifiKey(const KeyEvent& event);
    void HandlePasswordInputKey(const KeyEvent& event);
    void HandleManualInputKey(const KeyEvent& event);
    void HandleSavedListKey(const KeyEvent& event);
    void HandleConnectingKey(const KeyEvent& event);
    void HandleResultKey(const KeyEvent& event);
    void UpdateCursor();
};

#endif // WIFI_CONFIG_UI_H
