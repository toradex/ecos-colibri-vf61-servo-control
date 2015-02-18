/*
 * servo.h
 *
 * (c) 2015 Toradex AG
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

#ifndef SERVO_H_
#define SERVO_H_

#define MCC_NODE_A5        (0)
#define MCC_NODE_M4        (0)
#define MCC_SENDER_PORT    (1)
#define MCC_RESPONDER_PORT (2)

#define SERVO_STATE_INVALID	(0)
#define SERVO_STATE_CW		(1)
#define SERVO_STATE_CCW		(2)
#define SERVO_STATE_END		(3)

void thread_mcc_fn(void);

#endif /* SERVO_H_ */
