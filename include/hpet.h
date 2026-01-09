/*
 * ------------------------------------------------------------------------
 *   File Name: hpet.h
 *      Author: Zhao Yanbai
 *              2026-01-08 19:28:25 Thursday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

void hpet_init();

void hpet_init_timer0();

// 使能HPET定时器
void hpet_enable();

//
void hpet_disable();

// 调用此函数前后需要手动调用 hpet_enable 和 hpet_disable
void hpet_prepare_calibration();

bool hpet_calibration_end();
