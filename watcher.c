#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <Winuser.h>
#include "hidapi.h"
#else
#include <unistd.h>
#include <hidapi/hidapi.h>
#define Sleep(x) usleep((x)*1000)
#endif

#define VENDOR_ID  0x0C45
#define PRODUCT_ID 0xFDFD

#ifdef _WIN32
#define PAGE_COMMAND 0xFF60
#define PAGE_STATUS  0xFFFF
#else
#define PAGE_COMMAND 0xFF60
#define PAGE_STATUS  0xFFFF
#endif

typedef enum {
    STATE_UNKNOWN,
    STATE_CONNECTED,
    STATE_DISCONNECTED
} LinkState;

typedef enum {
    STATIC = 0x1,
    SINGLE_ON = 0x2,
    SINGLE_OFF = 0x3,
    GLITTERING = 0x4,
    FALLING = 0x5,
    COLOURFUL = 0x6,
    BREATH = 0x7,
    SPECTRUM = 0x8,
    OUTWARD = 0x9,
    SCROLLING = 0xA,
    ROLLING = 0xB,
    ROTATING = 0xC,
    EXPLODE = 0xD,
    LAUNCH = 0xE,
    RIPPLES = 0xF,
    FLOWING = 0x10,
    PULSATING = 0x11,
    TILT = 0x12,
    SHUTTLE = 0x13,
    LED_OFF = 0x0
} LightEffectType;

LinkState current_state = STATE_UNKNOWN;

typedef struct S_Keyboard {
    LightEffectType effect;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t colorful;
    uint8_t brightness;
    uint8_t speed;
    uint8_t direction;
    uint32_t available;
} KeyboardLights;

#define HEADER_1 0
#define HEADER_2 1
#define HEADER_3 2
#define STYLE 3
#define RED 4
#define GREEN 5
#define BLUE 6
#define COLORFUL 11
#define BRIGHTNESS 12
#define SPEED 13
#define DIRECTION 14
#define MARKER_0 17
#define MARKER_1 18
#define XOR_CHECKSUM 31

#define FLAG_RED (1 << RED)
#define FLAG_GREEN (1 << GREEN)
#define FLAG_BLUE (1 << BLUE)
#define FLAG_COLORFUL (1 << COLORFUL)
#define FLAG_BRIGHTNESS (1 << BRIGHTNESS)
#define FLAG_SPEED (1 << SPEED)
#define FLAG_DIRECTION (1 << DIRECTION)

#define DEFAULT_RED 0xFF
#define DEFAULT_GREEN 0xFF
#define DEFAULT_BLUE 0xFF
#define DEFAULT_COLORFUL 0x01
#define DEFAULT_BRIGHTNESS 0x05
#define DEFAULT_SPEED 0x03
#define DEFAULT_DIRECTION 0x00

#define MIN_RGB 0x00
#define MAX_RGB 0xFF

#define MIN_COLORFUL 0x00
#define MAX_COLORFUL 0x01

#define MIN_BRIGHTNESS 0x01
#define MAX_BRIGHTNESS 0x05

#define MIN_SPEED 0x01
#define MAX_SPEED 0x05

#define MIN_DIRECTION 0x00
#define MAX_DIRECTION 0x03

#define STATIC_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS)
#define SINGLE_ON_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define SINGLE_OFF_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define GLITTERING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define FALLING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define COLOURFUL_AVAILABLE  (FLAG_BRIGHTNESS | FLAG_SPEED)
#define BREATH_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define SPECTRUM_AVAILABLE (FLAG_BRIGHTNESS | FLAG_SPEED)
#define OUTWARD_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define SCROLLING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION)
#define ROLLING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION)
#define ROTATING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION)
#define EXPLODE_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define LAUNCH_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define RIPPLES_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define FLOWING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION)
#define PULSATING_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define TILT_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION)
#define SHUTTLE_AVAILABLE (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED)
#define LED_OFF_AVAILABLE (0)

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define PACKET_SIZE 33
#define HID_REPORT_ID 0x00
#define LIGHT_HEADER_BYTE_1 0x05
#define LIGHT_HEADER_BYTE_2 0x10
#define LIGHT_HEADER_BYTE_3 0x00
#define CMD_MARKER_0_VAL 0xAA
#define CMD_MARKER_1_VAL 0x55

typedef uint8_t Packet[PACKET_SIZE];

unsigned char CMD_INIT[PACKET_SIZE] = {
    HID_REPORT_ID, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
};

unsigned char CMD_PING[PACKET_SIZE] = {
    HID_REPORT_ID, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21
};

unsigned char CMD_RIPPLE[PACKET_SIZE] = {
    HID_REPORT_ID, LIGHT_HEADER_BYTE_1, LIGHT_HEADER_BYTE_2, LIGHT_HEADER_BYTE_3,
    RIPPLES, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x5, 0x4, 0x00, 0x00, 0x00,
    CMD_MARKER_0_VAL, CMD_MARKER_1_VAL, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A
};


void set_layout(int layout_id) {
#ifdef _WIN32
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return;

    char hkl_string[9];
    sprintf(hkl_string, "0000%04x", layout_id);

    HKL new_hkl = LoadKeyboardLayoutA(hkl_string, KLF_ACTIVATE | KLF_SUBSTITUTE_OK);

    if (new_hkl)
        PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)new_hkl);
#else
    (void) layout_id;
    // Linux implementation to be added later
#endif
}

void set_effect(KeyboardLights *keyboard, LightEffectType effect) {
    keyboard->effect = effect;
    switch (effect) {
        case STATIC: keyboard->available = STATIC_AVAILABLE; break;
        case SINGLE_ON: keyboard->available = SINGLE_ON_AVAILABLE; break;
        case SINGLE_OFF: keyboard->available = SINGLE_OFF_AVAILABLE; break;
        case GLITTERING: keyboard->available = GLITTERING_AVAILABLE; break;
        case FALLING: keyboard->available = FALLING_AVAILABLE; break;
        case COLOURFUL: keyboard->available = COLOURFUL_AVAILABLE; break;
        case BREATH: keyboard->available = BREATH_AVAILABLE; break;
        case SPECTRUM: keyboard->available = SPECTRUM_AVAILABLE; break;
        case OUTWARD: keyboard->available = OUTWARD_AVAILABLE; break;
        case SCROLLING: keyboard->available = SCROLLING_AVAILABLE; break;
        case ROLLING: keyboard->available = ROLLING_AVAILABLE; break;
        case ROTATING: keyboard->available = ROTATING_AVAILABLE; break;
        case EXPLODE: keyboard->available = EXPLODE_AVAILABLE; break;
        case LAUNCH: keyboard->available = LAUNCH_AVAILABLE; break;
        case RIPPLES: keyboard->available = RIPPLES_AVAILABLE; break;
        case FLOWING: keyboard->available = FLOWING_AVAILABLE; break;
        case PULSATING: keyboard->available = PULSATING_AVAILABLE; break;
        case TILT: keyboard->available = TILT_AVAILABLE; break;
        case SHUTTLE: keyboard->available = SHUTTLE_AVAILABLE; break;
        case LED_OFF: keyboard->available = LED_OFF_AVAILABLE; break;
        default: keyboard->available = 0; break;
    }
}

void set_rgb(KeyboardLights *keyboard, uint8_t red, uint8_t green, uint8_t blue) {
    keyboard->red = CLAMP(red, MIN_RGB, MAX_RGB);
    keyboard->green = CLAMP(green, MIN_RGB, MAX_RGB);
    keyboard->blue = CLAMP(blue, MIN_RGB, MAX_RGB);
}

void set_colorful(KeyboardLights *keyboard, uint8_t colorful) {
    keyboard->colorful = CLAMP(colorful, MIN_COLORFUL, MAX_COLORFUL);
}

void set_brightness(KeyboardLights *keyboard, uint8_t brightness) {
    keyboard->brightness = CLAMP(brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
}

void set_speed(KeyboardLights *keyboard, uint8_t speed) {
    keyboard->speed = CLAMP(speed, MIN_SPEED, MAX_SPEED);
}

void set_direction(KeyboardLights *keyboard, uint8_t direction) {
    keyboard->direction = CLAMP(direction, MIN_DIRECTION, MAX_DIRECTION);
}

void keyboard_light_to_packet(KeyboardLights *k, Packet p) {
    memset(p, 0, PACKET_SIZE);

    p[0] = HID_REPORT_ID;
    p[1 + HEADER_1] = LIGHT_HEADER_BYTE_1;
    p[1 + HEADER_2] = LIGHT_HEADER_BYTE_2;
    p[1 + HEADER_3] = LIGHT_HEADER_BYTE_3;

    p[1 + STYLE] = k->effect;
    p[1 + RED] = (k->available & FLAG_RED) ? k->red : DEFAULT_RED;
    p[1 + GREEN] = (k->available & FLAG_GREEN) ? k->green : DEFAULT_GREEN;
    p[1 + BLUE] = (k->available & FLAG_BLUE) ? k->blue : DEFAULT_BLUE;
    p[1 + COLORFUL] = (k->available & FLAG_COLORFUL) ? k->colorful : DEFAULT_COLORFUL;
    p[1 + BRIGHTNESS] = (k->available & FLAG_BRIGHTNESS) ? k->brightness : DEFAULT_BRIGHTNESS;
    p[1 + SPEED] = (k->available & FLAG_SPEED) ? k->speed : DEFAULT_SPEED;
    p[1 + DIRECTION] = (k->available & FLAG_DIRECTION) ? k->direction : DEFAULT_DIRECTION;

    p[1 + MARKER_0] = CMD_MARKER_0_VAL;
    p[1 + MARKER_1] = CMD_MARKER_1_VAL;

    uint8_t checksum = 0;
    for (int i = 1; i < PACKET_SIZE - 1; i++) {
        checksum ^= p[i];
    }
    p[PACKET_SIZE - 1] = checksum;
}

int process_packet(unsigned char *data, int len) {
    if (len < 4) return 0;

    // STATUS
    if (data[0] == 0x05 && data[1] == 0xA6) {
        int state_byte = data[3];
        if (state_byte == 0x01 && current_state != STATE_CONNECTED) {
            printf(">>> [EVENT] KEYBOARD CONNECTED (Link Up)\n");
            current_state = STATE_CONNECTED;
            // US Layout (0x0409)
            set_layout(0x0409);
        } else if (state_byte == 0x02 && current_state != STATE_DISCONNECTED) {
            printf(">>> [EVENT] KEYBOARD DISCONNECTED (Link Down)\n");
            current_state = STATE_DISCONNECTED;
            // PT Layout (0x0816)
            set_layout(0x0816);
        }
        return 0;
    }

    // PONG
    if (data[0] == 0x20 && data[1] == 0x01) {
        return 1;
    }

    return 0;
}

int find_interfaces(char **path_cmd, char **path_stat) {
    struct hid_device_info *devs, *cur_dev;

    devs = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    cur_dev = devs;

    int found_cmd = 0;
    int found_stat = 0;

    while (cur_dev) {
        if (cur_dev->usage_page == PAGE_COMMAND) {
            if (*path_cmd) free(*path_cmd);
            *path_cmd = strdup(cur_dev->path);
            found_cmd = 1;
        }
        else if (cur_dev->usage_page == PAGE_STATUS) {
            if (*path_stat) free(*path_stat);
            *path_stat = strdup(cur_dev->path);
            found_stat = 1;
        }
        cur_dev = cur_dev->next;
    }

    hid_free_enumeration(devs);
    return (found_cmd && found_stat);
}

void check_initial_connection(hid_device *dev_cmd, hid_device *dev_stat) {
    printf(">> Checking initial connection state...\n");
    unsigned char buf[64];

    hid_write(dev_cmd, CMD_RIPPLE, sizeof(CMD_RIPPLE));
    Sleep(50);

    int pong_seen = 0;

    for (int i = 0; i < 10; i++) {
        hid_write(dev_cmd, CMD_PING, sizeof(CMD_PING));

        int res_cmd = hid_read_timeout(dev_cmd, buf, 64, 100);
        if (res_cmd > 0) {
            if (process_packet(buf, res_cmd)) {
                printf("  [PONG] on attempt #%d\n", i+3);
                pong_seen = 1;
                break;
            }
        }

        int res_stat = hid_read_timeout(dev_stat, buf, 64, 50);
        if (res_stat > 0) {
            process_packet(buf, res_stat);
        }
    }

    if (pong_seen && current_state == STATE_UNKNOWN) {
        printf("   [RESULT] Keyboard is CONNECTED\n");
        current_state = STATE_CONNECTED;
        set_layout(0x0409);
    } else if (!pong_seen && current_state == STATE_UNKNOWN) {
        printf("   [RESULT] Keyboard is DISCONNECTED (No Pong)\n");
        current_state = STATE_DISCONNECTED;
        set_layout(0x0816);
    }
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    printf("Starting for VID=%04X PID=%04X...\n", VENDOR_ID, PRODUCT_ID);

    if (hid_init()) return -1;

    hid_device *dev_cmd = NULL;
    hid_device *dev_stat = NULL;
    char *path_cmd = NULL;
    char *path_stat = NULL;

    unsigned char buf[64];

    while (1) {
        if (!dev_cmd || !dev_stat) {
            if (find_interfaces(&path_cmd, &path_stat)) {

                dev_cmd = hid_open_path(path_cmd);
                dev_stat = hid_open_path(path_stat);

                if (dev_cmd && dev_stat) {
                    printf(">> Interfaces Opened\n");
                    check_initial_connection(dev_cmd, dev_stat);
                    printf(">> Listening for events\n");
                } else {
                    printf(">> Error opening paths\n");
                    if (dev_cmd) { hid_close(dev_cmd); dev_cmd = NULL; }
                    if (dev_stat) { hid_close(dev_stat); dev_stat = NULL; }
                }
            } else {
                printf("Waiting for dongle...\n");
            }

            if (!dev_cmd || !dev_stat) {
                Sleep(2000);
                continue;
            }
        }

        int res = hid_read_timeout(dev_stat, buf, 64, 1000);

        if (res > 0) {
            process_packet(buf, res);
        } else if (res == -1) {
            printf(">> Device removed\n");
            hid_close(dev_cmd);
            hid_close(dev_stat);
            dev_cmd = NULL;
            dev_stat = NULL;
            current_state = STATE_UNKNOWN;
        }
        hid_read_timeout(dev_cmd, buf, 64, 1);
    }
    hid_exit();
    return 0;
}