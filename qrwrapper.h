/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   qrwrapper.h
 * Author: jauhari-aino
 *
 * Created on January 31, 2019, 8:53 PM
 */

#ifndef QRWRAPPER_H
#define QRWRAPPER_H
#define VERSIONQR 1.3
#ifdef __cplusplus
extern "C" {
#endif
    int open_device(unsigned short VID, unsigned short PID);
    void print_hidon_system();
    int hid_manufacture(char *manufacture);
    int hid_product(char *product);
    int hid_serial_number(char *serialnumber);
    int hid_set_blockingread(int state);
    int read_qrcode(char *data, int *data_len, int timeout);
    int open_device_inputEvent(const char *path_event);
#ifdef __cplusplus
}
#endif

#endif /* QRWRAPPER_H */

