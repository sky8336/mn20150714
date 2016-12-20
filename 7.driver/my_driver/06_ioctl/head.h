#ifndef __HEAD_H__
#define __HEAD_H__


#define LED_ON _IO('K',0)
//#define LED_OFF _IO('K',1)
#define LED_OFF _IOW('K',1,int) 

#endif
