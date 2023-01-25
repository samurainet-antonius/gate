#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "hidutil.h"
#include "qrwrapper.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <bits/fcntl-linux.h>
#define MAX_STR 255
#define BUF_SIZE 1024
#define TIMEOUT_READ 500000
static int fevdev = 0;
static bool device_status = false;
static char hidtable1[] = {
    ' ', ' ', ' ', ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', '0', ' ', ' ', ' ', '\t', ' ', '-', '=', '[',
    ']', '\\', '`', ';', '\'', '`', ',', '.', '/'
};

static char hidtable2[] = {
    ' ', ' ', ' ', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
    '#', '$', '%', '^', '&', '*', '(', ')', ' ', ' ', ' ', '\t', ' ', '_', '+', '{',
    '}', '|', '~', ':', '"', '~', '<', '>', '?'
};

static char eventTable[] = {
    ' ', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '-', '=', '\b', 't', 'q', 'w', 'e', 'r', 't', 'y',
    'u', 'i', 'o', 'p', '[', ']', '\n', ' ', 'a', 's', 'd',
    'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', ' ', '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', ' ',
    ' ', ' ', ' '
};
static char eventTable2[] = {
    ' ', ' ', '!', '@', '#', '$', '%', '^', '&', '*', '-',
    ')', '_', '+', '\b', 't', 'Q', 'W', 'E', 'R', 'T', 'Y',
    'U', 'I', 'O', 'P', '{', '}', '\n', ' ', 'A', 'S', 'D',
    'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', ' ', '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', ' ',
    ' ', ' ', ' '
};


static hid_device *handle;
struct termios orig_termios;
static char path_device[64] = {0};

void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode() {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof (new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof (c))) < 0) {
        return r;
    } else {
        return c;
    }
}

int open_device(unsigned short VID, unsigned short PID) {
    handle = hid_open(VID, PID, NULL);
    hid_set_nonblocking(handle, 1);
    if (handle == NULL) {
        return -1;
    } else return 0;
}

int open_device_inputEvent(const char *path_event) {
    strcpy(path_device, path_event);
    char name[256] = "Unknown";
    //    fevdev = open(path_event, O_RDONLY | O_NDELAY | O_NONBLOCK);
    fevdev = open(path_event, O_RDONLY);
    if (fevdev == -1) {
        printf("Failed to open event device.\n");
        close(fevdev);
        return -1;
    }
    int result = 0;
    result = ioctl(fevdev, EVIOCGNAME(sizeof (name)), name);
    printf("Reading From : %s (%s)\n", path_event, name);
    printf("Getting exclusive access: ");
    result = ioctl(fevdev, EVIOCGRAB, 1);
    printf("%s\n", (result == 0) ? "SUCCESS" : "FAILURE");
    if (result == 0) device_status = true;
    return result;
}

void print_hidon_system() {
    struct hid_device_info *devs, *cur_dev;
    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;
    while (cur_dev) {
        printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
                cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
        printf("\n");
        printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
        printf("  Product:      %ls\n", cur_dev->product_string);
        printf("\n");
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
}

int hid_manufacture(char *manufacture) {
    if (handle == NULL) return -1;
    wchar_t wstr[MAX_STR];
    int res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    memcpy(manufacture, wstr, sizeof (wstr));
    return res;
}

int hid_product(char *product) {
    if (handle == NULL) return -1;
    wchar_t wstr[MAX_STR];
    int res = hid_get_product_string(handle, wstr, MAX_STR);
    memcpy(product, wstr, sizeof (wstr));
    return res;
}

int hid_serial_number(char *serialnumber) {
    if (handle == NULL) return -1;
    wchar_t wstr[MAX_STR];
    int res = hid_get_serial_number_string(handle, wstr, MAX_STR);
    memcpy(serialnumber, wstr, sizeof (wstr));
    return res;
}

int hid_set_blockingread(int state) {
    if (handle == NULL) return -1;
    int res = hid_set_nonblocking(handle, 1);
    return res;
}

//int read_qrcode(char *data, int *data_len, int timeout) {
//    if (device_status) {
//        struct input_event ev[64];
//        int size = sizeof (struct input_event);
//        int rd;
//        int value;
//        char buf[256];
//        char output[65536];
//        int ready_for_reading = 0;
//        int readIndex = 0;
//        fd_set input_set;
//        struct timeval time;
//        int time_out = 0;
//        bool done = false;
//        bool shift = false;
//        while (1) {
//            FD_SET(fevdev, &input_set);
//            time.tv_sec = 0;
//            time.tv_usec = timeout;
//            ready_for_reading = select(fevdev + 1, &input_set, NULL, NULL, &time);
//            if (ready_for_reading == -1) {
//                printf("not ready\n");
//                return -1;
//            }
//            if (ready_for_reading) {
//                if ((rd = read(fevdev, ev, size * 64)) < size) {
//                    printf("errno: %s\n", strerror(errno));
//                    if (errno == ENODEV || errno == EIO || errno == ENXIO) {
//                        device_status = false;
//                        close(fevdev);
//                    }
//                    break;
//                }
//                printf("rd: %d\n", rd);
//                printf("ev[1].type:%d ev[1].value: %d ev.value: %c ev[1].code: %d -> %c\n",ev[1].type, ev[1].value, value, ev[1].code, output[readIndex - 1]);
//                value = ev[0].value;
//                if (value != ' ' && ev[1].value == 1 && ev[1].type == 1) {
//                    if (ev[1].code == 28) {
//                        done = true;
//                        break;
//                    }
//                    if (shift) {
//                        if (ev[1].code == 42) shift = true;
//                        else {
//                            output[readIndex] = eventTable2[ev[1].code];
//                            shift = false;
//                            readIndex++;
//                        }
//                    } else {
//                        if (ev[1].code == 42) shift = true;
//                        else {
//                            output[readIndex] = eventTable[ev[1].code];
//                            shift = false;
//                            readIndex++;
//                        }
//                    }
//                }
//            } else {
//                timeout = 1;
//                break;
//            }
//        }
//        if (done == false && timeout == 1) return -1;
//        else if (done == true || timeout == 0) {
//            memcpy(data, output, readIndex);
//            *data_len = readIndex;
//            //            unsigned char a = 0;
//            //            if (kbhit()) {
//            //                a = getch();
//            //                while (kbhit())
//            //                    getch();
//            //            }
//            return 0;
//        } else {
//            //            unsigned char a = 0;
//            //            if (kbhit()) {
//            //                a = getch();
//            //                while (kbhit())
//            //                    getch();
//            //            }
//            return -1;
//        }
//        return 0;
//    } else {
//        open_device_inputEvent(path_device);
//        return -1;
//    }
//    //    printf("kata: %s\n", output);
//}

int read_qrcode(char *data, int *data_len, int timeout) {
    if (device_status) {
        char output[1024];
        struct input_event ev;
        int size = sizeof (ev);
        int rd;
        int value;
        int ready_for_reading = 0;
        int readIndex = 0;
        fd_set input_set;
        int timeouts = 0;
        struct timeval time;
        bool done = false;
        bool shift = false;
        while (1) {
            FD_SET(fevdev, &input_set);
            time.tv_sec = 0;
            time.tv_usec = timeout;
            ready_for_reading = select(fevdev + 1, &input_set, NULL, NULL, &time);
            if (ready_for_reading == -1) {
                printf("not ready\n");
                printf("errno: %s\n", strerror(errno));
                device_status = false;
                close(fevdev);
                return -1;
            }
            if (ready_for_reading) {
                if ((rd = read(fevdev, &ev, sizeof (ev))) < size) {
                    printf("errno: %s\n", strerror(errno));
                    if (errno == ENODEV || errno == EIO || errno == ENXIO) {
                        printf("close fevdev\n");
                        device_status = false;
                        close(fevdev);
                    }
                    break;
                }
                value = ev.value;
                if (value != ' ' && ev.value == 1 && ev.type == 1) {
                    if (ev.code == 96 || ev.code == 108) return -1;
                    if (ev.code == 28) {
                        done = true;
                        break;
                    }
                    if (shift) {
                        if (ev.code == 42) shift = true;
                        else {
                            output[readIndex] = eventTable2[ev.code];
                            shift = false;
                            readIndex++;
                        }
                    } else {
                        if (ev.code == 42) shift = true;
                        else {
                            output[readIndex] = eventTable[ev.code];
                            shift = false;
                            readIndex++;
                        }
                    }
                }
            } else {
                timeouts = 1;
                break;
            }
        }
        if (done == false && timeouts == 1) {
            return -1;
        } else if (done == true || timeouts == 0) {
            memcpy(data, output, readIndex);
            *data_len = readIndex;
            //            usleep(100000);
            //            unsigned char a = 0;
            //            if (kbhit()) {
            //                a = getch();
            //                while (kbhit())
            //                    getch();
            //            }
            return 0;
        }
        return 0;
    } else {
        open_device_inputEvent(path_device);
        return -1;
    }
    //    printf("kata: %s\n", output);
}

//
//int read_qrcode(char *data, int *data_len, int timeout) {
//    fd_set input_set;
//    struct timeval time;
//
//    bool done = false;
//    bool shift = false;
//    int fd_event = open("/dev/input/event15", O_RDWR | O_NOCTTY | O_NDELAY);
//    char buf[256];
//    char output[65536];
//    int ready_for_reading = 0;
//    int readIndex = 0;
//    int time_out = 0;
//    unsigned char a = 0;
//    if (kbhit()) {
//        a = getch();
//        while (kbhit())
//            getch();
//    }
//    while (!time_out) {
//        FD_SET(hid_fd, &input_set);
//        time.tv_sec = 0;
//        time.tv_usec = timeout;
//        ready_for_reading = select(hid_fd + 1, &input_set, NULL, NULL, &time);
//        if (ready_for_reading == -1) {
//            return -1;
//        }
//        if (ready_for_reading) {
//            int rb = hid_read(handle, buf, 8);
//            //            int rb = read(fd_event, buf, 8);
//            if (rb > 0) {
//                for (int x = 0; x < 8; x++) {
//                    if ((int) buf[x] == 0) continue;
//                    if ((int) buf[x] == 40) {
//                        done = true;
//                        break;
//                    }
//                    if (shift) {
//                        if ((int) buf[x] == 2) shift = true;
//                        else {
//                            output[readIndex] = hidtable2[(int) buf[x]];
//                            shift = false;
//                            readIndex++;
//                        }
//                    } else {
//                        if ((int) buf[x] == 2) shift = true;
//                        else {
//                            output[readIndex] = hidtable1[(int) buf[x]];
//                            readIndex++;
//                        }
//                    }
//                }
//            }
//        } else {
//            timeout = 1;
//            break;
//        }
//    }
//    if (done == false && timeout == 1) return -1;
//    else if (done == true || timeout == 0) {
//        memcpy(data, output, readIndex);
//        *data_len = readIndex;
//        unsigned char a = 0;
//        if (kbhit()) {
//            a = getch();
//            while (kbhit())
//                getch();
//        }
//        return 0;
//    } else {
//        unsigned char a = 0;
//        if (kbhit()) {
//            a = getch();
//            while (kbhit())
//                getch();
//        }
//        return -1;
//    }
//}