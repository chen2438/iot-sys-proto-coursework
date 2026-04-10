import csv
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


BASE = Path(__file__).resolve().parent
CSV_PATH = BASE / "mcu_latency_template.csv"
WIDTH = 1200
HEIGHT = 800
MARGIN_LEFT = 120
MARGIN_RIGHT = 60
MARGIN_TOP = 80
MARGIN_BOTTOM = 120


def load_rows():
    rows = []
    with CSV_PATH.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(
                {
                    "model_name": row["model_name"],
                    "tflite_acc": float(row["test_accuracy_tflite_pct"]),
                    "size": int(row["quant_tflite_size_bytes"]),
                    "lat_avg": float(row["latency_us_avg"]),
                }
            )
    return rows


def get_font(size):
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttc",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


FONT_TITLE = get_font(34)
FONT_LABEL = get_font(24)
FONT_TEXT = get_font(20)
FONT_SMALL = get_font(18)


def nice_padding(values, frac=0.08):
    low = min(values)
    high = max(values)
    spread = high - low
    pad = max(spread * frac, 1)
    return low - pad, high + pad


def map_point(x, y, x_min, x_max, y_min, y_max):
    plot_w = WIDTH - MARGIN_LEFT - MARGIN_RIGHT
    plot_h = HEIGHT - MARGIN_TOP - MARGIN_BOTTOM
    px = MARGIN_LEFT + (x - x_min) / (x_max - x_min) * plot_w
    py = HEIGHT - MARGIN_BOTTOM - (y - y_min) / (y_max - y_min) * plot_h
    return px, py


def draw_axes(draw, title, x_label, y_label):
    plot_left = MARGIN_LEFT
    plot_right = WIDTH - MARGIN_RIGHT
    plot_top = MARGIN_TOP
    plot_bottom = HEIGHT - MARGIN_BOTTOM

    draw.rectangle((0, 0, WIDTH, HEIGHT), fill="white")
    draw.line((plot_left, plot_bottom, plot_right, plot_bottom), fill="black", width=3)
    draw.line((plot_left, plot_bottom, plot_left, plot_top), fill="black", width=3)
    draw.text((WIDTH / 2 - 180, 24), title, fill="black", font=FONT_TITLE)
    draw.text((WIDTH / 2 - 150, HEIGHT - 60), x_label, fill="black", font=FONT_LABEL)
    draw.text((20, 28), y_label, fill="black", font=FONT_LABEL)
    return plot_left, plot_right, plot_top, plot_bottom


def draw_grid_and_ticks(draw, x_ticks, y_ticks, x_range, y_range):
    x_min, x_max = x_range
    y_min, y_max = y_range
    plot_left = MARGIN_LEFT
    plot_right = WIDTH - MARGIN_RIGHT
    plot_top = MARGIN_TOP
    plot_bottom = HEIGHT - MARGIN_BOTTOM

    for tick in x_ticks:
        px, _ = map_point(tick, y_min, x_min, x_max, y_min, y_max)
        draw.line((px, plot_bottom, px, plot_top), fill="#d9d9d9", width=1)
        draw.text((px - 25, plot_bottom + 15), f"{int(tick)}", fill="black", font=FONT_SMALL)
    for tick in y_ticks:
        _, py = map_point(x_min, tick, x_min, x_max, y_min, y_max)
        draw.line((plot_left, py, plot_right, py), fill="#d9d9d9", width=1)
        draw.text((40, py - 10), f"{tick:.0f}", fill="black", font=FONT_SMALL)


def draw_series(draw, rows, x_key, y_key, x_range, y_range, point_color, line_color):
    x_min, x_max = x_range
    y_min, y_max = y_range

    pts = [map_point(row[x_key], row[y_key], x_min, x_max, y_min, y_max) for row in rows]
    for i in range(len(pts) - 1):
        draw.line((pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1]), fill=line_color, width=3)

    for (px, py), row in zip(pts, rows):
        draw.ellipse((px - 7, py - 7, px + 7, py + 7), fill=point_color, outline="black")
        draw.text((px + 10, py - 24), row["model_name"], fill="black", font=FONT_TEXT)


def create_plot(rows, x_key, y_key, title, x_label, out_name, point_color, line_color, x_ticks):
    y_values = [row[y_key] for row in rows]
    x_values = [row[x_key] for row in rows]
    x_range = nice_padding(x_values)
    y_range = nice_padding(y_values)

    image = Image.new("RGB", (WIDTH, HEIGHT), "white")
    draw = ImageDraw.Draw(image)
    draw_axes(draw, title, x_label, "Quantized test accuracy (%)")
    y_ticks = [70, 75, 80, 85, 90]
    draw_grid_and_ticks(draw, x_ticks, y_ticks, x_range, y_range)
    draw_series(draw, rows, x_key, y_key, x_range, y_range, point_color, line_color)
    image.save(BASE / out_name)


def main():
    rows = load_rows()
    create_plot(
        rows,
        x_key="size",
        y_key="tflite_acc",
        title="Accuracy vs Model Size",
        x_label="Quantized model size (bytes)",
        out_name="accuracy_vs_model_size.png",
        point_color="#0b6e4f",
        line_color="#8ecae6",
        x_ticks=[5000, 7500, 10000, 12500, 15000],
    )
    create_plot(
        rows,
        x_key="lat_avg",
        y_key="tflite_acc",
        title="Accuracy vs MCU Latency",
        x_label="Average MCU latency (us)",
        out_name="accuracy_vs_mcu_latency.png",
        point_color="#bc4b51",
        line_color="#f4a261",
        x_ticks=[40000, 50000, 60000, 70000, 80000],
    )


if __name__ == "__main__":
    main()
