/*
 * GPIO control through the Arduino Console library.
 *
 * Commands:
 *   gpio read <pin>
 *   gpio write <pin> <0|1>
 *   gpio mode <pin> <in|out|in_pu|in_pd>
 */

#include <Arduino.h>
#include <Console.h>
#include "argtable3/argtable3.h"

#define RGB_BRIGHTNESS 64

static struct {
  struct arg_int *pin;
  struct arg_end *end;
} gpio_read_args;

static struct {
  struct arg_int *pin;
  struct arg_int *value;
  struct arg_end *end;
} gpio_write_args;

static struct {
  struct arg_int *pin;
  struct arg_str *mode;
  struct arg_end *end;
} gpio_mode_args;

static int cmd_gpio_read(int argc, char **argv) {
  const int error_count = arg_parse(argc, argv, (void **)&gpio_read_args);
  if (error_count != 0) {
    arg_print_errors(stderr, gpio_read_args.end, argv[0]);
    return 1;
  }

  const int pin = gpio_read_args.pin->ival[0];
  const int value = digitalRead(pin);
  Serial.printf("GPIO %d = %d (%s)\n", pin, value, value ? "HIGH" : "LOW");
  return 0;
}

static int cmd_gpio_write(int argc, char **argv) {
  const int error_count = arg_parse(argc, argv, (void **)&gpio_write_args);
  if (error_count != 0) {
    arg_print_errors(stderr, gpio_write_args.end, argv[0]);
    return 1;
  }

  const int pin = gpio_write_args.pin->ival[0];
  const int value = gpio_write_args.value->ival[0];
  if ((value != 0) && (value != 1)) {
    Serial.println("gpio write: value must be 0 or 1");
    return 1;
  }

  digitalWrite(pin, value);
  Serial.printf("GPIO %d set to %d (%s)\n", pin, value, value ? "HIGH" : "LOW");
  return 0;
}

static int cmd_gpio_mode(int argc, char **argv) {
  const int error_count = arg_parse(argc, argv, (void **)&gpio_mode_args);
  if (error_count != 0) {
    arg_print_errors(stderr, gpio_mode_args.end, argv[0]);
    return 1;
  }

  const int pin = gpio_mode_args.pin->ival[0];
  const char *mode_name = gpio_mode_args.mode->sval[0];
  uint8_t mode = INPUT;

  if (strcmp(mode_name, "in") == 0) {
    mode = INPUT;
  } else if (strcmp(mode_name, "out") == 0) {
    mode = OUTPUT;
  } else if (strcmp(mode_name, "in_pu") == 0) {
    mode = INPUT_PULLUP;
  } else if (strcmp(mode_name, "in_pd") == 0) {
    mode = INPUT_PULLDOWN;
  } else {
    Serial.printf("gpio mode: unknown mode '%s'. Use: in, out, in_pu, in_pd\n", mode_name);
    return 1;
  }

  pinMode(pin, mode);
  Serial.printf("GPIO %d mode set to %s\n", pin, mode_name);
  return 0;
}

static int cmd_gpio(int argc, char **argv) {
  if (argc < 2) {
    Serial.println("Usage: gpio <read|write|mode> ...");
    Serial.println("  gpio read  <pin>");
    Serial.println("  gpio write <pin> <0|1>");
    Serial.println("  gpio mode  <pin> <in|out|in_pu|in_pd>");
    return 1;
  }

  if (strcmp(argv[1], "read") == 0) {
    return cmd_gpio_read(argc - 1, argv + 1);
  }
  if (strcmp(argv[1], "write") == 0) {
    return cmd_gpio_write(argc - 1, argv + 1);
  }
  if (strcmp(argv[1], "mode") == 0) {
    return cmd_gpio_mode(argc - 1, argv + 1);
  }

  Serial.printf("gpio: unknown sub-command '%s'\n", argv[1]);
  return 1;
}

#ifdef LED_BUILTIN
#ifdef RGB_BUILTIN
struct NamedColor {
  const char *name;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

static const NamedColor named_colors[] = {
  {"red", RGB_BRIGHTNESS, 0, 0},
  {"green", 0, RGB_BRIGHTNESS, 0},
  {"blue", 0, 0, RGB_BRIGHTNESS},
  {"white", RGB_BRIGHTNESS, RGB_BRIGHTNESS, RGB_BRIGHTNESS},
  {"yellow", RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0},
};
#else
static bool led_initialized = false;

static int get_led_level(bool on) {
#ifdef LED_BUILTIN_ACTIVE
  return on ? LED_BUILTIN_ACTIVE : !LED_BUILTIN_ACTIVE;
#else
  return on ? HIGH : LOW;
#endif
}
#endif

static void print_led_usage() {
#ifdef RGB_BUILTIN
  Serial.println("Usage: led <on [color]|off>");
  Serial.println("  Colors: red, green, blue, white, yellow");
#else
  Serial.println("Usage: led <on|off>");
#endif
}

static int cmd_led(int argc, char **argv) {
  if (argc < 2) {
    print_led_usage();
    return 1;
  }

#ifdef RGB_BUILTIN
  if (strcmp(argv[1], "on") == 0) {
    const char *color_name = (argc > 2) ? argv[2] : "white";
    for (const auto &color : named_colors) {
      if (strcasecmp(color_name, color.name) == 0) {
        rgbLedWrite(RGB_BUILTIN, color.red, color.green, color.blue);
        Serial.printf("RGB LED on: %s\n", color.name);
        return 0;
      }
    }
    Serial.printf("Unknown color '%s'\n", color_name);
    return 1;
  }
  if (strcmp(argv[1], "off") == 0) {
    rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
    Serial.println("RGB LED off");
    return 0;
  }
#else
  if (strcmp(argv[1], "on") == 0) {
    if (!led_initialized) {
      Console.run(String("gpio mode ") + LED_BUILTIN + " out");
      led_initialized = true;
    }
    return Console.run(String("gpio write ") + LED_BUILTIN + " " + get_led_level(true));
  }
  if (strcmp(argv[1], "off") == 0) {
    if (!led_initialized) {
      Console.run(String("gpio mode ") + LED_BUILTIN + " out");
      led_initialized = true;
    }
    return Console.run(String("gpio write ") + LED_BUILTIN + " " + get_led_level(false));
  }
#endif

  print_led_usage();
  return 1;
}
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  gpio_read_args.pin = arg_int1(nullptr, nullptr, "<pin>", "GPIO pin number to read");
  gpio_read_args.end = arg_end(1);

  gpio_write_args.pin = arg_int1(nullptr, nullptr, "<pin>", "GPIO pin number to write");
  gpio_write_args.value = arg_int1(nullptr, nullptr, "<0|1>", "0 for LOW or 1 for HIGH");
  gpio_write_args.end = arg_end(2);

  gpio_mode_args.pin = arg_int1(nullptr, nullptr, "<pin>", "GPIO pin number to configure");
  gpio_mode_args.mode = arg_str1(nullptr, nullptr, "<in|out|in_pu|in_pd>", "GPIO mode");
  gpio_mode_args.end = arg_end(2);

  Console.setPrompt("gpio> ");
  if (!Console.begin()) {
    Serial.println("Console init failed");
    return;
  }

  Console.addCmd("gpio", "Control GPIO pins", "<read|write|mode> ...", cmd_gpio);
#ifdef LED_BUILTIN
  Console.addCmd("led", "Toggle LED_BUILTIN", "<on|off>", cmd_led);
#endif
  Console.addHelpCmd();
  Console.attachToSerial(true);
}

void loop() {
  vTaskDelete(nullptr);
}
