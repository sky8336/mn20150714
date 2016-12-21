#ifndef __S5PC100_LED_HHHH
#define __S5PC100_LED_HHHH

//need arg = 0/1/2/3
#define PWM_ON 	_IO('K', 0)
#define PWM_OFF _IO('K', 1)
#define SET_PRE _IOW('K', 2, int)
#define SET_CNT _IOW('K', 3, int)

#endif
