"""Generate assets/img_app_uart_tool.c (LVGL9 ARGB8888, byte order B,G,R,A)."""
import os

SIZE = 112
MARGIN = 4
RADIUS = 24
BG = (0x1E, 0x88, 0xE5)   # R, G, B
FG = (0xFF, 0xFF, 0xFF)


def in_round_rect(x, y):
    if x < MARGIN or y < MARGIN or x >= SIZE - MARGIN or y >= SIZE - MARGIN:
        return False
    cx = min(x - MARGIN, SIZE - 1 - MARGIN - x)
    cy = min(y - MARGIN, SIZE - 1 - MARGIN - y)
    if cx < RADIUS and cy < RADIUS:
        dx, dy = RADIUS - cx, RADIUS - cy
        return dx * dx + dy * dy <= RADIUS * RADIUS
    return True


def in_tx_glyph(x, y):
    # Up arrow head at x-center 38, apex y=26, base y=54, half-width -> 16
    if 26 <= y <= 54 and abs(x - 38) <= (y - 26) * 16 // 28:
        return True
    # Stem
    return 34 <= x <= 42 and 54 < y <= 86


def in_rx_glyph(x, y):
    # Down arrow head at x-center 74, base y=58, apex y=86
    if 58 <= y <= 86 and abs(x - 74) <= (86 - y) * 16 // 28:
        return True
    # Stem
    return 70 <= x <= 78 and 26 <= y < 58


def pixel(x, y):
    if not in_round_rect(x, y):
        return (0, 0, 0, 0)
    r, g, b = FG if (in_tx_glyph(x, y) or in_rx_glyph(x, y)) else BG
    return (b, g, r, 0xFF)


out_path = os.path.join(os.path.dirname(__file__), "..", "assets", "img_app_uart_tool.c")
os.makedirs(os.path.dirname(out_path), exist_ok=True)
with open(out_path, "w", newline="\n") as f:
    f.write(
        "#ifdef __has_include\n"
        "    #if __has_include(\"lvgl.h\")\n"
        "        #ifndef LV_LVGL_H_INCLUDE_SIMPLE\n"
        "            #define LV_LVGL_H_INCLUDE_SIMPLE\n"
        "        #endif\n"
        "    #endif\n"
        "#endif\n\n"
        "#if defined(LV_LVGL_H_INCLUDE_SIMPLE)\n"
        "    #include \"lvgl.h\"\n"
        "#else\n"
        "    #include \"lvgl/lvgl.h\"\n"
        "#endif\n\n"
        "#ifndef LV_ATTRIBUTE_MEM_ALIGN\n"
        "#define LV_ATTRIBUTE_MEM_ALIGN\n"
        "#endif\n\n"
        "const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t img_app_uart_tool_map[] = {\n"
    )
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            row += ["0x%02x" % v for v in pixel(x, y)]
        f.write("  " + ", ".join(row) + ",\n")
    f.write(
        "};\n\n"
        "const lv_image_dsc_t img_app_uart_tool = {\n"
        "  .header.cf = LV_COLOR_FORMAT_ARGB8888,\n"
        "  .header.magic = LV_IMAGE_HEADER_MAGIC,\n"
        "  .header.w = 112,\n"
        "  .header.h = 112,\n"
        "  .data_size = 12544 * 4,\n"
        "  .data = img_app_uart_tool_map,\n"
        "};\n"
    )
print("written:", out_path)
