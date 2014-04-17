/*
 * plotter.h
 *
 *  Created on: 2013-12-14
 *      Author: Antmicro Ltd
 */

#include <stdio.h>
#include <unistd.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/var_io.h>
#include <cyg/hal/var_io_gpio.h>
#include <cyg/io/io.h>
#include <cyg/io/serialio.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_if.h>
#include <cyg/kernel/kapi.h>

#ifndef TEST_APP_MIN_H_
#define TEST_APP_MIN_H_

#define MCC_MQX_NODE_A5        (0)
#define MCC_MQX_NODE_M4        (0)
#define MCC_MQX_SENDER_PORT    (1)
#define MCC_MQX_RESPONDER_PORT (2)

#define PLOTTER_STOP     (0)
#define PLOTTER_START    (1)
#define PLOTTER_DRAW_MEM (2)
#define PLOTTER_UNPAUSE	 (9)
#define PLOTTER_PAUSE    (10)
#define PLOTTER_HOME     (11)
#define PLOTTER_WELCOME	 (12)

void thread_fn_1(void);
void thread_fn_2(void);
void thread_fn_3(void);

void plotter_goto(int X, int Y);
void plotter_arc_cw(double x_end, double y_end, double x_center, double y_center);
void plotter_arc_ccw(double x_end, double y_end, double x_center, double y_center);

void plotter_go_home(void);
void setPen(int down);

int GCD(int a, int b);
void stepX(int i);
void stepY(int i);

#define FTM0_SC 0x40038000
#define FTM0_MOD 0x40038008
#define FTM0_C1SC 0x40038014
#define FTM0_C1V  0x40038018

#define FTM1_SC 0x40039000 
#define FTM1_MOD 0x40039008 
#define FTM1_C1SC 0x40039014 
#define FTM1_C1V  0x40039018 

#endif /* TEST_APP_MIN_H_ */
