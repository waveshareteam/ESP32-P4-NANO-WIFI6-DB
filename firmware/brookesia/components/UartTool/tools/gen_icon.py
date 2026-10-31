"""Generate the original, geometry-only UART launcher icon."""

from pathlib import Path
import sys


BROOKESIA_DIR = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(BROOKESIA_DIR / "tools"))

from icon_renderer import (  # noqa: E402
    Canvas,
    highlighted_gradient,
    vertical_gradient,
    write_lvgl_argb8888_c,
    write_rgba_png,
)


SIZE = 112
SYMBOL = "img_app_uart_tool"
ATTRIBUTE = "LV_ATTRIBUTE_IMAGE_IMG_APP_UART_TOOL"
COMPONENT_DIR = Path(__file__).resolve().parent.parent
PNG_PATH = COMPONENT_DIR / "assets" / f"{SYMBOL}.png"
C_PATH = COMPONENT_DIR / "assets" / f"{SYMBOL}.c"


def render_icon() -> bytes:
    canvas = Canvas(size=SIZE, scale=4)

    canvas.rounded_rect(
        (4, 4, 108, 108),
        24,
        highlighted_gradient(
            (82, 163, 229, 255),
            (42, 82, 126, 255),
            4,
            108,
            (30, 19),
            78,
            15,
        ),
    )

    shell = (
        (22, 39),
        (34, 32),
        (78, 32),
        (90, 39),
        (96, 46),
        (96, 66),
        (90, 73),
        (78, 79),
        (34, 79),
        (22, 73),
        (16, 66),
        (16, 46),
    )
    canvas.rounded_rect((15, 39, 97, 83), 13, (8, 18, 30, 48))
    canvas.polygon(
        shell,
        vertical_gradient(
            (255, 255, 255, 255),
            (210, 218, 225, 255),
            32,
            79,
        ),
    )

    face = (
        (32, 39),
        (80, 39),
        (86, 45),
        (82, 68),
        (77, 73),
        (35, 73),
        (30, 68),
        (26, 45),
    )
    canvas.polygon(face, (29, 36, 43, 255))
    canvas.polygon(
        (
            (33, 41),
            (79, 41),
            (83, 45),
            (80, 66),
            (76, 70),
            (36, 70),
            (32, 66),
            (29, 45),
        ),
        vertical_gradient(
            (56, 64, 73, 255),
            (30, 36, 43, 255),
            41,
            70,
        ),
    )

    for center_x in (22, 90):
        canvas.ellipse(
            (center_x - 7, 49, center_x + 7, 63),
            vertical_gradient(
                (70, 79, 88, 255),
                (23, 29, 35, 255),
                49,
                63,
            ),
        )
        canvas.ellipse((center_x - 3.5, 52.5, center_x + 3.5, 59.5), (11, 16, 21, 255))

    pin_positions = [
        *((x, 50) for x in (38, 47, 56, 65, 74)),
        *((x, 62) for x in (42.5, 51.5, 60.5, 69.5)),
    ]
    for center_x, center_y in pin_positions:
        canvas.ellipse(
            (center_x - 3.1, center_y - 2.2, center_x + 3.1, center_y + 4.0),
            (6, 11, 16, 95),
        )
        canvas.ellipse(
            (center_x - 2.8, center_y - 2.8, center_x + 2.8, center_y + 2.8),
            vertical_gradient(
                (255, 255, 255, 255),
                (203, 211, 218, 255),
                center_y - 2.8,
                center_y + 2.8,
            ),
        )

    return canvas.rgba_bytes()


def main() -> None:
    rgba = render_icon()
    write_rgba_png(PNG_PATH, SIZE, SIZE, rgba)
    write_lvgl_argb8888_c(C_PATH, SYMBOL, ATTRIBUTE, SIZE, SIZE, rgba)
    print(f"written: {PNG_PATH}")
    print(f"written: {C_PATH}")


if __name__ == "__main__":
    main()
