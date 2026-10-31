#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:UartTool"
#include "esp_lib_utils.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include "UartTool.hpp"
#include "uart_tool_format.hpp"

#define APP_NAME            "UART"
#define BAUD_OPTIONS        "9600\n19200\n38400\n57600\n115200\n230400\n460800"
#define DATA_BITS_OPTIONS   "5\n6\n7\n8"
#define STOP_BITS_OPTIONS   "1\n1.5\n2"
#define PARITY_OPTIONS      "None\nEven\nOdd"
#define RX_RAW_MAX          2048
#define RX_POLL_PERIOD_MS   100
#define RX_DRAIN_CHUNK      2048
#define RX_DRAIN_BUDGET     16384

LV_IMG_DECLARE(img_app_uart_tool);

namespace esp_brookesia::apps {

namespace {

constexpr uint32_t COLOR_SCREEN = 0x0B0F14;
constexpr uint32_t COLOR_CARD = 0x151B23;
constexpr uint32_t COLOR_TERMINAL = 0x090D12;
constexpr uint32_t COLOR_BORDER = 0x334155;
constexpr uint32_t COLOR_PRIMARY = 0x3B82F6;
constexpr uint32_t COLOR_PRIMARY_DARK = 0x1D4ED8;
constexpr uint32_t COLOR_ACCENT = 0x38BDF8;
constexpr uint32_t COLOR_TEXT = 0xF8FAFC;
constexpr uint32_t COLOR_TEXT_MUTED = 0x94A3B8;
constexpr uint32_t COLOR_SUCCESS = 0x22C55E;
constexpr uint32_t COLOR_WARNING = 0xF59E0B;
constexpr uint32_t COLOR_ERROR = 0xEF5350;
constexpr uint32_t COLOR_RX_TEXT = 0xD1FAE5;

static const char *hex_kb_map[] = {
    "1", "2", "3", "A", "B", "\n",
    "4", "5", "6", "C", "D", "\n",
    "7", "8", "9", "E", "F", "\n",
    "0", " ", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t hex_kb_ctrl[] = {
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
};

static const int baud_values[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800};

static int baud_to_index(int baud)
{
    for (size_t i = 0; i < sizeof(baud_values) / sizeof(baud_values[0]); i++) {
        if (baud_values[i] == baud) {
            return static_cast<int>(i);
        }
    }
    return 4;
}

static void style_transparent(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void style_card(lv_obj_t *obj, int radius = 18)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, 0);
    lv_obj_set_style_shadow_width(obj, 12, 0);
    lv_obj_set_style_shadow_ofs_y(obj, 4, 0);
}

static void style_button(lv_obj_t *obj, bool primary = false)
{
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(primary ? COLOR_PRIMARY : COLOR_BORDER), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(primary ? COLOR_PRIMARY : COLOR_CARD), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);

    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_PRIMARY_DARK), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_PRIMARY), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_PRIMARY), LV_STATE_CHECKED);
    lv_obj_set_style_text_color(obj, lv_color_hex(COLOR_TEXT), LV_STATE_CHECKED);
}

static lv_obj_t *make_label(
    lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color
)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *make_button(
    lv_obj_t *parent, const char *text, int width, int height, bool primary = false
)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, width, height);
    style_button(button, primary);
    lv_obj_t *label = make_label(button, text, &lv_font_montserrat_22, COLOR_TEXT);
    lv_obj_center(label);
    return button;
}

static void set_checked(lv_obj_t *obj, bool checked)
{
    if (obj == nullptr) {
        return;
    }
    if (checked) {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
    }
}

} // namespace

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
    _rx_render_text.clear();
    _rx_render_text.reserve(RX_RAW_MAX * 3);
    _line_ending = 0;

    auto visual_area = getVisualArea();
    int w = lv_area_get_width(&visual_area);
    int h = lv_area_get_height(&visual_area);
    if (w == 0 || h == 0) {
        w = 720;
        h = 1280;
    }

    _screen = lv_screen_active();
    lv_obj_set_style_bg_color(_screen, lv_color_hex(COLOR_SCREEN), 0);
    createUi(_screen, w, h);
    lv_obj_add_event_cb(_screen, screen_size_changed_event_cb, LV_EVENT_SIZE_CHANGED, this);

    if (uart_tool_fixed_pins_ready(_config.mode) && !_service.start(_config)) {
        setStatus("Serial error", COLOR_ERROR);
    } else {
        updateConnectionStatus();
    }

    _rx_timer = lv_timer_create(rx_timer_cb, RX_POLL_PERIOD_MS, this);
    return true;
}

void UartTool::createUi(lv_obj_t *screen, int width, int height)
{
    const int pad_x = std::max(20, width * 4 / 100);
    const int pad_y = std::max(18, height * 2 / 100);
    _layout_w = width;
    _layout_h = height;
    _content_w = width - 2 * pad_x;
    _row_h = std::max(58, height / 16);
    _compact_layout = (height < 800);
    _narrow_layout = (_content_w < 560);

    _root = lv_obj_create(screen);
    lv_obj_set_size(_root, _content_w, height - 2 * pad_y);
    lv_obj_center(_root);
    style_transparent(_root);
    lv_obj_set_style_pad_row(_root, _compact_layout ? 10 : 16, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    if (_compact_layout) {
        lv_obj_set_scroll_dir(_root, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);
    } else {
        lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);
        lv_obj_remove_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    }

    createHeader(_root);
    createConnectionCard(_root);
    createRxSection(_root);
    createTxSection(_root);
    createConfigSection(screen);
    createKeyboard();

    updateConnectionSummary();
    updateModeButtons();
    renderRxText();
    updateConnectionStatus();
}

void UartTool::rebuildUi(int width, int height)
{
    if (_screen == nullptr || width <= 0 || height <= 0 ||
        (width == _layout_w && height == _layout_h)) {
        return;
    }

    ESP_UTILS_LOGI("Relayout UI: %dx%d -> %dx%d", _layout_w, _layout_h, width, height);
    deleteUiObjects();
    createUi(_screen, width, height);
}

void UartTool::deleteUiObjects()
{
    if (_keyboard != nullptr) {
        lv_obj_delete(_keyboard);
    }
    if (_settings_overlay != nullptr) {
        lv_obj_delete(_settings_overlay);
    }
    if (_root != nullptr) {
        lv_obj_delete(_root);
    }
    clearUiPointers();
}

void UartTool::clearUiPointers()
{
    _root = nullptr;
    _status_dot = nullptr;
    _status_label = nullptr;
    for (auto &button : _serial_mode_buttons) {
        button = nullptr;
    }
    _baud_summary_label = nullptr;
    _format_summary_label = nullptr;
    _settings_overlay = nullptr;
    _baud_dropdown = nullptr;
    _data_dropdown = nullptr;
    _stop_dropdown = nullptr;
    _parity_dropdown = nullptr;
    _rx_scroll = nullptr;
    _rx_label = nullptr;
    _rx_count_label = nullptr;
    _rx_ascii_mode_btn = nullptr;
    _rx_hex_switch = nullptr;
    _auto_scroll_checkbox = nullptr;
    _tx_textarea = nullptr;
    _tx_hex_switch = nullptr;
    _tx_text_mode_btn = nullptr;
    for (auto &button : _line_ending_buttons) {
        button = nullptr;
    }
    _keyboard = nullptr;
}

void UartTool::createHeader(lv_obj_t *parent)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, LV_PCT(100), _compact_layout ? 52 : 64);
    style_transparent(header);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    make_label(
        header, "Serial Terminal",
        _narrow_layout ? &lv_font_montserrat_30 : &lv_font_montserrat_36,
        COLOR_TEXT
    );

    lv_obj_t *status = lv_obj_create(header);
    lv_obj_set_size(status, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    style_transparent(status);
    lv_obj_set_style_pad_column(status, 12, 0);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);

    _status_dot = lv_obj_create(status);
    lv_obj_set_size(_status_dot, _narrow_layout ? 12 : 16, _narrow_layout ? 12 : 16);
    lv_obj_set_style_radius(_status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_status_dot, 0, 0);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(COLOR_SUCCESS), 0);
    lv_obj_remove_flag(_status_dot, LV_OBJ_FLAG_SCROLLABLE);

    _status_label = make_label(
        status, "Ready",
        _narrow_layout ? &lv_font_montserrat_20 : &lv_font_montserrat_24,
        COLOR_SUCCESS
    );
}

void UartTool::createConnectionCard(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), _narrow_layout ? 204 : (_compact_layout ? 168 : 184));
    style_card(card);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *mode_row = lv_obj_create(card);
    lv_obj_set_size(mode_row, LV_PCT(100), _compact_layout ? 62 : 70);
    lv_obj_set_style_bg_color(mode_row, lv_color_hex(COLOR_TERMINAL), 0);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(mode_row, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(mode_row, 1, 0);
    lv_obj_set_style_radius(mode_row, 14, 0);
    lv_obj_set_style_pad_all(mode_row, 4, 0);
    lv_obj_set_style_pad_column(mode_row, 4, 0);
    lv_obj_set_flex_flow(mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);

    const char *mode_names[] = {"UART", "RS485", "RS232"};
    for (int i = 0; i < 3; i++) {
        _serial_mode_buttons[i] = make_button(
            mode_row, mode_names[i], 0, _compact_layout ? 54 : 62
        );
        lv_obj_set_flex_grow(_serial_mode_buttons[i], 1);
        lv_obj_add_event_cb(
            _serial_mode_buttons[i], serial_mode_btn_event_cb, LV_EVENT_CLICKED, this
        );
        // No RS485/RS232 transceiver is wired on this board. A hidden button stays out of
        // the flex layout and ignores clicks, so UART fills the row on its own.
        if (i != static_cast<int>(UartToolMode::Uart)) {
            lv_obj_add_flag(_serial_mode_buttons[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    lv_obj_t *summary_row = lv_obj_create(card);
    lv_obj_set_size(summary_row, LV_PCT(100), _compact_layout ? 62 : 70);
    style_transparent(summary_row);
    lv_obj_set_style_pad_column(summary_row, 12, 0);
    lv_obj_set_flex_flow(summary_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        summary_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(summary_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *serial_summary = lv_obj_create(summary_row);
    lv_obj_set_size(serial_summary, 0, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(serial_summary, 1);
    style_transparent(serial_summary);
    lv_obj_set_style_pad_column(serial_summary, _narrow_layout ? 12 : 24, 0);
    lv_obj_set_flex_flow(serial_summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        serial_summary, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(serial_summary, LV_OBJ_FLAG_SCROLLABLE);

    _baud_summary_label = make_label(
        serial_summary, "", _narrow_layout ? &lv_font_montserrat_20 : &lv_font_montserrat_22,
        COLOR_TEXT
    );
    make_label(
        serial_summary, "|", _narrow_layout ? &lv_font_montserrat_20 : &lv_font_montserrat_22,
        COLOR_BORDER
    );
    _format_summary_label = make_label(
        serial_summary, "", _narrow_layout ? &lv_font_montserrat_20 : &lv_font_montserrat_22,
        COLOR_TEXT
    );

    const char *settings_text = _narrow_layout ? LV_SYMBOL_SETTINGS : LV_SYMBOL_SETTINGS "  Settings";
    lv_obj_t *settings_btn = make_button(
        summary_row, settings_text, _narrow_layout ? 62 : 168,
        _compact_layout ? 56 : 62
    );
    lv_obj_add_event_cb(settings_btn, settings_btn_event_cb, LV_EVENT_CLICKED, this);
}

void UartTool::createConfigSection(lv_obj_t *parent)
{
    _settings_overlay = lv_obj_create(parent);
    lv_obj_set_size(_settings_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(_settings_overlay);
    lv_obj_set_style_bg_color(_settings_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_settings_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(_settings_overlay, 0, 0);
    lv_obj_set_style_pad_all(_settings_overlay, 0, 0);
    lv_obj_add_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(_settings_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(_settings_overlay);
    const int panel_h = std::min(
        680, static_cast<int>(lv_display_get_vertical_resolution(nullptr)) - 80
    );
    lv_obj_set_size(panel, std::min(_content_w, 640), panel_h);
    lv_obj_center(panel);
    style_card(panel, 22);
    lv_obj_set_style_pad_all(panel, 24, 0);
    lv_obj_set_style_pad_row(panel, 12, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *title_row = lv_obj_create(panel);
    lv_obj_set_size(title_row, LV_PCT(100), 58);
    style_transparent(title_row);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);
    make_label(title_row, "Serial Settings", &lv_font_montserrat_30, COLOR_TEXT);
    lv_obj_t *close_btn = make_button(title_row, LV_SYMBOL_CLOSE, 54, 54);
    lv_obj_add_event_cb(close_btn, settings_close_btn_event_cb, LV_EVENT_CLICKED, this);

    auto add_row = [&](const char *name) -> lv_obj_t * {
        lv_obj_t *row = lv_obj_create(panel);
        lv_obj_set_size(row, LV_PCT(100), _narrow_layout ? 116 : _row_h);
        style_transparent(row);
        lv_obj_set_style_pad_row(row, _narrow_layout ? 6 : 0, 0);
        lv_obj_set_flex_flow(row, _narrow_layout ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            row,
            _narrow_layout ? LV_FLEX_ALIGN_START : LV_FLEX_ALIGN_SPACE_BETWEEN,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER
        );
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        make_label(row, name, &lv_font_montserrat_24, COLOR_TEXT_MUTED);
        return row;
    };

    auto add_dropdown_row = [&](const char *name, const char *options) -> lv_obj_t * {
        lv_obj_t *row = add_row(name);
        lv_obj_t *dropdown = lv_dropdown_create(row);
        lv_dropdown_set_options_static(dropdown, options);
        const int dropdown_w = _narrow_layout ? LV_PCT(100) : std::min(300, std::max(250, _content_w - 160));
        lv_obj_set_size(dropdown, dropdown_w, 58);
        lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(dropdown, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_set_style_bg_color(dropdown, lv_color_hex(COLOR_TERMINAL), 0);
        lv_obj_set_style_border_color(dropdown, lv_color_hex(COLOR_BORDER), 0);
        lv_obj_set_style_border_width(dropdown, 1, 0);
        lv_obj_set_style_radius(dropdown, 10, 0);
        return dropdown;
    };

    _baud_dropdown = add_dropdown_row("Baud rate", BAUD_OPTIONS);
    _data_dropdown = add_dropdown_row("Data bits", DATA_BITS_OPTIONS);
    _stop_dropdown = add_dropdown_row("Stop bits", STOP_BITS_OPTIONS);
    _parity_dropdown = add_dropdown_row("Parity", PARITY_OPTIONS);

    lv_obj_t *apply_btn = make_button(panel, "Apply settings", LV_PCT(100), 68, true);
    lv_obj_add_event_cb(apply_btn, apply_btn_event_cb, LV_EVENT_CLICKED, this);

    loadConfigToWidgets();
}

void UartTool::createRxSection(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_PCT(100));
    if (_compact_layout) {
        lv_obj_set_height(card, 300);
    } else {
        lv_obj_set_height(card, 0);
        lv_obj_set_flex_grow(card, 1);
    }
    style_card(card);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_set_style_pad_row(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *heading = lv_obj_create(card);
    lv_obj_set_size(heading, LV_PCT(100), 42);
    style_transparent(heading);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(heading, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(heading, LV_OBJ_FLAG_SCROLLABLE);
    make_label(heading, "RECEIVE", &lv_font_montserrat_28, COLOR_PRIMARY);
    _rx_count_label = make_label(heading, "0 B", &lv_font_montserrat_22, COLOR_PRIMARY);

    _rx_scroll = lv_obj_create(card);
    lv_obj_set_width(_rx_scroll, LV_PCT(100));
    lv_obj_set_height(_rx_scroll, 0);
    lv_obj_set_flex_grow(_rx_scroll, 1);
    lv_obj_set_scroll_dir(_rx_scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_rx_scroll, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(_rx_scroll, lv_color_hex(COLOR_TERMINAL), 0);
    lv_obj_set_style_bg_opa(_rx_scroll, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_rx_scroll, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(_rx_scroll, 1, 0);
    lv_obj_set_style_radius(_rx_scroll, 14, 0);
    lv_obj_set_style_pad_all(_rx_scroll, 16, 0);

    _rx_label = lv_label_create(_rx_scroll);
    lv_obj_set_width(_rx_label, LV_PCT(100));
    lv_obj_set_height(_rx_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(_rx_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(_rx_label, "");
    lv_obj_set_style_text_color(_rx_label, lv_color_hex(COLOR_RX_TEXT), 0);
    lv_obj_set_style_text_font(_rx_label, &lv_font_montserrat_22, 0);

    lv_obj_t *toolbar = lv_obj_create(card);
    lv_obj_set_size(toolbar, LV_PCT(100), 62);
    style_transparent(toolbar);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *rx_modes = lv_obj_create(toolbar);
    lv_obj_set_size(rx_modes, _narrow_layout ? 176 : 230, 58);
    style_transparent(rx_modes);
    lv_obj_set_style_pad_column(rx_modes, 6, 0);
    lv_obj_set_flex_flow(rx_modes, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        rx_modes, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(rx_modes, LV_OBJ_FLAG_SCROLLABLE);

    _rx_ascii_mode_btn = make_button(rx_modes, "ASCII", _narrow_layout ? 96 : 126, 58);
    lv_obj_add_event_cb(_rx_ascii_mode_btn, rx_hex_switch_event_cb, LV_EVENT_CLICKED, this);
    _rx_hex_switch = make_button(rx_modes, "HEX", _narrow_layout ? 74 : 98, 58);
    lv_obj_add_event_cb(_rx_hex_switch, rx_hex_switch_event_cb, LV_EVENT_CLICKED, this);

    _auto_scroll_checkbox = lv_checkbox_create(toolbar);
    lv_checkbox_set_text(_auto_scroll_checkbox, "Auto scroll");
    lv_obj_add_state(_auto_scroll_checkbox, LV_STATE_CHECKED);
    lv_obj_set_style_text_font(_auto_scroll_checkbox, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(_auto_scroll_checkbox, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lv_obj_set_style_border_color(_auto_scroll_checkbox, lv_color_hex(COLOR_PRIMARY), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(
        _auto_scroll_checkbox,
        lv_color_hex(COLOR_PRIMARY),
        static_cast<lv_style_selector_t>(LV_PART_INDICATOR) |
        static_cast<lv_style_selector_t>(LV_STATE_CHECKED)
    );

    lv_obj_t *clear_btn = make_button(toolbar, "Clear", _narrow_layout ? 100 : 140, 58);
    lv_obj_add_event_cb(clear_btn, rx_clear_btn_event_cb, LV_EVENT_CLICKED, this);
}

void UartTool::createTxSection(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), _narrow_layout ? 314 : (_compact_layout ? 226 : 246));
    style_card(card);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_set_style_pad_row(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    make_label(card, "SEND", &lv_font_montserrat_28, COLOR_PRIMARY);

    lv_obj_t *input_row = lv_obj_create(card);
    lv_obj_set_size(input_row, LV_PCT(100), 76);
    style_transparent(input_row);
    lv_obj_set_style_pad_column(input_row, 14, 0);
    lv_obj_set_flex_flow(input_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(input_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(input_row, LV_OBJ_FLAG_SCROLLABLE);

    _tx_textarea = lv_textarea_create(input_row);
    lv_obj_set_width(_tx_textarea, 0);
    lv_obj_set_height(_tx_textarea, 72);
    lv_obj_set_flex_grow(_tx_textarea, 1);
    lv_textarea_set_one_line(_tx_textarea, true);
    lv_textarea_set_text(_tx_textarea, "ESP32-P4-WIFI6-DB-TOUCH-LCD-10.1 by Waveshare");
    lv_textarea_set_placeholder_text(_tx_textarea, _config.hex_tx ? "AA BB 01" : "Enter text or HEX...");
    lv_obj_set_style_bg_color(_tx_textarea, lv_color_hex(COLOR_TERMINAL), 0);
    lv_obj_set_style_border_color(_tx_textarea, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(_tx_textarea, 1, 0);
    lv_obj_set_style_radius(_tx_textarea, 12, 0);
    lv_obj_set_style_text_color(_tx_textarea, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_color(_tx_textarea, lv_color_hex(COLOR_TEXT_MUTED), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_font(_tx_textarea, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(_tx_textarea, &lv_font_montserrat_22, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_add_event_cb(_tx_textarea, tx_ta_event_cb, LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(_tx_textarea, tx_ta_event_cb, LV_EVENT_DEFOCUSED, this);
    lv_obj_add_event_cb(_tx_textarea, tx_ta_event_cb, LV_EVENT_READY, this);

    lv_obj_t *send_btn = make_button(input_row, "Send", _narrow_layout ? 112 : 150, 72, true);
    lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, this);

    lv_obj_t *modes = lv_obj_create(card);
    lv_obj_set_size(modes, LV_PCT(100), _narrow_layout ? 124 : 60);
    style_transparent(modes);
    lv_obj_set_style_pad_column(modes, 10, 0);
    lv_obj_set_flex_flow(modes, _narrow_layout ? LV_FLEX_FLOW_ROW_WRAP : LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        modes, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY
    );
    lv_obj_remove_flag(modes, LV_OBJ_FLAG_SCROLLABLE);

    _tx_text_mode_btn = make_button(modes, "Text", _narrow_layout ? 82 : 100, 56);
    lv_obj_add_event_cb(_tx_text_mode_btn, tx_hex_switch_event_cb, LV_EVENT_CLICKED, this);
    _tx_hex_switch = make_button(modes, "HEX", _narrow_layout ? 82 : 100, 56);
    lv_obj_add_event_cb(_tx_hex_switch, tx_hex_switch_event_cb, LV_EVENT_CLICKED, this);

    const char *ending_names[] = {"None", "CR", "LF", "CRLF"};
    for (int i = 0; i < 4; i++) {
        const int width = _narrow_layout ?
                          ((i == 0 || i == 3) ? 88 : 68) :
                          ((i == 0) ? 100 : ((i == 3) ? 108 : 82));
        _line_ending_buttons[i] = make_button(modes, ending_names[i], width, 56);
        lv_obj_add_event_cb(_line_ending_buttons[i], line_ending_btn_event_cb, LV_EVENT_CLICKED, this);
    }
}

void UartTool::createKeyboard()
{
    _keyboard = lv_keyboard_create(_screen);
    lv_keyboard_set_map(_keyboard, LV_KEYBOARD_MODE_USER_1, hex_kb_map, hex_kb_ctrl);
    lv_obj_set_size(_keyboard, LV_PCT(100), LV_PCT(38));
    lv_obj_set_style_bg_color(_keyboard, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_border_color(_keyboard, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(_keyboard, 1, 0);
    lv_obj_set_style_text_font(_keyboard, &lv_font_montserrat_24, 0);
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void UartTool::loadConfigToWidgets()
{
    if (_baud_dropdown == nullptr) {
        return;
    }
    lv_dropdown_set_selected(_baud_dropdown, baud_to_index(_config.baud));
    lv_dropdown_set_selected(_data_dropdown, _config.data_bits - UART_DATA_5_BITS);
    const int stop_index = (_config.stop_bits == UART_STOP_BITS_1) ? 0 :
                           (_config.stop_bits == UART_STOP_BITS_1_5) ? 1 : 2;
    lv_dropdown_set_selected(_stop_dropdown, stop_index);
    const int parity_index = (_config.parity == UART_PARITY_DISABLE) ? 0 :
                             (_config.parity == UART_PARITY_EVEN) ? 1 : 2;
    lv_dropdown_set_selected(_parity_dropdown, parity_index);
}

void UartTool::applyConfig()
{
    UartToolConfig new_config = _config;
    new_config.baud = baud_values[lv_dropdown_get_selected(_baud_dropdown)];
    new_config.data_bits = static_cast<uart_word_length_t>(
        UART_DATA_5_BITS + lv_dropdown_get_selected(_data_dropdown)
    );
    static const uart_stop_bits_t stop_map[] = {UART_STOP_BITS_1, UART_STOP_BITS_1_5, UART_STOP_BITS_2};
    new_config.stop_bits = stop_map[lv_dropdown_get_selected(_stop_dropdown)];
    static const uart_parity_t parity_map[] = {UART_PARITY_DISABLE, UART_PARITY_EVEN, UART_PARITY_ODD};
    new_config.parity = parity_map[lv_dropdown_get_selected(_parity_dropdown)];

    if (!commitConfig(new_config)) {
        loadConfigToWidgets();
        return;
    }

    lv_obj_add_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN);
}

bool UartTool::commitConfig(const UartToolConfig &config)
{
    const bool old_pins_ready = uart_tool_fixed_pins_ready(_config.mode);
    const bool new_pins_ready = uart_tool_fixed_pins_ready(config.mode);

    if (new_pins_ready) {
        if (!_service.start(config)) {
            const bool restored = !old_pins_ready || _service.start(_config);
            setStatus(restored ? "Apply failed" : "Serial error", COLOR_ERROR);
            return false;
        }
    } else {
        _service.stop();
    }

    _config = config;
    const bool saved = uart_tool_config_save(_config);
    updateConnectionSummary();
    updateModeButtons();
    if (saved) {
        updateConnectionStatus();
    } else {
        setStatus("Save failed", COLOR_ERROR);
    }
    return true;
}

void UartTool::setStatus(const char *text, uint32_t color)
{
    if (_status_label == nullptr || _status_dot == nullptr) {
        return;
    }
    lv_label_set_text(_status_label, text);
    lv_obj_set_style_text_color(_status_label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(color), 0);
}

void UartTool::updateConnectionStatus()
{
    if (!uart_tool_fixed_pins_ready(_config.mode)) {
        setStatus("Pins NC", COLOR_WARNING);
    } else if (_service.isRunning()) {
        setStatus("Ready", COLOR_SUCCESS);
    } else {
        setStatus("Serial error", COLOR_ERROR);
    }
}

void UartTool::updateConnectionSummary()
{
    if (_baud_summary_label == nullptr) {
        return;
    }
    char text[24];
    snprintf(text, sizeof(text), "%d baud", _config.baud);
    lv_label_set_text(_baud_summary_label, text);

    const int data_bits = static_cast<int>(_config.data_bits) - static_cast<int>(UART_DATA_5_BITS) + 5;
    const char parity = (_config.parity == UART_PARITY_EVEN) ? 'E' :
                        (_config.parity == UART_PARITY_ODD) ? 'O' : 'N';
    const char *stop = (_config.stop_bits == UART_STOP_BITS_1_5) ? "1.5" :
                       (_config.stop_bits == UART_STOP_BITS_2) ? "2" : "1";
    snprintf(text, sizeof(text), "%d%c%s", data_bits, parity, stop);
    lv_label_set_text(_format_summary_label, text);
}

void UartTool::updateModeButtons()
{
    const int serial_mode = static_cast<int>(_config.mode);
    for (int i = 0; i < 3; i++) {
        set_checked(_serial_mode_buttons[i], serial_mode == i);
    }
    set_checked(_rx_ascii_mode_btn, !_config.hex_rx);
    set_checked(_rx_hex_switch, _config.hex_rx);
    set_checked(_tx_text_mode_btn, !_config.hex_tx);
    set_checked(_tx_hex_switch, _config.hex_tx);
    for (int i = 0; i < 4; i++) {
        set_checked(_line_ending_buttons[i], _line_ending == i);
    }
}

void UartTool::renderRxText()
{
    if (_rx_label == nullptr || _rx_scroll == nullptr) {
        return;
    }
    if (_config.hex_rx) {
        uart_tool_bytes_to_hex(_rx_raw.data(), _rx_raw.size(), _rx_render_text);
    } else {
        uart_tool_bytes_to_ascii(_rx_raw.data(), _rx_raw.size(), _rx_render_text);
    }
    lv_label_set_text(_rx_label, _rx_render_text.c_str());

    char count[24];
    snprintf(count, sizeof(count), "%u B", static_cast<unsigned>(_rx_raw.size()));
    lv_label_set_text(_rx_count_label, count);

    if (_auto_scroll_checkbox != nullptr &&
        lv_obj_has_state(_auto_scroll_checkbox, LV_STATE_CHECKED)) {
        lv_obj_update_layout(_rx_scroll);
        lv_obj_scroll_to_y(_rx_scroll, LV_COORD_MAX, LV_ANIM_OFF);
    }
}

void UartTool::appendRxBytes(const uint8_t *data, size_t len)
{
    if (len >= RX_RAW_MAX) {
        _rx_raw.assign(data + len - RX_RAW_MAX, data + len);
        return;
    }
    const size_t required = _rx_raw.size() + len;
    if (required > RX_RAW_MAX) {
        const size_t discard = required - RX_RAW_MAX;
        const size_t keep = _rx_raw.size() - discard;
        memmove(_rx_raw.data(), _rx_raw.data() + discard, keep);
        _rx_raw.resize(keep);
    }
    _rx_raw.insert(_rx_raw.end(), data, data + len);
}

void UartTool::sendCurrentInput()
{
    if (!uart_tool_fixed_pins_ready(_config.mode)) {
        setStatus("Pins NC", COLOR_WARNING);
        return;
    }

    const char *text = lv_textarea_get_text(_tx_textarea);
    if (text == nullptr) {
        return;
    }

    std::vector<uint8_t> bytes;
    if (_config.hex_tx) {
        if (!uart_tool_hex_to_bytes(text, bytes)) {
            setStatus("Bad HEX", COLOR_ERROR);
            return;
        }
    } else {
        bytes.assign(text, text + strlen(text));
    }

    if (_line_ending == 1 || _line_ending == 3) {
        bytes.push_back('\r');
    }
    if (_line_ending == 2 || _line_ending == 3) {
        bytes.push_back('\n');
    }
    if (bytes.empty()) {
        return;
    }
    if (!_service.send(bytes.data(), bytes.size())) {
        setStatus("Send failed", COLOR_ERROR);
        return;
    }
    setStatus("Queued", COLOR_SUCCESS);
}

void UartTool::settings_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->loadConfigToWidgets();
    lv_obj_remove_flag(self->_settings_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(self->_settings_overlay);
}

void UartTool::settings_close_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    lv_obj_add_flag(self->_settings_overlay, LV_OBJ_FLAG_HIDDEN);
}

void UartTool::apply_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->applyConfig();
}

void UartTool::serial_mode_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));

    for (int i = 0; i < 3; i++) {
        if (target != self->_serial_mode_buttons[i]) {
            continue;
        }

        const UartToolMode mode = static_cast<UartToolMode>(i);
        if (mode == self->_config.mode) {
            return;
        }

        UartToolConfig new_config = self->_config;
        new_config.mode = mode;
        if (!self->commitConfig(new_config)) {
            self->updateModeButtons();
        }
        return;
    }
}

void UartTool::rx_timer_cb(lv_timer_t *timer)
{
    auto *self = static_cast<UartTool *>(lv_timer_get_user_data(timer));
    uint8_t chunk[RX_DRAIN_CHUNK];
    size_t drained = 0;
    while (drained < RX_DRAIN_BUDGET) {
        const size_t n = self->_service.readRx(chunk, sizeof(chunk));
        if (n == 0) {
            break;
        }
        self->appendRxBytes(chunk, n);
        drained += n;
        if (n < sizeof(chunk)) {
            break;
        }
    }
    if (drained > 0) {
        self->renderRxText();
    }
}

void UartTool::rx_hex_switch_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
    self->_config.hex_rx = (target == self->_rx_hex_switch);
    uart_tool_config_save(self->_config);
    self->updateModeButtons();
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
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
    self->_config.hex_tx = (target == self->_tx_hex_switch);
    uart_tool_config_save(self->_config);
    lv_textarea_set_placeholder_text(
        self->_tx_textarea, self->_config.hex_tx ? "AA BB 01" : "Enter text or HEX..."
    );
    self->updateModeButtons();
}

void UartTool::line_ending_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
    for (int i = 0; i < 4; i++) {
        if (target == self->_line_ending_buttons[i]) {
            self->_line_ending = static_cast<uint8_t>(i);
            break;
        }
    }
    self->updateModeButtons();
}

void UartTool::send_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    self->sendCurrentInput();
}

void UartTool::tx_ta_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    const lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(self->_keyboard, self->_tx_textarea);
        lv_keyboard_set_mode(
            self->_keyboard,
            self->_config.hex_tx ? LV_KEYBOARD_MODE_USER_1 : LV_KEYBOARD_MODE_TEXT_LOWER
        );
        lv_obj_remove_flag(self->_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(self->_keyboard);
        lv_obj_align(self->_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    } else {
        lv_keyboard_set_textarea(self->_keyboard, nullptr);
        lv_obj_add_flag(self->_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void UartTool::screen_size_changed_event_cb(lv_event_t *e)
{
    auto *self = static_cast<UartTool *>(lv_event_get_user_data(e));
    if (self == nullptr || self->_screen == nullptr || self->_layout_update_pending) {
        return;
    }

    self->_layout_update_pending = true;
    if (lv_async_call(layout_update_async_cb, self) != LV_RESULT_OK) {
        self->_layout_update_pending = false;
    }
}

void UartTool::layout_update_async_cb(void *user_data)
{
    auto *self = static_cast<UartTool *>(user_data);
    if (self == nullptr) {
        return;
    }
    self->_layout_update_pending = false;
    if (self->_screen == nullptr) {
        return;
    }

    self->rebuildUi(
        lv_obj_get_width(self->_screen),
        lv_obj_get_height(self->_screen)
    );
}

bool UartTool::back(void)
{
    if (_settings_overlay != nullptr &&
        !lv_obj_has_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN);
        return true;
    }
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
    if (_rx_timer != nullptr) {
        lv_timer_pause(_rx_timer);
    }
    return true;
}

bool UartTool::resume(void)
{
    ESP_UTILS_LOGD("Resume(@0x%p)", this);
    if (_rx_timer != nullptr) {
        lv_timer_resume(_rx_timer);
    }
    return true;
}

bool UartTool::cleanResource(void)
{
    ESP_UTILS_LOGD("CleanResource(@0x%p)", this);
    lv_async_call_cancel(layout_update_async_cb, this);
    _layout_update_pending = false;
    if (_screen != nullptr) {
        lv_obj_remove_event_cb_with_user_data(
            _screen, screen_size_changed_event_cb, this
        );
    }
    _service.stop();
    _rx_raw.clear();
    _rx_raw.shrink_to_fit();
    _rx_render_text.clear();
    _rx_render_text.shrink_to_fit();
    clearUiPointers();
    _screen = nullptr;
    _layout_w = 0;
    _layout_h = 0;
    _rx_timer = nullptr;
    return true;
}

// The app is a singleton owned by itself; the registry must not delete it.
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, UartTool, APP_NAME, []()
{
    return std::shared_ptr<UartTool>(UartTool::requestInstance(), [](UartTool *) {});
})

} // namespace esp_brookesia::apps
