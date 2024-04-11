/*
 * ------------------------------------------------------------------------
 *   File Name: time.c
 *      Author: Zhao Yanbai
 *              2024-04-11 19:27:39 Thursday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "printk.h"
#include "types.h"

#define MINUTE 60
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
#define YEAR (365 * DAY)

uint32_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int is_leap_year(uint32_t year) { return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); }

void timestamp_to_date(uint32_t timestamp) {
    // 将时间戳转换为东8区时间
    timestamp += 8 * HOUR;

    uint32_t seconds = timestamp % 60;
    uint32_t minutes = (timestamp / MINUTE) % 60;
    uint32_t hours = (timestamp / HOUR) % 24;
    uint32_t days_since_1970 = timestamp / DAY;

    uint32_t year = 1970;
    while (days_since_1970 >= (is_leap_year(year) ? 366 : 365)) {
        days_since_1970 -= (is_leap_year(year) ? 366 : 365);
        year++;
    }

    if (is_leap_year(year)) {
        days_in_month[2] = 29;
    } else {
        days_in_month[2] = 28;
    }

    uint32_t month = 1;
    while (days_since_1970 >= days_in_month[month]) {
        days_since_1970 -= days_in_month[month];
        month++;
    }

    uint32_t day = days_since_1970 + 1;

    printk("%u-%02u-%02u %02u:%02u:%02u\n", year, month, day, hours, minutes, seconds);
}
