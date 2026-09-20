#include "wifi_config_ui.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <wifi_manager.h>
#include <ssid_manager.h>
#include <cstring>

#define TAG "WifiConfigUI"

WifiConfigUI::WifiConfigUI(LcdDisplay* display)
    : display_(display),
      state_(WifiConfigState::Scanning),
      is_active_(false),
      selected_index_(0),
      scroll_offset_(0),
      saved_selected_index_(0),
      saved_scroll_offset_(0),
      input_focus_on_password_(false),
      cursor_visible_(true),
      last_cursor_toggle_(0) {
}

WifiConfigUI::~WifiConfigUI() {
}

void WifiConfigUI::Start() {
    ESP_LOGI(TAG, "Starting WiFi config UI");
    is_active_ = true;
    state_ = WifiConfigState::Scanning;
    selected_index_ = 0;
    scroll_offset_ = 0;
    input_ssid_.clear();
    input_password_.clear();
    selected_ssid_.clear();

    // Load saved WiFi list
    LoadSavedWifiList();

    // Start scanning
    StartScanning();
}

void WifiConfigUI::StartWithSavedList() {
    ESP_LOGI(TAG, "Starting WiFi config UI with saved list");
    is_active_ = true;
    selected_index_ = 0;
    scroll_offset_ = 0;
    input_ssid_.clear();
    input_password_.clear();
    selected_ssid_.clear();

    // Show saved list directly (ShowSavedList will load the list)
    ShowSavedList();
}

void WifiConfigUI::StartScanning() {
    state_ = WifiConfigState::Scanning;

    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);
    DrawHeader("Scanning WiFi...");
    DrawFooter("Please wait...");

    // Perform WiFi scan
    DoWifiScan();

    // Show results
    if (scan_results_.empty()) {
        lv_obj_clean(canvas);
        DrawHeader("No WiFi Found");
        DrawFooter("W:Manual Esc:Back");
    } else {
        state_ = WifiConfigState::SelectWifi;
        ShowScanResults();
    }
}

void WifiConfigUI::DoWifiScan() {
    scan_results_.clear();

    // Configure scan
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = false;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min = 100;
    scan_config.scan_time.active.max = 300;

    // Start scan (blocking)
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(err));
        return;
    }

    // Get scan results
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count > 0) {
        wifi_ap_record_t* ap_records = new wifi_ap_record_t[ap_count];
        esp_wifi_scan_get_ap_records(&ap_count, ap_records);

        for (int i = 0; i < ap_count && i < 20; i++) {
            WifiScanResult result;
            result.ssid = std::string(reinterpret_cast<char*>(ap_records[i].ssid));
            result.rssi = ap_records[i].rssi;
            result.is_encrypted = (ap_records[i].authmode != WIFI_AUTH_OPEN);

            // Skip empty SSIDs
            if (!result.ssid.empty()) {
                scan_results_.push_back(result);
            }
        }

        delete[] ap_records;
    }

    ESP_LOGI(TAG, "Found %d WiFi networks", (int)scan_results_.size());
}

void WifiConfigUI::ShowScanResults() {
    DrawWifiList(scan_results_, selected_index_, scroll_offset_);
}

void WifiConfigUI::ShowPasswordInput() {
    // Only clear password and set state on first entry (not on redraw)
    if (state_ != WifiConfigState::InputPassword) {
        state_ = WifiConfigState::InputPassword;
        input_password_.clear();
    }

    RedrawPasswordInput();
}

void WifiConfigUI::RedrawPasswordInput() {
    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);

    DrawHeader("Enter Password");

    // Show selected SSID
    lv_obj_t* label = lv_label_create(canvas);
    lv_label_set_text_fmt(label, "Connect: %s", selected_ssid_.c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 5);

    lv_obj_t* pwd_label = lv_label_create(canvas);
    lv_label_set_text(pwd_label, "Password:");
    lv_obj_set_style_text_color(pwd_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(pwd_label, LV_ALIGN_TOP_LEFT, 5, 30);

    lv_obj_t* input_label = lv_label_create(canvas);
    std::string display_pwd(input_password_.length(), '*');
    display_pwd += cursor_visible_ ? "_" : " ";
    lv_label_set_text_fmt(input_label, ">>> %s", display_pwd.c_str());
    lv_obj_set_style_text_color(input_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(input_label, LV_ALIGN_TOP_LEFT, 5, 55);

    DrawFooter("Enter:OK Esc:Back");
}

void WifiConfigUI::ShowSavedList() {
    state_ = WifiConfigState::SavedList;
    saved_selected_index_ = 0;
    saved_scroll_offset_ = 0;

    LoadSavedWifiList();
    DrawSavedWifiList();
}

void WifiConfigUI::DrawSavedWifiList() {
    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);

    char title[48];
    snprintf(title, sizeof(title), "Saved WiFi (%d/10)", (int)saved_wifi_list_.size());
    DrawHeader(title);

    if (saved_wifi_list_.empty()) {
        lv_obj_t* empty_label = lv_label_create(canvas);
        lv_label_set_text(empty_label, "No saved WiFi");
        lv_obj_set_style_text_color(empty_label, lv_color_hex(0x888888), 0);
        lv_obj_align(empty_label, LV_ALIGN_CENTER, 0, 0);
        DrawFooter("Esc:Back");
        return;
    }

    int y_offset = 25;
    int visible_count = std::min((int)saved_wifi_list_.size() - saved_scroll_offset_, MAX_VISIBLE_ITEMS);

    for (int i = 0; i < visible_count; i++) {
        int idx = saved_scroll_offset_ + i;
        bool is_selected = (idx == saved_selected_index_);

        lv_obj_t* item_label = lv_label_create(canvas);
        char item_text[48];
        snprintf(item_text, sizeof(item_text), "%s %d. %s",
                 is_selected ? ">" : " ",
                 idx + 1,
                 saved_wifi_list_[idx].first.c_str());
        lv_label_set_text(item_label, item_text);
        lv_obj_set_style_text_color(item_label, is_selected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(item_label, LV_ALIGN_TOP_LEFT, 5, y_offset);
        y_offset += 20;
    }

    DrawFooter("Up/Down:Select Enter:Connect Del:Delete Esc:Back");
}

void WifiConfigUI::ShowConnecting() {
    state_ = WifiConfigState::Connecting;

    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);

    DrawHeader("Connecting...");

    lv_obj_t* ssid_label = lv_label_create(canvas);
    lv_label_set_text_fmt(ssid_label, "Connecting to: %s", selected_ssid_.c_str());
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, 0);

    DrawFooter("Please wait...");
}

void WifiConfigUI::ShowSuccess() {
    state_ = WifiConfigState::Success;

    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);

    DrawHeader("Connected!");

    lv_obj_t* ssid_label = lv_label_create(canvas);
    lv_label_set_text_fmt(ssid_label, "Connected: %s", selected_ssid_.c_str());
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t* saved_label = lv_label_create(canvas);
    lv_label_set_text(saved_label, "WiFi Config Saved");
    lv_obj_set_style_text_color(saved_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(saved_label, LV_ALIGN_CENTER, 0, 15);

    DrawFooter("Enter:Continue");
}

void WifiConfigUI::ShowFailed() {
    state_ = WifiConfigState::Failed;

    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);

    DrawHeader("Connection Failed");

    lv_obj_t* ssid_label = lv_label_create(canvas);
    lv_label_set_text_fmt(ssid_label, "Failed: %s", selected_ssid_.c_str());
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xFF0000), 0);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, 0);

    DrawFooter("Enter:Retry Esc:Back");
}

void WifiConfigUI::DrawHeader(const char* title) {
    lv_obj_t* canvas = lv_scr_act();

    lv_obj_t* header = lv_label_create(canvas);
    lv_label_set_text(header, title);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 5, 2);
}

void WifiConfigUI::DrawFooter(const char* hint) {
    lv_obj_t* canvas = lv_scr_act();

    lv_obj_t* footer = lv_label_create(canvas);
    lv_label_set_text(footer, hint);
    lv_obj_set_style_text_color(footer, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_LEFT, 5, -2);
}

void WifiConfigUI::DrawWifiList(const std::vector<WifiScanResult>& list, int selected, int scroll) {
    lv_obj_t* canvas = lv_scr_act();
    lv_obj_clean(canvas);

    DrawHeader("Select WiFi");

    int y_offset = 25;
    int visible_count = std::min((int)list.size() - scroll, MAX_VISIBLE_ITEMS);

    for (int i = 0; i < visible_count; i++) {
        int idx = scroll + i;
        bool is_selected = (idx == selected);
        const WifiScanResult& wifi = list[idx];

        lv_obj_t* item_label = lv_label_create(canvas);
        std::string signal = GetSignalBars(wifi.rssi);
        char item_text[64];
        snprintf(item_text, sizeof(item_text), "%s%d.%-12s %4ddBm %s",
                 is_selected ? ">" : " ",
                 idx + 1,
                 wifi.ssid.substr(0, 12).c_str(),
                 wifi.rssi,
                 signal.c_str());
        lv_label_set_text(item_label, item_text);
        lv_obj_set_style_text_color(item_label, is_selected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(item_label, LV_ALIGN_TOP_LEFT, 2, y_offset);
        y_offset += 20;
    }

    DrawFooter("Up/Down:Select Enter:Connect W:Manual S:Saved");
}

std::string WifiConfigUI::GetSignalBars(int8_t rssi) {
    if (rssi >= -50) return "████";
    if (rssi >= -60) return "███░";
    if (rssi >= -70) return "██░░";
    if (rssi >= -80) return "█░░░";
    return "░░░░";
}

void WifiConfigUI::LoadSavedWifiList() {
    saved_wifi_list_.clear();
    auto& ssid_manager = SsidManager::GetInstance();
    const auto& ssid_list = ssid_manager.GetSsidList();

    for (const auto& item : ssid_list) {
        saved_wifi_list_.push_back({item.ssid, item.password});
    }
}

void WifiConfigUI::SaveWifiCredentials(const std::string& ssid, const std::string& password) {
    auto& ssid_manager = SsidManager::GetInstance();
    ssid_manager.AddSsid(ssid, password);
    ESP_LOGI(TAG, "Saved WiFi credentials for: %s", ssid.c_str());
}

void WifiConfigUI::DeleteSavedWifi(int index) {
    if (index >= 0 && index < (int)saved_wifi_list_.size()) {
        auto& ssid_manager = SsidManager::GetInstance();
        ssid_manager.RemoveSsid(index);
        ESP_LOGI(TAG, "Deleted saved WiFi at index: %d", index);
        LoadSavedWifiList();
    }
}

void WifiConfigUI::AttemptConnection() {
    ShowConnecting();

    if (connect_callback_) {
        connect_callback_(selected_ssid_, input_password_);
    }
}

void WifiConfigUI::OnConnectResult(bool success) {
    if (success) {
        SaveWifiCredentials(selected_ssid_, input_password_);
        ShowSuccess();
    } else {
        ShowFailed();
    }
}

WifiConfigResult WifiConfigUI::HandleKeyEvent(const KeyEvent& event) {
    // Only handle key press events, skip modifiers
    if (!event.pressed || event.is_modifier) {
        return WifiConfigResult::None;
    }

    // Check for ESC to cancel from Scanning or SelectWifi states
    if (event.key_code == KC_ESC) {
        if (state_ == WifiConfigState::Scanning ||
            state_ == WifiConfigState::SelectWifi) {
            is_active_ = false;
            return WifiConfigResult::Cancelled;
        }
    }

    // Check if not active (was cancelled in a handler)
    if (!is_active_) {
        return WifiConfigResult::Cancelled;
    }

    switch (state_) {
        case WifiConfigState::Scanning:
            HandleScanningKey(event);
            break;
        case WifiConfigState::SelectWifi:
            HandleSelectWifiKey(event);
            break;
        case WifiConfigState::InputPassword:
            HandlePasswordInputKey(event);
            break;
        case WifiConfigState::Connecting:
            HandleConnectingKey(event);
            break;
        case WifiConfigState::Success:
            HandleResultKey(event);
            if (event.key_code == KC_ENTER) {
                is_active_ = false;
                return WifiConfigResult::Connected;
            }
            break;
        case WifiConfigState::Failed:
            HandleResultKey(event);
            break;
        case WifiConfigState::SavedList:
            HandleSavedListKey(event);
            break;
    }

    // Check if cancelled by a handler
    if (!is_active_) {
        return WifiConfigResult::Cancelled;
    }

    return WifiConfigResult::None;
}

void WifiConfigUI::HandleScanningKey(const KeyEvent& event) {
    if (event.key_code == KC_W) {
        // Manual input - go to select wifi first
        ShowScanResults();
    } else if (event.key_code == KC_S) {
        ShowSavedList();
    }
}

void WifiConfigUI::HandleSelectWifiKey(const KeyEvent& event) {
    switch (event.key_code) {
        case KC_UP:
            if (selected_index_ > 0) {
                selected_index_--;
                if (selected_index_ < scroll_offset_) {
                    scroll_offset_ = selected_index_;
                }
                ShowScanResults();
            }
            break;

        case KC_DOWN:
            if (selected_index_ < (int)scan_results_.size() - 1) {
                selected_index_++;
                if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_ITEMS) {
                    scroll_offset_ = selected_index_ - MAX_VISIBLE_ITEMS + 1;
                }
                ShowScanResults();
            }
            break;

        case KC_ENTER:
            if (!scan_results_.empty()) {
                selected_ssid_ = scan_results_[selected_index_].ssid;
                ShowPasswordInput();
            }
            break;

        case KC_W:
            // Manual input would go here in full version
            break;

        case KC_S:
            ShowSavedList();
            break;

        default:
            break;
    }
}

void WifiConfigUI::HandlePasswordInputKey(const KeyEvent& event) {
    switch (event.key_code) {
        case KC_ENTER:
            if (!input_password_.empty()) {
                AttemptConnection();
            }
            break;

        case KC_ESC:
            state_ = WifiConfigState::SelectWifi;
            ShowScanResults();
            break;

        case KC_BACKSPACE:
            if (!input_password_.empty()) {
                input_password_.pop_back();
                RedrawPasswordInput();
            }
            break;

        default:
            // Add character if it's a printable key
            if (event.key_char && strlen(event.key_char) > 0 && input_password_.length() < MAX_INPUT_LENGTH) {
                input_password_ += event.key_char;
                RedrawPasswordInput();
            }
            break;
    }
}

void WifiConfigUI::HandleSavedListKey(const KeyEvent& event) {
    switch (event.key_code) {
        case KC_UP:
            if (saved_selected_index_ > 0) {
                saved_selected_index_--;
                if (saved_selected_index_ < saved_scroll_offset_) {
                    saved_scroll_offset_ = saved_selected_index_;
                }
                DrawSavedWifiList();
            }
            break;

        case KC_DOWN:
            if (saved_selected_index_ < (int)saved_wifi_list_.size() - 1) {
                saved_selected_index_++;
                if (saved_selected_index_ >= saved_scroll_offset_ + MAX_VISIBLE_ITEMS) {
                    saved_scroll_offset_ = saved_selected_index_ - MAX_VISIBLE_ITEMS + 1;
                }
                DrawSavedWifiList();
            }
            break;

        case KC_ENTER:
            if (!saved_wifi_list_.empty()) {
                selected_ssid_ = saved_wifi_list_[saved_selected_index_].first;
                input_password_ = saved_wifi_list_[saved_selected_index_].second;
                AttemptConnection();
            }
            break;

        case KC_BACKSPACE:  // Del key for delete
            if (!saved_wifi_list_.empty()) {
                DeleteSavedWifi(saved_selected_index_);
                if (saved_selected_index_ >= (int)saved_wifi_list_.size() && saved_selected_index_ > 0) {
                    saved_selected_index_--;
                }
                DrawSavedWifiList();
            }
            break;

        case KC_ESC:
            state_ = WifiConfigState::SelectWifi;
            ShowScanResults();
            break;

        default:
            break;
    }
}

void WifiConfigUI::HandleConnectingKey(const KeyEvent& event) {
    // No key handling during connection
    (void)event;
}

void WifiConfigUI::HandleResultKey(const KeyEvent& event) {
    if (state_ == WifiConfigState::Success) {
        if (event.key_code == KC_ENTER) {
            // Will be handled in HandleKeyEvent to return Connected
        }
    } else if (state_ == WifiConfigState::Failed) {
        if (event.key_code == KC_ENTER) {
            // Retry - go back to password input
            state_ = WifiConfigState::InputPassword;
            RedrawPasswordInput();
        } else if (event.key_code == KC_ESC) {
            state_ = WifiConfigState::SelectWifi;
            ShowScanResults();
        }
    }
}
