#ifndef led_driver_HHHHHHHHHHHHHHHHHHHHHHHHHH
#define led_driver_HHHHHHHHHHHHHHHHHHHHHHHHHH
#define led_on _IOW('M',0,int)
#define led_off _IOW('M',1,int)

struct led_action{
	int act;
	int nr;
};
#endif
