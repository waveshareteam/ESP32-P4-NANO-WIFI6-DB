#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:UartTool"
#include "esp_lib_utils.h"

#include <cstring>
#include "UartTool.hpp"
#include "uart_tool_format.hpp"

#define APP_NAME            "UART"
#define GPIO_PIN_MAX        54
#define BAUD_OPTIONS        "9600\n19200\n38400\n57600\n115200\n230400\n460800\n921600"
#define DATA_BITS_OPTIONS   "5\n6\n7\n8"
#define STOP_BITS_OPTIONS   "1\n1.5\n2"
#define PARITY_OPTIONS      "None\nEven\nOdd"
#define RX_RAW_MAX          2048
#define RX_POLL_PERIOD_MS   100
#define RX_DRAIN_CHUNK      512

LV_IMG_DECLARE(img_app_uart_tool);

namespace esp_brookesia::apps {

static const char *hex_kb_map[] = {
    "1", "2", "3", "A", "B", "\n",
    "4", "5", "6", "C", "D", "\n",
    "7", "8", "9", "E", "F", "\n",
    "0", " ", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t hex_kb_ctrl[] = {
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
};

static const int baud_values[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

// pins already used by the BSP (I2C/I2S/SD/AMP); selecting them only warns
static const int reserved_pins[] = {7, 8, 9, 10, 11, 12, 13, 39, 40, 41, 42, 43, 44, 53};

static bool is_reserved_pin(int pin)
{
    for (int p : reserved_pins) {
        if (p == pin) {
            return true;
        }
    }
    return false;
}

static int baud_to_index(int baud)
{
    for (size_t i = 0; i < sizeof(baud_values) / sizeof(baud_values[0]); i++) {
        if (baud_values[i] == baud) {
            return static_cast<int>(i);
        }
    }
    return 4;   // 115200
}

UartTool *UartTool::_instance = nullptr;

UartTool *UartTool::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new UartTool(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

UartTool::UartTool(bool use_status_bar, bool use_navigation_bar):
    App(APP_NAME, &img_app_uart_tool, true, use_status_bar, use_navigation_bar)
{
}

UartTool::~UartTool()
{
    ESP_UTILS_LOGD("Destroy(@0x%p)", this);
}

bool UartTool::run(void)
{
    ESP_UTILS_LOGD("Run(@0x%p)", this);

    _config = uart_tool_config_load();
    _rx_raw.clear();

    auto visual_area = getVisualArea();
    int w = lv_area_get_width(&visual_area);
    int h = lv_area_get_height(&visual_area);
    if (w == 0 || h == 0) {
        w = 360;
        h = 360;
    }
    // Round screens report a square visual area; inset so content stays out of the corners
    int pad = (w == h) ? (w * 6 / 100) : (w * 2 / 100);
    _content_w = w - 2 * pad;
    _row_h = h / 9;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x14181C), 0);

    _root = lv_obj_create(scr);
    lv_obj_set_size(_root, _content_w, h - 2 * pad);
    lv_obj_center(_root);
    lv_obj_set_style_bg_opa(_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_set_style_pad_row(_root, _row_h / 8, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);

    createConfigSection(_root);
    createRxSection(_root);
    createTxSection(_root);

    if (!_service.start(_config)) {
        setStatus("UART start failed", true);
    } else {
        setStatus("Running", false);
    }

    lv_timer_create(rx_timer_cb, RX_POLL_PERIOD_MS, this);

    return true;
}

void UartTool::createConfigSection(lv_obj_t *parent)
{
    auto add_row = [&](const char *name) -> lv_obj_t * {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_size(row, LV_PCT(100), _row_h);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(row);
        lv_label_set_text(label, name);
        lv_obj_set_style_text_color(label, lv_color_hex(0xE0E0E0), 0);
        return row;
    };

    auto add_spinbox_row = [&](const char *name) -> lv_obj_t * {
        lv_obj_t *row = add_row(name);

        lv_obj_t *btn_minus = lv_button_create(row);
        lv_obj_set_size(btn_minus, _row_h * 3 / 4, _row_h * 3 / 4);
        lv_obj_t *minus_label = lv_label_create(btn_minus);
        lv_label_set_text(minus_label, LV_SYMBOL_MINUS);
        lv_obj_center(minus_label);

        lv_obj_t *spinbox = lv_spinbox_create(row);
        lv_spinbox_set_range(spinbox, 0, GPIO_PIN_MAX);
        lv_spinbox_set_digit_format(spinbox, 2, 0);
        lv_obj_set_size(spinbox, _content_w / 5, _row_h * 3 / 4);
        lv_obj_remove_flag(spinbox, LV_OBJ_FLAG_CLICK_FOCUSABLE);

        lv_obj_t *btn_plus = lv_button_create(row);
        lv_obj_set_size(btn_plus, _row_h * 3 / 4, _row_h * 3 / 4);
        lv_obj_t *plus_label = lv_label_create(btn_plus);
        lv_label_set_text(plus_label, LV_SYMBOL_PLUS);
        lv_obj_center(plus_label);

        lv_obj_add_event_cb(btn_minus, [](lv_event_t *e) {
            lv_spinbox_decrement(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
        }, LV_EVENT_CLICKED, spinbox);
        lv_obj_add_event_cb(btn_plus, [](lv_event_t *e) {
            lv_spinbox_increment(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
        }, LV_EVENT_CLICKED, spinbox);

        return spinbox;
    };

    auto add_dropdown_row = [&](const char *name, const char *options) -> lv_obj_t * {
        lv_obj_t *row = add_row(name);
        lv_obj_t *dropdown = lv_dropdown_create(row);
        lv_dropdown_set_options_static(dropdown, options);
        lv_obj_set_size(dropdown, _content_w / 2, _row_h * 3 / 4);
        return dropdown;
    };

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Config");
    lv_obj_set_style_text_color(title, lv_color_hex(0x90CAF9), 0);

    _tx_pin_spinbox = add_spinbox_row("TX");
    _rx_pin_spinbox = add_spinbox_row("RX");
    _baud_dropdown = add_dropdown_row("Baud", BAUD_OPTIONS);
    _data_dropdown = add_dropdown_row("Data", DATA_BITS_OPTIONS);
    _stop_dropdown = add_dropdown_row("Stop", STOP_BITS_OPTIONS);
    _parity_dropdown = add_dropdown_row("Parity", PARITY_OPTIONS);

    lv_obj_t *apply_btn = lv_button_create(parent);
    lv_obj_set_size(apply_btn, LV_PCT(100), _row_h);
    lv_obj_t *apply_label = lv_label_create(apply_btn);
    lv_label_set_text(apply_label, "Apply");
    lv_obj_center(apply_label);
    lv_obj_add_event_cb(apply_btn, apply_btn_event_cb, LV_EVENT_CLICKED, this);

    _status_label = lv_label_create(parent);
    lv_label_set_text(_status_label, "");
    lv_label_set_long_mode(_status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_status_label, LV_PCT(100));
    lv_obj_set_style_text_color(_status_label, lv_color_hex(0xB0BEC5), 0);

    loadConfigToWidgets();
}

void UartTool::createRxSection(lv_obj_t *parent)
{
    lv_obj_t *toolbar = lv_obj_create(parent);
    lv_obj_set_size(toolbar, LV_PCT(100), _row_h);
    lv_obj_set_style_bg_opa(toolbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(toolbar, 0, 0);
    lv_obj_set_style_pad_all(toolbar, 0, 0);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(toolbar);
    lv_label_set_text(title, "RX  HEX:");
    lv_obj_set_style_text_color(title, lv_color_hex(0x90CAF9), 0);

    _rx_hex_switch = lv_switch_create(toolbar);
    if (_config.hex_rx) {
        lv_obj_add_state(_rx_hex_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(_rx_hex_switch, rx_hex_switch_event_cb, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t *clear_btn = lv_button_create(toolbar);
    lv_obj_set_size(clear_btn, _row_h * 3 / 2, _row_h * 3 / 4);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, LV_SYMBOL_TRASH);
    lv_obj_center(clear_label);
    lv_obj_add_event_cb(clear_btn, rx_clear_btn_event_cb, LV_EVENT_CLICKED, this);

    _rx_textarea = lv_textarea_create(parent);
    lv_obj_set_size(_rx_textarea, LV_PCT(100), _row_h * 3);
    lv_textarea_set_text(_rx_textarea, "");
    lv_obj_remove_flag(_rx_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);   // read-only view
    lv_obj_set_style_bg_color(_rx_textarea, lv_color_hex(0x1E2429), 0);
    lv_obj_set_style_text_color(_rx_textarea, lv_color_hex(0xC8E6C9), 0);
}

void UartTool::createTxSection(lv_obj_t *parent)
{
    lv_obj_t *toolbar = lv_obj_create(parent);
    lv_obj_set_size(toolbar, LV_PCT(100), _row_h);
    lv_obj_set_style_bg_opa(toolbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(toolbar, 0, 0);
    lv_obj_set_style_pad_all(toolbar, 0, 0);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(toolbar);
    lv_label_set_text(title, "TX  HEX:");
    lv_obj_set_style_text_color(title, lv_color_hex(0x90CAF9), 0);

    _tx_hex_switch = lv_switch_create(toolbar);
    if (_config.hex_tx) {
        lv_obj_add_state(_tx_hex_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(_tx_hex_switch, tx_hex_switch_event_cb, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t *send_btn = lv_button_create(toolbar);
    lv_obj_set_size(send_btn, _row_h * 2, _row_h * 3 / 4);
    lv_obj_t *send_label = lv_label_create(send_btn);
    lv_label_set_text(send_label, "Send");
    lv_obj_center(send_label);
    lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, this);

    _tx_textarea = lv_textarea_create(parent);
    lv_obj_set_size(_tx_textarea, LV_PCT(100), _row_h * 3 / 2);
    lv_textarea_set_placeholder_text(_tx_textarea, _config.hex_tx ? "AA BB 01" : "text to send");
    lv_obj_add_event_cb(_tx_textarea, tx_ta_event_cb, LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(_tx_textarea, tx_ta_event_cb, LV_EVENT_DEFOCUSED, this);
    lv_obj_add_event_cb(_tx_textarea, tx_ta_event_cb, LV_EVENT_READY, this);

    // Pre-created hidden keyboard on the screen itself, floating at the bottom;
    // creating it here (inside run()) keeps it recorded by the core for recycling
    _keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_map(_keyboard, LV_KEYBOARD_MODE_USER_1, hex_kb_map, hex_kb_ctrl);
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void UartTool::loadConfigToWidgets()
{
    lv_spinbox_set_value(_tx_pin_spinbox, _config.tx_pin);
    lv_spinbox_set_value(_rx_pin_spinbox, _config.rx_pin);
    lv_dropdown_set_selected(_baud_dropdown, baud_to_index(_config.baud));
    lv_dropdown_set_selected(_data_dropdown, _config.data_bits - UART_DATA_5_BITS);
    int stop_index = (_config.stop_bits == UART_STOP_BITS_1) ? 0 :
                     (_config.stop_bits == UART_STOP_BITS_1_5) ? 1 : 2;
    lv_dropdown_set_selected(_stop_dropdown, stop_index);
    int parity_index = (_config.parity == UART_PARITY_DISABLE) ? 0 :
                       (_config.parity == UART_PARITY_EVEN) ? 1 : 2;
    lv_dropdown_set_selected(_parity_dropdown, parity_index);
}

void UartTool::applyConfig()
{
    UartToolConfig new_config = _config;
    new_config.tx_pin = static_cast<int>(lv_spinbox_get_value(_tx_pin_spinbox));
    new_config.rx_pin = static_cast<int>(lv_spinbox_get_value(_rx_pin_spinbox));
    new_config.baud = baud_values[lv_dropdown_get_selected(_baud_dropdown)];
    new_config.data_bits = static_cast<uart_word_length_t>(UART_DATA_5_BITS + lv_dropdown_get_selected(_data_dropdown));
    static const uart_stop_bits_t stop_map[] = {UART_STOP_BITS_1, UART_STOP_BITS_1_5, UART_STOP_BITS_2};
    new_config.stop_bits = stop_map[lv_dropdown_get_selected(_stop_dropdown)];
    static const uart_parity_t parity_map[] = {UART_PARITY_DISABLE, UART_PARITY_EVEN, UART_PARITY_ODD};
    new_config.parity = parity_map[lv_dropdown_get_selected(_parity_dropdown)];

    const char *warning = nullptr;
    if (new_config.tx_pin == new_config.rx_pin) {
        warning = "TX == RX";
    } else if (is_reserved_pin(new_config.tx_pin) || is_reserved_pin(new_config.rx_pin)) {
        warning = "pin used by BSP";
    }

    if (!_service.start(new_config)) {
        setStatus("Apply failed, restoring", true);
        if (!_service.start(_config)) {
            setStatus("UART dead, check pins", true);
        }
        loadConfigToWidgets();   // resync widgets to the still-active old config
        return;
    }

    _config = new_config;
    uart_tool_config_save(_config);

    char text[96];
    if (warning != nullptr) {
        snprintf(text, sizeof(text), "OK: TX%d RX%d %d (Warn: %s)",
                 _config.tx_pin, _config.rx_pin, _config.baud, warning);
        setStatus(text, true);
    } else {
        snprintf(text, sizeof(text), "OK: TX%d RX%d %d", _config.tx_pin, _config.rx_pin, _config.baud);
        setStatus(text, false);
    }
}

void UartTool::setStatus(const char *text, bool is_error)
{
    if (_status_label == nullptr) {
        return;
    }
    lv_label_set_text(_status_label, text);
    lv_obj_set_style_text_color(_status_label, is_error ? lv_color_hex(0xEF5350) : lv_color_hex(0x81C784), 0);
}

void UartTool::renderRxText()
{
    if (_rx_textarea == nullptr) {
        return;
    }
    std::string text = _config.hex_rx ?
                       uart_tool_bytes_to_hex(_rx_raw.data(), _rx_raw.size()) :
                       uart_tool_bytes_to_ascii(_rx_raw.data(), _rx_raw.size());
    lv_textarea_set_text(_rx_textarea, text.c_str());
    lv_obj_scroll_to_y(_rx_textarea, LV_COORD_MAX, LV_ANIM_OFF);
}

void UartTool::appendRxBytes(const uint8_t *data, size_t len)
{
    _rx_raw.insert(_rx_raw.end(), data, data + len);
    if (_rx_raw.size() > RX_RAW_MAX) {
        _rx_raw.erase(_rx_raw.begin(), _rx_raw.begin() + (_rx_raw.size() - RX_RAW_MAX));
    }
    renderRxText();
}

void UartTool::sendCurrentInput()
{
    const char *text = lv_textarea_get_text(_tx_textarea);
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    if (_config.hex_tx) {
        std::vector<uint8_t> bytes;
        if (!uart_tool_hex_to_bytes(text, bytes)) {
            setStatus("Bad HEX input", true);
            return;
        }
        if (bytes.empty()) {
            return;
        }
        if (!_service.send(bytes.data(), bytes.size())) {
            setStatus("Send failed", true);
            return;
        }
    } else {
        if (!_service.send(reinterpret_cast<const uint8_t *>(text), strlen(text))) {
            setStatus("Send failed", true);
            return;
        }
    }
    setStatus("Sent", false);
}

void UartTool::apply_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->applyConfig();
}

void UartTool::rx_timer_cb(lv_timer_t *timer)
{
    auto *self = static_cast<UartTool *>(lv_timer_get_user_data(timer));
    uint8_t chunk[RX_DRAIN_CHUNK];
    size_t n = self->_service.readRx(chunk, sizeof(chunk));
    if (n > 0) {
        self->appendRxBytes(chunk, n);
    }
}

void UartTool::rx_hex_switch_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->_config.hex_rx = lv_obj_has_state(self->_rx_hex_switch, LV_STATE_CHECKED);
    uart_tool_config_save(self->_config);
    self->renderRxText();
}

void UartTool::rx_clear_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->_rx_raw.clear();
    self->renderRxText();
}

void UartTool::tx_hex_switch_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->_config.hex_tx = lv_obj_has_state(self->_tx_hex_switch, LV_STATE_CHECKED);
    uart_tool_config_save(self->_config);
    lv_textarea_set_placeholder_text(self->_tx_textarea, self->_config.hex_tx ? "AA BB 01" : "text to send");
}

void UartTool::send_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->sendCurrentInput();
}

void UartTool::tx_ta_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(self->_keyboard, self->_tx_textarea);
        lv_keyboard_set_mode(self->_keyboard,
                             self->_config.hex_tx ? LV_KEYBOARD_MODE_USER_1 : LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_remove_flag(self->_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(self->_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    } else {   // LV_EVENT_DEFOCUSED or LV_EVENT_READY
        lv_keyboard_set_textarea(self->_keyboard, nullptr);
        lv_obj_add_flag(self->_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

bool UartTool::back(void)
{
    ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
    return true;
}

bool UartTool::close(void)
{
    ESP_UTILS_LOGD("Close(@0x%p)", this);
    return true;
}

bool UartTool::pause(void)
{
    ESP_UTILS_LOGD("Pause(@0x%p)", this);
    // keep the UART service running so data is still collected in background
    return true;
}

bool UartTool::resume(void)
{
    ESP_UTILS_LOGD("Resume(@0x%p)", this);
    return true;
}

bool UartTool::cleanResource(void)
{
    ESP_UTILS_LOGD("CleanResource(@0x%p)", this);
    _service.stop();
    _rx_raw.clear();
    _rx_raw.shrink_to_fit();
    _root = nullptr;
    _tx_pin_spinbox = nullptr;
    _rx_pin_spinbox = nullptr;
    _baud_dropdown = nullptr;
    _data_dropdown = nullptr;
    _stop_dropdown = nullptr;
    _parity_dropdown = nullptr;
    _status_label = nullptr;
    _rx_textarea = nullptr;
    _rx_hex_switch = nullptr;
    _tx_textarea = nullptr;
    _tx_hex_switch = nullptr;
    _keyboard = nullptr;
    return true;
}

} // namespace esp_brookesia::apps
