#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <Winuser.h>
#define Sleep(x) Sleep(x)
#else
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#define Sleep(x) usleep((x)*1000)
#endif

typedef enum {
    USB_VENDOR_ID = 0x0C45,
    USB_PRODUCT_ID = 0xFDFD,

    INTERFACE_STATUS = 2,
    INTERFACE_CMD = 3,

    EP_STATUS_IN = 0x83,
    EP_CMD_OUT = 0x05,
    EP_CMD_IN = 0x85
} UsbConfig;

volatile sig_atomic_t stop_flag = 0;

void handle_sigint(int sig) {
    (void)sig;
    stop_flag = 1;
}

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

typedef enum {
    IDX_HEADER_1 = 0,
    IDX_HEADER_2 = 1,
    IDX_HEADER_3 = 2,
    IDX_STYLE = 3,
    IDX_RED = 4,
    IDX_GREEN = 5,
    IDX_BLUE = 6,
    IDX_COLORFUL = 11,
    IDX_BRIGHTNESS = 12,
    IDX_SPEED = 13,
    IDX_DIRECTION = 14,
    IDX_MARKER_0 = 17,
    IDX_MARKER_1 = 18,
    IDX_XOR_CHECKSUM = 31
} PacketIndex;

typedef enum {
    FLAG_RED = (1 << IDX_RED),
    FLAG_GREEN = (1 << IDX_GREEN),
    FLAG_BLUE = (1 << IDX_BLUE),
    FLAG_COLORFUL = (1 << IDX_COLORFUL),
    FLAG_BRIGHTNESS = (1 << IDX_BRIGHTNESS),
    FLAG_SPEED = (1 << IDX_SPEED),
    FLAG_DIRECTION = (1 << IDX_DIRECTION)
} PacketFlag;

typedef enum {
    DEFAULT_RED = 0xFF,
    DEFAULT_GREEN = 0xFF,
    DEFAULT_BLUE = 0xFF,
    DEFAULT_COLORFUL = 0x01,
    DEFAULT_BRIGHTNESS = 0x05,
    DEFAULT_SPEED = 0x03,
    DEFAULT_DIRECTION = 0x00
} PacketDefault;

typedef enum {
    MIN_RGB = 0x00,
    MAX_RGB = 0xFF,

    MIN_COLORFUL = 0x00,
    MAX_COLORFUL = 0x01,

    MIN_BRIGHTNESS = 0x01,
    MAX_BRIGHTNESS = 0x05,

    MIN_SPEED = 0x01,
    MAX_SPEED = 0x05,

    MIN_DIRECTION = 0x00,
    MAX_DIRECTION = 0x03
} PacketRange;

typedef enum {
    STATIC_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS),
    SINGLE_ON_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    SINGLE_OFF_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    GLITTERING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    FALLING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    COLOURFUL_AVAILABLE = (FLAG_BRIGHTNESS | FLAG_SPEED),
    BREATH_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    SPECTRUM_AVAILABLE = (FLAG_BRIGHTNESS | FLAG_SPEED),
    OUTWARD_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    SCROLLING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION),
    ROLLING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION),
    ROTATING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION),
    EXPLODE_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    LAUNCH_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    RIPPLES_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    FLOWING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION),
    PULSATING_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    TILT_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED | FLAG_DIRECTION),
    SHUTTLE_AVAILABLE = (FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_COLORFUL | FLAG_BRIGHTNESS | FLAG_SPEED),
    LED_OFF_AVAILABLE = 0
} EffectAvailableFlags;

typedef enum {
    PACKET_SIZE = 33,
    HID_REPORT_ID = 0x00,
    LIGHT_HEADER_BYTE_1 = 0x05,
    LIGHT_HEADER_BYTE_2 = 0x10,
    LIGHT_HEADER_BYTE_3 = 0x00,
    CMD_MARKER_0_VAL = 0xAA,
    CMD_MARKER_1_VAL = 0x55
} PacketConstants;

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

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
    p[1 + IDX_HEADER_1] = LIGHT_HEADER_BYTE_1;
    p[1 + IDX_HEADER_2] = LIGHT_HEADER_BYTE_2;
    p[1 + IDX_HEADER_3] = LIGHT_HEADER_BYTE_3;

    p[1 + IDX_STYLE] = k->effect;
    p[1 + IDX_RED] = (k->available & FLAG_RED) ? k->red : DEFAULT_RED;
    p[1 + IDX_GREEN] = (k->available & FLAG_GREEN) ? k->green : DEFAULT_GREEN;
    p[1 + IDX_BLUE] = (k->available & FLAG_BLUE) ? k->blue : DEFAULT_BLUE;
    p[1 + IDX_COLORFUL] = (k->available & FLAG_COLORFUL) ? k->colorful : DEFAULT_COLORFUL;
    p[1 + IDX_BRIGHTNESS] = (k->available & FLAG_BRIGHTNESS) ? k->brightness : DEFAULT_BRIGHTNESS;
    p[1 + IDX_SPEED] = (k->available & FLAG_SPEED) ? k->speed : DEFAULT_SPEED;
    p[1 + IDX_DIRECTION] = (k->available & FLAG_DIRECTION) ? k->direction : DEFAULT_DIRECTION;

    p[1 + IDX_MARKER_0] = CMD_MARKER_0_VAL;
    p[1 + IDX_MARKER_1] = CMD_MARKER_1_VAL;

    uint8_t checksum = 0;
    for (int i = 1; i < PACKET_SIZE - 1; i++) {
        checksum ^= p[i];
    }
    p[PACKET_SIZE - 1] = checksum;
}

void print_packet(int endpoint, unsigned char *data, int len) {
    printf("Packet [EP 0x%02X] [%d]: ", endpoint, len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int process_packet(int endpoint, unsigned char *data, int len) {
    if (len < 4) return 0;

    print_packet(endpoint, data, len);

    // STATUS (from EP 0x83)
    if (endpoint == EP_STATUS_IN && data[0] == 0x05 && data[1] == 0xA6) {
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

    // PONG (from EP 0x85)
    if (endpoint == EP_CMD_IN && data[0] == 0x20 && data[1] == 0x01) {
        return 1;
    }

    return 0;
}

// Libusb helper functions
libusb_device_handle *open_device() {
    libusb_device **devs;
    libusb_device_handle *handle = nullptr;
    ssize_t cnt = libusb_get_device_list(nullptr, &devs);
    int r;

    if (cnt < 0)
        return nullptr;

    for (ssize_t i = 0; i < cnt; i++) {
        struct libusb_device_descriptor desc;
        r = libusb_get_device_descriptor(devs[i], &desc);
        if (r < 0) continue;

        if (desc.idVendor == USB_VENDOR_ID && desc.idProduct == USB_PRODUCT_ID) {
            r = libusb_open(devs[i], &handle);
            if (r < 0) {
                fprintf(stderr, "Maybe check permissions\n");
                handle = nullptr;
            } else {
                break; // Found
            }
        }
    }

    libusb_free_device_list(devs, 1);

    if (!handle) return nullptr;

    for (int i = 0; i < 2; i++) {
        int interfaces[] = {INTERFACE_STATUS, INTERFACE_CMD};
        int iface = interfaces[i];
        if (libusb_kernel_driver_active(handle, iface) == 1) {
            r = libusb_detach_kernel_driver(handle, iface);
            if (r < 0) fprintf(stderr, "Failed to detach kernel driver Iface %d: %s\n", iface, libusb_error_name(r));
        }
        r = libusb_claim_interface(handle, iface);
        if (r < 0) {
            fprintf(stderr, "libusb_claim_interface Iface %d error: %s\n", iface, libusb_error_name(r));
        }
    }
    return handle;
}

void close_device(libusb_device_handle *handle) {
    if (handle) {
        libusb_release_interface(handle, INTERFACE_STATUS);
        libusb_release_interface(handle, INTERFACE_CMD);
        libusb_close(handle);
    }
}

int send_packet(libusb_device_handle *handle, unsigned char *data, int len) {
    int transferred;
    // Skip first byte if 0x00
    if (len > 0 && data[0] == 0x00) {
        data++;
        len--;
    }

    int r = libusb_interrupt_transfer(handle, EP_CMD_OUT, data, len, &transferred, 100);
    if (r < 0) {
        fprintf(stderr, "Write error to EP 0x%02X: %s\n", EP_CMD_OUT, libusb_error_name(r));
        return -1;
    }
    return transferred;
}

int read_packet(libusb_device_handle *handle, int endpoint, unsigned char *data, int len, int timeout) {
    int transferred;
    int r = libusb_interrupt_transfer(handle, endpoint, data, len, &transferred, timeout);
    if (r == 0)
        return transferred;
    if (r == LIBUSB_ERROR_TIMEOUT)
        return 0;
    return -1;
}

void check_initial_connection(libusb_device_handle *handle) {
    printf(">> Checking initial connection...\n");
    unsigned char buf[64];

    KeyboardLights k;
    set_layout(0x0816);
    set_effect(&k, COLOURFUL);
    set_rgb(&k, 0xFF, 0xFF, 0xFF);
    set_brightness(&k, 0x05);
    set_speed(&k, 0x03);
    keyboard_light_to_packet(&k, buf);
    send_packet(handle, buf, PACKET_SIZE);
    Sleep(50);

    int pong_seen = 0;

    for (int i = 0; i < 10; i++) {
        send_packet(handle, CMD_PING, sizeof(CMD_PING));
        int res = read_packet(handle, EP_CMD_IN, buf, 32, 100);
        if (res > 0) {
            if (process_packet(EP_CMD_IN, buf, res)) {
                printf("[PONG] on attempt #%d\n", i+3);
                pong_seen = 1;
                break;
            }
        }
        while (read_packet(handle, EP_STATUS_IN, buf, 64, 10) > 0) {
            //flush pending packets;
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

    signal(SIGINT, handle_sigint);

    printf("Starting for VID=%04X PID=%04X (libusb)...\n", USB_VENDOR_ID, USB_PRODUCT_ID);

    if (libusb_init(nullptr) < 0) return -1;

    libusb_device_handle *handle = nullptr;
    unsigned char buf[64];

    while (!stop_flag) {
        if (!handle) {
            handle = open_device();
            if (handle) {
                printf(">> Device Opened\n");
                check_initial_connection(handle);
                printf(">> Listening for events\n");
            } else {
                printf("Waiting for dongle...\n");
                Sleep(2000);
                continue;
            }
        }

        int res = read_packet(handle, EP_STATUS_IN, buf, 64, 100);
        if (res > 0) {
            process_packet(EP_STATUS_IN, buf, res);
        } else if (res < 0 && res != LIBUSB_ERROR_TIMEOUT) {
            printf(">> Device removed or error (EP 0x%02X)\n", EP_STATUS_IN);
            close_device(handle);
            handle = nullptr;
            current_state = STATE_UNKNOWN;
            continue;
        }

        // Poll Command Endpoint (32 bytes)
        res = read_packet(handle, EP_CMD_IN, buf, 32, 10);
        if (res > 0) {
            process_packet(EP_CMD_IN, buf, res);
        }
    }

    if (handle) close_device(handle);
    libusb_exit(nullptr);
    printf("\nExiting cleanly.\n");
    return 0;
}