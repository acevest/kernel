/*
 * ------------------------------------------------------------------------
 *   File Name: serial.c
 *      Author: Zhao Yanbai
 *              2023-09-26 21:03:27 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "io.h"

#define COMM1_PORT 0x3F8
#define COMM2_PORT 0x2F8
#define COMM3_PORT 0x3E8
#define COMM4_PORT 0x2F8
#define COMM5_PORT 0x5F8
#define COMM6_PORT 0x4F8
#define COMM7_PORT 0x5E8
#define COMM8_PORT 0x4E8

#define SERIAL_PORT COMM1_PORT

void init_serial() {
    uint32_t port = SERIAL_PORT;
    outb(0x00, port + 1);  // 禁用中断
    outb(0x80, port + 3);
    outb(115200 / 9600, port + 0);
    outb(0x00, port + 1);
    outb(0x03, port + 3);
    outb(0xC7, port + 2);
    outb(0x0B, port + 4);
}

void serial_putc(char c) {
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0) {
    }
    outb(c, SERIAL_PORT);
}

void serial_write(const char *buf, size_t size) {
    // return 0;
    for (size_t i = 0; i < size; i++) {
        serial_putc(buf[i]);
    }
}
