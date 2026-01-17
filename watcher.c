#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <Winuser.h>
#include "hidapi.h"

#define VENDOR_ID  0x0C45
#define PRODUCT_ID 0xFDFD

#define PAGE_COMMAND 0xFF60
#define PAGE_STATUS  0xFFFF

typedef enum {
    STATE_UNKNOWN,
    STATE_CONNECTED,
    STATE_DISCONNECTED
} LinkState;

LinkState current_state = STATE_UNKNOWN;

unsigned char CMD_INIT[33] = {
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
};

unsigned char CMD_PING[33] = {
    0x00, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21
};

void set_layout(int layout_id) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return;

    char hkl_string[9];
    sprintf(hkl_string, "0000%04x", layout_id);

    HKL new_hkl = LoadKeyboardLayoutA(hkl_string, KLF_ACTIVATE | KLF_SUBSTITUTE_OK);

    if (new_hkl)
        PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)new_hkl);
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

    hid_write(dev_cmd, CMD_INIT, sizeof(CMD_INIT));
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