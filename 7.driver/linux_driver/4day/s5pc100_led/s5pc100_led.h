#ifndef S5PC100_LED_HHH
#define S5PC100_LED_HHH

//这两个命令需要传递参数 arg=0/1/2/3 分别对应4个灯
#define LED_ON  _IOW('L', 0, int)
#define LED_OFF  _IOW('L', 1, int)

struct led_action 
{
	int act; //开灯还是关灯 LED_ON开灯  LED_OFF 关灯
	int nr;  //开第几个灯  范围 0/1/2/3
};

#endif
