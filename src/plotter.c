/*
 * plotter.c
 *
 *  Created on: 2013-12-14
 *      Author: Antmicro Ltd
 */
#include <stdlib.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_if.h>
#include <math.h>
#include "plotter.h"

#define USING_MCC

#ifdef USING_MCC
#include <cyg/mcc/mcc_api.h>
#endif

#define CYGHWR_HAL_VYBRID_PORT_ALT_GPIO 0
#define CYGHWR_HAL_VYBRID_PORT_INPUT 0x21
#define CYGHWR_HAL_VYBRID_PORT_OUTPUT 0x22

//Outputs
#define PUL_X CYGHWR_HAL_VYBRID_PIN(B, 19, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_OUTPUT)
#define DIR_X CYGHWR_HAL_VYBRID_PIN(C, 4, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_OUTPUT)
#define PUL_Y CYGHWR_HAL_VYBRID_PIN(C, 5, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_OUTPUT)
#define DIR_Y CYGHWR_HAL_VYBRID_PIN(C, 2, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_OUTPUT)

//Inputs
#define EM CYGHWR_HAL_VYBRID_PIN(D, 40, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_OUTPUT)
#define LIM_X CYGHWR_HAL_VYBRID_PIN(D, 9, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_INPUT)
#define LIM_Y CYGHWR_HAL_VYBRID_PIN(C, 1, CYGHWR_HAL_VYBRID_PORT_ALT_GPIO, CYGHWR_HAL_VYBRID_PORT_INPUT)

#ifdef USING_MCC
typedef struct the_message
{
   cyg_uint32  DATA;
   cyg_uint32 STATUS;
} THE_MESSAGE;

unsigned int endpoint_a5[] = {0,MCC_NODE_A5,MCC_SENDER_PORT};
unsigned int endpoint_m4[] = {1,MCC_NODE_M4,MCC_RESPONDER_PORT};
#endif

//Timers
#define FTM1 CYGHWR_HAL_VYBRID_FTM1_P

//Additional definitions
#define FACTOR 8.823529412*2

//Threads
static unsigned char stack_thread1[CYGNUM_HAL_STACK_SIZE_MINIMUM];
static unsigned char stack_thread2[CYGNUM_HAL_STACK_SIZE_MINIMUM];
static unsigned char stack_thread3[CYGNUM_HAL_STACK_SIZE_MINIMUM];

cyg_handle_t thread_1;
cyg_handle_t thread_2;
cyg_handle_t thread_3;

cyg_thread thread_data_1;
cyg_thread thread_data_2;
cyg_thread thread_data_3;

//Global variables
volatile int _x=0, _zadX=0;
volatile int _y=0, _zadY=0;
volatile double _delayX=1, _delayY=1;
volatile double _errorX=0, _errorY=0;
int im_size;

//Control variables
int progress;
int plotter_status = PLOTTER_STOP;

//PWM variables
volatile int PWM = 1;
volatile int PWM_PERIOD = 5;

//Memory registers
volatile CYG_WORD32 *EcosLoaded = (CYG_WORD32*) 0x8f9ffff4;
volatile CYG_WORD32 *InstructionFromLinux = (CYG_WORD32*) 0x8f9ffff8;
volatile CYG_WORD32 *StatusToLinux = (CYG_WORD32*) 0x8f9ffffc;
volatile float *SharedMemory = (float*)0x8fa00000;
volatile float *image_to_draw;

//----------------------------------------------------------------------
//Main Thread
int main(int argc, char **argv)
{
	cyg_thread_create(10, (cyg_thread_entry_t*) thread_fn_1, 0, "thread1", &stack_thread1[0], CYGNUM_HAL_STACK_SIZE_MINIMUM, &thread_1, &thread_data_1);
	cyg_thread_create(10, (cyg_thread_entry_t*) thread_fn_2, 0, "thread2", &stack_thread2[0], CYGNUM_HAL_STACK_SIZE_MINIMUM, &thread_2, &thread_data_2);
	cyg_thread_create(10, (cyg_thread_entry_t*) thread_fn_3, 0, "thread3", &stack_thread3[0], CYGNUM_HAL_STACK_SIZE_MINIMUM, &thread_3, &thread_data_3);

	diag_printf("--------------------------\n");
	diag_printf(" eCos Plotter Application \n");
	diag_printf("--------------------------\n");

	//Initialize GPIO
	 hal_set_pin_function(PUL_X);
	 hal_set_pin_function(DIR_X);
	 hal_set_pin_function(PUL_Y);
	 hal_set_pin_function(DIR_Y);
	 hal_set_pin_function(EM);
	 hal_set_pin_function(LIM_X);
	 hal_set_pin_function(LIM_Y);

	//Initialize FlexTimer for PWM generation
	FTM1->sc = 0xf; 		//SC: last 5 bits: 00(CLK source)001(Prescaler)
	FTM1->mod = 0x00fe;		//MOD register -> MOD+1 determine the period of PWM signal
	FTM1->c[1].sc = 0x28; 	//C1SC Status and Control, MSB set-> EPWM enabled, ELSB->clear out on match

	cyg_thread_resume(thread_1); //Xaxis
	cyg_thread_resume(thread_2); //Yaxis
	cyg_thread_resume(thread_3); //Communication

	HAL_WRITE_UINT32(EcosLoaded, 0xDEADBEEF); //eCos started

	int i;
	double previusPenState = 0.0;
	double trans_x, trans_y, trans_i, trans_j;

	//First image register
	image_to_draw = (volatile float *)(SharedMemory+1);

	//main loop - reading and executing instructions from image_to_draw to (from image_to_draw + im_size)
	while (1)
	{
		switch(plotter_status)
		{
			case PLOTTER_DRAW_MEM:
				HAL_READ_UINT32(SharedMemory, im_size);
				plotter_status = PLOTTER_START;
			break;
			case PLOTTER_START: case PLOTTER_PAUSE:
			    if(im_size <= 0) {plotter_status = PLOTTER_STOP; break;}
			    else
			    {
					for(i=0; i<im_size;)
					{
						if(plotter_status == PLOTTER_START)
						{
							////G00, G01 - linear move
							if( (*(volatile CYG_WORD32 *)(image_to_draw+i)) == 0 || (*(volatile CYG_WORD32 *)(image_to_draw+i)) == 1)
							{
								if((*(volatile CYG_WORD32 *)(image_to_draw+i+3)) != 0xffffffff && (image_to_draw[i+3]) != previusPenState)
								{
									if((image_to_draw[i+3])>=0.0) {setPen(0); cyg_thread_delay(150);} //PenUp
									else {setPen(1); cyg_thread_delay(150);} //PenDown
								}

								previusPenState = (image_to_draw[i+3]);

								if((*(volatile CYG_WORD32 *)(image_to_draw+i+1)) != 0xffffffff)
								{
									trans_x = (image_to_draw[i+1])*FACTOR;
								}
								else trans_x = _x;

								if((*(volatile CYG_WORD32 *)(image_to_draw+i+2)) != 0xffffffff)
								{
									trans_y = (image_to_draw[i+2])*FACTOR;
								}
								else trans_y = _y;
								//diag_printf(">>PLOT LINE %d %d\n", (int)trans_x, (int)trans_y);
								cyg_thread_delay(30);
								plotter_goto((int)trans_x, (int)trans_y);
								cyg_thread_delay(30);
							}

							//G02 - CW Arc
							else if((*(volatile CYG_WORD32 *)(image_to_draw+i)) == 2 )
							{

								trans_x = (image_to_draw[i+1])*FACTOR;
								trans_y = (image_to_draw[i+2])*FACTOR;
								trans_i = (image_to_draw[i+4])*FACTOR;
								trans_j = (image_to_draw[i+5])*FACTOR;
								//diag_printf(">>PLOT CW %d %d    %d %d\n", (int)trans_x, (int)trans_y,(int)trans_i, (int)trans_j );
								plotter_arc_cw(trans_x, trans_y, trans_i, trans_j);
							}

							//G03 - CCW Arc
							else if((*(volatile CYG_WORD32 *)(image_to_draw+i)) == 3 )
							{
								trans_x = (image_to_draw[i+1])*FACTOR;
								trans_y = (image_to_draw[i+2])*FACTOR;
								trans_i = (image_to_draw[i+4])*FACTOR;
								trans_j = (image_to_draw[i+5])*FACTOR;
								//diag_printf(">>PLOT CCW %d %d    %d %d\n", (int)trans_x, (int)trans_y,(int)trans_i, (int)trans_j );
								plotter_arc_ccw(trans_x, trans_y, trans_i, trans_j);
							}

							//G04 - Wait [ms]
							else if((*(volatile CYG_WORD32 *)(image_to_draw+i)) == 4 )
							{
								cyg_thread_delay((*(volatile CYG_WORD32 *)(image_to_draw+i+6)));
							}
						}

						if((i+7)>= im_size-7 && progress != 100) {progress = 100;}
						else if(plotter_status == PLOTTER_START) {i+=7;if(progress != 100)progress=(int)ceil((i*100)/im_size);}
						else if(plotter_status == PLOTTER_PAUSE) {cyg_thread_delay(10);}
						else if(plotter_status == PLOTTER_UNPAUSE) {plotter_status = PLOTTER_START;i+=7;}
						else if(plotter_status == PLOTTER_HOME || plotter_status == PLOTTER_STOP) {break;}
					}
					if(i >= im_size) plotter_status = PLOTTER_STOP;
			    }
				break;
			case PLOTTER_HOME:
				progress = 0;
				setPen(0); cyg_thread_delay(100);
				plotter_go_home();
				plotter_status = PLOTTER_STOP;
				break;
			case PLOTTER_STOP:
				progress = 0;
				i=0;
				cyg_thread_delay(10);
				break;
			default:
				plotter_status=PLOTTER_STOP;
				cyg_thread_delay(10);
				break;
		}
	}
}

//----------------------------------------------Threads

/**
 * X Axis Thread
 *  -->
 */
void thread_fn_1(void)
{
	int slope_x=0;

	while(1)
	{
		//Stop motor when PLOTTER_STOP or PLOTTER_PAUSE
		if(plotter_status != PLOTTER_STOP && plotter_status != PLOTTER_PAUSE)
		{

			if(_x < _zadX)
			{
				hal_gpio_clear_pin(DIR_X);
				slope_x ^=1;
				stepX(slope_x);
				if(slope_x)
				{
					_x++;
				}

			}
			if(_x > _zadX)
			{
				hal_gpio_set_pin(DIR_X);
				slope_x ^=1;
				stepX(slope_x);
				if(slope_x )
				{
					_x--;
				}
			}

			if(_x == _zadX) {cyg_thread_delay(1); _errorX = 0;}
			else
			{
				cyg_thread_delay((int)(_delayX + _errorX));
				if(slope_x)
				{
						_errorX += (_delayX - (int)_delayX);
						_errorX -= (int)_errorX;
				}
			}

		}
		else cyg_thread_delay(1);
	}
}

/**
 * Y Axis Thread
 *  |
 *  V
 */
void thread_fn_2(void)
{
	int slope_y=0;

	while(1)
	{
		//Stop motor when PLOTTER_STOP or PLOTTER_PAUSE
		if(plotter_status != PLOTTER_STOP && plotter_status != PLOTTER_PAUSE)
		{
			if(_y < _zadY)
			{
				hal_gpio_clear_pin(DIR_Y);
				slope_y ^=1;
				stepY(slope_y);
				if(slope_y)
				{
					_y++;
				}
			}
			if(_y > _zadY)
			{
				hal_gpio_set_pin(DIR_Y);
				slope_y ^=1;
				stepY(slope_y);
				if(slope_y )
				{
					_y--;
				}
			}

			if(_y == _zadY) {cyg_thread_delay(1); _errorY = 0;}
			else
			{
				cyg_thread_delay((int)(_delayY + _errorY));
				if(slope_y)
				{
						_errorY += (_delayY - (int)_delayY);
						_errorY -= (int)_errorY;
				}
			}
		}
		else cyg_thread_delay(1);
	}
}

#ifdef USING_MCC

/**
 * MCC Thread
 */
void thread_fn_3(void)
{

	THE_MESSAGE     msg, smsg;
	unsigned int    num_of_received_bytes;
	int             ret_value;

	msg.DATA = 1;
	ret_value = mcc_initialize(MCC_NODE_M4);

	if(0 != ret_value) {diag_printf("Error! App stopped\n"); while(1);}

	ret_value = mcc_create_endpoint(endpoint_m4, MCC_RESPONDER_PORT);

	while(1)
	{
		//diag_printf("MCC_START \n");
		ret_value = mcc_recv_copy(endpoint_m4, &msg, sizeof(THE_MESSAGE), &num_of_received_bytes, 500000);
		if(0 != ret_value) {
			//diag_printf("Responder task receive error: 0x%02x\n", ret_value);
		} else {
			if(msg.DATA <= PLOTTER_WELCOME) plotter_status = msg.DATA;
			//diag_printf("Responder OK... %d %d\n", msg.DATA, msg.STATUS);
			//cyg_thread_delay(1);
			smsg.DATA=progress;
			smsg.STATUS = plotter_status;
			ret_value = mcc_send(endpoint_a5, &smsg, sizeof(THE_MESSAGE), 5000000);
		}
		//diag_printf("MCC_END \n");
		//diag_printf("Plotter status: %d Progress: %d\n", plotter_status, progress);
		cyg_thread_delay(50);
	}

}
//----------------------------------------------Functions
#else

/**
 * Communication thread
 */
void thread_fn_3(void)
{
	cyg_int32 MyStatus;
	cyg_int32 UsrInstruction=0, PrevUsrInstruction=0;

	while(1)
	{
		HAL_READ_UINT32(InstructionFromLinux, UsrInstruction);

		if(UsrInstruction != PrevUsrInstruction && UsrInstruction <= 12) plotter_status = UsrInstruction;

		MyStatus = (cyg_uint32)((((cyg_uint32)(plotter_status << 16))) | ((cyg_uint16)progress));
		HAL_WRITE_UINT32(StatusToLinux, MyStatus);

		PrevUsrInstruction = UsrInstruction;

		cyg_thread_delay(10);
	}

}
#endif

/**
 * Set PWM for electromagnet  1: 0%; 0: 50%
 */
void setPen(int down)
{
int duty;
if (down)
        duty = 0 & 0x00ff;
else
        duty = 140 & 0x00ff;
FTM1->c[1].v = duty;
}

/**
 * Move to point X, Y
 */
void plotter_goto(int X, int Y)
{
	int diffX = X - _x;
	int diffY = Y - _y;

	if(diffX != 0 && diffY != 0)
	{
		if(abs(diffX)>=abs(diffY))
		{
			_delayX = 1;
			_delayY = 1*((double)abs(diffX)/(double)abs(diffY));
		}
		else
		{
			_delayY = 1;
			_delayX = 1*((double)abs(diffY)/(double)abs(diffX));
		}
	}
	else if(diffX == 0 || diffY == 0)
	{
		_delayY = _delayX = 1.0;
	}

	if(_delayX<1) _delayX = 1.0;
	if(_delayY<1) _delayY = 1.0;


	_zadX = X;
	_zadY = Y;

	do{
		cyg_thread_delay(1);
	}while(((_x != X) || (_y != Y)) && plotter_status != PLOTTER_STOP );
}

/**
 * Clockwise arc
 */
void plotter_arc_cw(double x_end, double y_end, double ii, double jj)
{
    //Variable initialization
	int i=0;
    double x_center= ii+_x;
    double y_center= jj+_y;
    double cx=x_end; double cy=y_end;
    double radius = sqrt(pow((x_center - x_end),2) + pow((y_center - y_end),2));
    if(radius<=0) radius = 1; //fail safe
    double start_angle = 0, end_angle = 0;

    //Computing start angle
    start_angle = atan2((_x - x_center),(y_center - _y))*(180/M_PI);
    if(start_angle<0) start_angle += 360;

    //Computing end angle
    end_angle = atan2((x_end-x_center),(y_center-y_end))*(180/M_PI);
    if(end_angle<0) end_angle += 360;

    //Drawing CW Arc
    if(abs(end_angle - start_angle) > 2)
    {
		if(end_angle < start_angle) end_angle += 360.0;
		for(i=(int)start_angle; i<(int)end_angle; i++)
		{
				cx = (x_center+(radius*sin((i)*(M_PI/180.0))));
				cy = (y_center-(radius*cos((i)*(M_PI/180.0))));
				plotter_goto((int)(cx), (int)(cy));
		}
    }
	plotter_goto((int)(x_end), (int)(y_end));
}

/**
 * Counter clockwise arc
 */
void plotter_arc_ccw(double x_end, double y_end, double ii, double jj)
{
	//Variable initialization
	int i=0;
	double x_center= ii + _x;
	double y_center= jj + _y;
	double cx=x_end; double cy=y_end;
	double radius = sqrt(pow((x_center - x_end),2) + pow((y_center - y_end),2));
	if(radius<=0) radius = 1; //fail safe
	double start_angle = 0, end_angle = 0;

	//Computing start angle
	start_angle = atan2((_x - x_center),(y_center - _y))*(180/M_PI);
	if(start_angle<0) start_angle += 360;

	//Computing end angle
	end_angle = atan2((x_end-x_center),(y_center-y_end))*(180/M_PI);
	if(end_angle<0) end_angle += 360;

	//Draw CCW Arc
	if(abs(end_angle - start_angle) > 2)
	{
		if(end_angle > start_angle) start_angle += 360.0;
		for(i=(int)start_angle; i>(int)end_angle; i--)
		{
				cx = (x_center+(radius*sin((i)*(M_PI/180.0))));
				cy = (y_center-(radius*cos((i)*(M_PI/180.0))));
				plotter_goto((int)(cx), (int)(cy));
		}
	}
	plotter_goto((int)(x_end), (int)(y_end));
}

/**
 * Calibration procedure
 */
void plotter_go_home(void)
{
	diag_printf("Homing started! \n");
	//Checking
	//XY Calibration
	_zadX = -5000;
	_zadY = -5000;

	_delayX = 1;
	_delayY = 1;

	int xcounter = 0;
	int ycounter = 0;

	while( _x != _zadX || _y != _zadY)
	{
		if(hal_gpio_get_pin(LIM_X) == 0)
		{
			xcounter++;
			if(xcounter>=2)
			{
				_x = 0; _zadX=0;
			}
		}
		if(hal_gpio_get_pin(LIM_Y) == 0)
		{
			ycounter++;
			if (ycounter>=2)
			{
				_y = 0; _zadY=0;
			}
		}
		cyg_thread_delay(1);
	}
	_x =_zadX = _y = _zadY = 0;
	diag_printf("Homing finished! \n");
}

/**
 * Stepping X motor
 */
void stepX(int i)
{
	if (i)
		hal_gpio_set_pin(PUL_X);
	else
		hal_gpio_clear_pin(PUL_X);
}

/**
 * Stepping Y motor
 */
void stepY(int i)
{
	if (i)
		hal_gpio_set_pin(PUL_Y);
	else
		hal_gpio_clear_pin(PUL_Y);
}
