#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>

#include"sensorcali.h"
#include "sensor.h"


//int WriteDateToFile(const char *pathname, char *buf, size_t count);
void SwapData(SENSOR_DATA_T* a, SENSOR_DATA_T* b);
//int Filter(SENSOR_DATA_T* data, int size);
int SetAGM(void);
int Calibrated = 0;

SENSOR_CALIBRATION file_content ={
	{ 
		/*.calibrated*/1, /*0: not calibrated; 1: per-system calibrate; 2: per-model calibrate*/
		/*.spen = */0,
		/*.shake_th = */1800,
		/*.shake_shock = */50,
		/*.shake_quiet = */300,
		/*.accl_tolarence = */20,
		/*.gyro_tolarence = */400,
		/*.magn_tolarence = */50,
		/*.stableMS = */2000
	},
	{
		/*ULONG magnx,*/(ULONG)(-5),
		/*ULONG magny,*/305UL,
		/*ULONG magnz,*/(ULONG)(-118),
		/*ULONG magnxnx;*/(ULONG)(-15),
		/*ULONG magnxny;*/(ULONG)(-15),
		/*ULONG magnxnz;*/(ULONG)(-15),
		/*ULONG magnyux;*/(ULONG)(-15),
		/*ULONG magnyuy;*/(ULONG)(-15),
		/*ULONG magnyuz;*/(ULONG)(-15),
		/*ULONG magnxsx;*/(ULONG)(-15),
		/*ULONG magnxsy;*/(ULONG)(-15),
		/*ULONG magnxsz;*/(ULONG)(-15),
		/*ULONG magnydx;*/(ULONG)(-15),
		/*ULONG magnydy;*/(ULONG)(-15),
		/*ULONG magnydz;*/(ULONG)(-15),
		/*ULONG magnnx,*/(ULONG)(-15),
		/*ULONG magnny,*/268UL,
		/*ULONG magnnz,*/(ULONG)(-225),
		/*ULONG magnsx,*/(ULONG)(-13),
		/*ULONG magnsy,*/(ULONG)(-233),
		/*ULONG magnsz,*/241UL,
		/*ULONG acclx,*/8UL,
		/*ULONG accly,*/(ULONG)(-17),
		/*ULONG acclz,*/(ULONG)(-30),
		/*ULONG acclzx;//horizontal*/(ULONG)(-15),
		/*ULONG acclzy;*/(ULONG)(-15),
		/*ULONG acclzz;*/(ULONG)(-15),
		/*ULONG acclyx;//vertical*/(ULONG)(-15),
		/*ULONG acclyy;*/(ULONG)(-15),
		/*ULONG acclyz;*/(ULONG)(-15),
		/*ULONG gyrox,*/0UL,
		/*ULONG gyroy,*/(ULONG)(234),
		/*ULONG gyroz,*/(ULONG)(-242),
		/*ULONG gyrozx;*/(ULONG)(0),
		/*ULONG gyrozy;*/(ULONG)(-0),
		/*ULONG gyrozz;*/(ULONG)(-45),
		/*ULONG gyroyx;*/(ULONG)(-0),
		/*ULONG gyroyy;*/(ULONG)(-45),
		/*ULONG gyroyz;*/0UL,
		/*ULONG als_curve[20]*/12UL,0UL,24UL,0UL,35UL,1UL,47UL,4UL,59UL,10UL,71UL,28UL,82UL,75UL,94UL,206UL,106UL,565UL,118UL,1547UL,
		/*ULONG als_multiplier*/125UL

	}
};

#if 1
int WriteDateToFile(const char *pathname, char *buf, size_t count)
{
	int fd;

	if((fd = open(pathname,O_RDWR | O_CREAT | O_APPEND,0664)) == -1){
		perror("open");
		return -1;
	}

	//while((n = read(fd,buf,sizeof(buf))) > 0){
	write(fd,buf,count);
	//}

	close(fd);

	return 0;
}
#endif

#if 1
#if 1
void SwapData(SENSOR_DATA_T* a, SENSOR_DATA_T* b)
{
	SENSOR_DATA_T t;
	t.data.data.x = a->data.data.x;
	t.data.data.y = a->data.data.y;
	t.data.data.z = a->data.data.z;
	t.data.data.n = a->data.data.n;
	a->data.data.x = b->data.data.x;
	a->data.data.y = b->data.data.y;
	a->data.data.z = b->data.data.z;
	a->data.data.n = b->data.data.n;
	b->data.data.x = t.data.data.x;
	b->data.data.y = t.data.data.y;
	b->data.data.z = t.data.data.z;
	b->data.data.n = t.data.data.n;
}

int Filter(SENSOR_DATA_T* data, int size)
{
	//SENSOR_DATA_T a,g,m,sum;
	//unsigned long norm = 0;

	int i,j;
	//sort data
	for(i = 0; i < size*3; i++)
	{
		data[i].data.data.n = data[i].data.data.x*data[i].data.data.x+data[i].data.data.y*data[i].data.data.y+data[i].data.data.z*data[i].data.data.z;
	}
	for(i=0; i< size; i++)
	{
		for(j = 0; j <i; j++)
		{
			if(data[i*3].data.data.n <data[j*3].data.data.n)
			{ //swap
				SwapData(&data[i*3], &data[j*3]);
			}
			if(data[i*3+1].data.data.n <data[j*3+1].data.data.n)
			{ //swap
				SwapData(&data[i*3+1], &data[j*3+1]);
			}
			if(data[i*3+2].data.data.n <data[j*3+2].data.data.n)
			{ //swap
				SwapData(&data[i*3+2], &data[j*3+2]);
			}
		}
	}

	data[0].data.data.x = 0;
	data[0].data.data.y = 0;
	data[0].data.data.z = 0;
	data[1].data.data.x = 0;
	data[1].data.data.y = 0;
	data[1].data.data.z = 0;
	data[2].data.data.x = 0;
	data[2].data.data.y = 0;
	data[2].data.data.z = 0;
	for(i = 2; i< size-2; i++)
	{
		data[0].data.data.x += data[i*3].data.data.x;
		data[0].data.data.y += data[i*3].data.data.y;
		data[0].data.data.z += data[i*3].data.data.z;
		data[1].data.data.x += data[i*3+1].data.data.x;
		data[1].data.data.y += data[i*3+1].data.data.y;
		data[1].data.data.z += data[i*3+1].data.data.z;
		data[2].data.data.x += data[i*3+2].data.data.x;
		data[2].data.data.y += data[i*3+2].data.data.y;
		data[2].data.data.z += data[i*3+2].data.data.z;
	}
	data[0].data.data.x /=(size-4);
	data[0].data.data.y /=(size-4);
	data[0].data.data.z /=(size-4);
	data[1].data.data.x /=(size-4);
	data[1].data.data.y /=(size-4);
	data[1].data.data.z /=(size-4);
	data[2].data.data.x /=(size-4);
	data[2].data.data.y /=(size-4);
	data[2].data.data.z /=(size-4);
	return 0;
}

#endif

int SetAGM(void)
{
	SENSOR_DATA_T data;
	int in, i;
	static const int SKIP_DATA = 10;
	static const int DATA_SIZE = 11;
	SENSOR_DATA_T temp[DATA_SIZE*3];
	int S_OK = 0;
	int HReadA = S_OK;
	int HReadG = S_OK;
	int HReadM = S_OK;


	//calibrating
	//step 1: because ALS isn't calibrated,so we should get the value of ALS from firmware first

	printf("Please check sensor drivers status all ready!\n\n");
	printf("[1/6]Lay the device on a flat surface with the Windows button pointing due South\n");
	printf("Press enter when ready or press s to skip.\n");
	//FlushKeyBuffer();
	in = getchar();
#if 0
	if (in == 'q'||in == 'Q')
	{
		goto lable;
	}
	else if (in != 's'&&in != 'S')
	{
		//read the data...
		for(i =0; i < SKIP_DATA; i++)
		{// skip 10 data...
			HReadA = m_pSensorManagerEvents->ReadAcceSensor(data);
			HReadG = m_pSensorManagerEvents->ReadGyroSensor(data);
			HReadM = m_pSensorManagerEvents->ReadMagSensor(data);
			Sleep(75); //note: magn update time is 50ms
			printf("\b=>");
		}
		for(i = 0; i < DATA_SIZE; i++)
		{
			HReadA = m_pSensorManagerEvents->ReadAcceSensor(temp[i*3]);
			HReadG = m_pSensorManagerEvents->ReadGyroSensor(temp[i*3+1]);
			HReadM = m_pSensorManagerEvents->ReadMagSensor(temp[i*3+2]);
			Sleep(75); //note: magn update time is 50ms
			printf("\b=>");
		}
		printf("\n");
		//filt data
		//Filter(temp, DATA_SIZE);
		if (S_OK == HReadA)
		{
			file_content.calibration.acclzx = temp[0].data.accelMilliG.accelX;
			file_content.calibration.acclzy = temp[0].data.accelMilliG.accelY;
			file_content.calibration.acclzz = temp[0].data.accelMilliG.accelZ;
		}

		//cout<<"accl("<<temp[0].data.accelMilliG.accelX<<','<<temp[0].data.accelMilliG.accel<<','<<temp[0].data.accelMilliG.accelZ<<")"<<endl;
		if (S_OK == HReadG)
		{
			file_content.calibration.gyrox = temp[1].data.gyroMilliDegreesPerSecond.gyroX;
			file_content.calibration.gyroy = temp[1].data.gyroMilliDegreesPerSecond.gyroY;
			file_content.calibration.gyroz = temp[1].data.gyroMilliDegreesPerSecond.gyroZ;
		}
		if (S_OK == HReadM)
		{
			file_content.calibration.magnxnx = temp[2].data.magFieldMilliGauss.magFieldX;
			file_content.calibration.magnxny = temp[2].data.magFieldMilliGauss.magFieldY;
			file_content.calibration.magnxnz = temp[2].data.magFieldMilliGauss.magFieldZ;
		}
	}

#endif
	printf("The first step is completed\n");

	//step 2:
	printf("[2/6]Rotate the screen 180 degrees clockwise so the Windows button is pointing due North\n");
	printf("Press enter when ready or press s to skip.\n");
	//FlushKeyBuffer();
	in = getchar();
#if 0
	if (in == 'q'||in == 'Q')
	{
		goto lable;
	}
	else if (in != 's'&&in != 'S')
	{
		//read the data...
		for(i = 0; i < DATA_SIZE; i++)
		{
			HReadM = m_pSensorManagerEvents->ReadMagSensor(temp[i*3+2]);
			Sleep(75); //note: magn update time is 50ms
			printf("\b=>");
		}
		printf("\n");
		//filt data
		//Filter(temp, DATA_SIZE);
		if (S_OK == HReadM)
		{
			file_content.calibration.magnxsx = temp[2].data.magFieldMilliGauss.magFieldX;
			file_content.calibration.magnxsy = temp[2].data.magFieldMilliGauss.magFieldY;
		}
	}
#endif
	printf("The 2nd step is completed\n");

	//step 3:
	printf("[3/6]Now Lay the device flat, screen down with the windows button pointing due South\n");
	printf("Press enter when ready or press s to skip.\n");
	//FlushKeyBuffer();
	in = getchar();
#if 0
	if (in == 'q'||in == 'Q')
	{
		goto lable;
	}
	else if (in != 's'&&in != 'S')
	{
		//read the data...
		for(i = 0; i < DATA_SIZE; i++)
		{
			HReadM = m_pSensorManagerEvents->ReadMagSensor(temp[i*3+2]);
			Sleep(75); //note: magn update time is 50ms
			printf("\b=>");
		}
		printf("\n");
		//filt data
		//Filter(temp, DATA_SIZE);
		if (S_OK == HReadM)
		{
			file_content.calibration.magnxsz = temp[2].data.magFieldMilliGauss.magFieldZ;
		}
	}

#endif

	printf("The 3rd step is completed\n");

	//step 4:
	printf("[4/6]Lay the device flat with the screen up, rotate the device counter clockwise\n");
	printf("Press enter when ready or press s to skip.\n");
	//FlushKeyBuffer();
	in = getchar();
#if 0
	if (in == 'q'||in == 'Q')
	{
		goto lable;
	}
	else if (in != 's'&&in != 'S')
	{
		//read the data...
		static const int SAMPLES = 20;
		//SENSOR_DATA_T* buf = new SENSOR_DATA_T[SAMPLES*3];

		for(i = 0; i< SAMPLES;)
		{
			//Sleep(20); //gyro update is 20ms
			//HReadG= m_pSensorManagerEvents->ReadGyroSensor(data);
			if (S_OK == HReadG)
			{
				if(data.data.gyroMilliDegreesPerSecond.gyroX > 45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroX < -45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroY > 45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroY < -45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroZ > 45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroZ < -45000)
				{
					buf[i*3+1].data.gyroMilliDegreesPerSecond.gyroX = data.data.gyroMilliDegreesPerSecond.gyroX;
					buf[i*3+1].data.gyroMilliDegreesPerSecond.gyroY = data.data.gyroMilliDegreesPerSecond.gyroY;
					buf[i*3+1].data.gyroMilliDegreesPerSecond.gyroZ = data.data.gyroMilliDegreesPerSecond.gyroZ;
					HReadA = m_pSensorManagerEvents->ReadAcceSensor(data);
					if (S_OK == HReadA)
					{
						buf[i*3].data.accelMilliG.accelX = data.data.accelMilliG.accelX;
						buf[i*3].data.accelMilliG.accelY = data.data.accelMilliG.accelY;
						buf[i*3].data.accelMilliG.accelZ = data.data.accelMilliG.accelZ;
					}
					HReadM = m_pSensorManagerEvents->ReadMagSensor(data);
					if (S_OK == HReadM)
					{
						buf[i*3+2].data.magFieldMilliGauss.magFieldX = data.data.magFieldMilliGauss.magFieldX;
						buf[i*3+2].data.magFieldMilliGauss.magFieldY = data.data.magFieldMilliGauss.magFieldY;
						buf[i*3+2].data.magFieldMilliGauss.magFieldZ = data.data.magFieldMilliGauss.magFieldZ;
					}
					i++;
					printf("\b=>");
				}
			}
			else
			{
				HReadA = m_pSensorManagerEvents->ReadAcceSensor(data);
				if (S_OK == HReadA)
				{
					buf[i*3].data.accelMilliG.accelX = data.data.accelMilliG.accelX;
					buf[i*3].data.accelMilliG.accelY = data.data.accelMilliG.accelY;
					buf[i*3].data.accelMilliG.accelZ = data.data.accelMilliG.accelZ;
				}
				HReadM = m_pSensorManagerEvents->ReadMagSensor(data);
				if (S_OK == HReadM)
				{
					buf[i*3+2].data.magFieldMilliGauss.magFieldX = data.data.magFieldMilliGauss.magFieldX;
					buf[i*3+2].data.magFieldMilliGauss.magFieldY = data.data.magFieldMilliGauss.magFieldY;
					buf[i*3+2].data.magFieldMilliGauss.magFieldZ = data.data.magFieldMilliGauss.magFieldZ;
				}
				i++;
				printf("\b=>");
			}
			Sleep(30);
		}
		printf("\n");
		//filt data
		//Filter(buf, SAMPLES);
		if (S_OK == HReadG)
		{
			file_content.calibration.gyrozx = buf[1].data.gyroMilliDegreesPerSecond.gyroX;
			file_content.calibration.gyrozy = buf[1].data.gyroMilliDegreesPerSecond.gyroY;
			file_content.calibration.gyrozz = buf[1].data.gyroMilliDegreesPerSecond.gyroZ;
		}
		/*
		//calibrate magnatic
		CalibrateZ(buf, SAMPLES, offsetx, offsety, offsetz);
		*/
		//delete []buf;

	}
#endif
	printf("The 4th step is completed\n");

	//step 5:
	printf("[5/6]Hold the device vertical with the windows button on the bottom, rotate the device counter clockwise along the axis between the top of the screen and the windows button\n");
	printf("Press enter when ready or press s to skip.\n");
	//FlushKeyBuffer();
	in = getchar();

#if 0
	if (in == 'q'||in == 'Q')
	{
		goto lable;
	}
	else if (in != 's'&&in != 'S')
	{
		//read the data...
		static const int SAMPLES = 20;
		//SENSOR_DATA_T* buf = new SENSOR_DATA_T[SAMPLES*3];

		for(i = 0; i< SAMPLES;)
		{
			Sleep(20); //gyro update is 20ms
			HReadG = m_pSensorManagerEvents->ReadGyroSensor(data);
			if (S_OK == HReadG)
			{
				if(data.data.gyroMilliDegreesPerSecond.gyroX > 45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroX < -45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroY > 45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroY < -45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroZ > 45000 ||
						data.data.gyroMilliDegreesPerSecond.gyroZ < -45000)
				{
					buf[i*3+1].data.gyroMilliDegreesPerSecond.gyroX = data.data.gyroMilliDegreesPerSecond.gyroX;
					buf[i*3+1].data.gyroMilliDegreesPerSecond.gyroY = data.data.gyroMilliDegreesPerSecond.gyroY;
					buf[i*3+1].data.gyroMilliDegreesPerSecond.gyroZ = data.data.gyroMilliDegreesPerSecond.gyroZ;
					HReadA = m_pSensorManagerEvents->ReadAcceSensor(data);
					if (S_OK == HReadA)
					{
						buf[i*3].data.accelMilliG.accelX = data.data.accelMilliG.accelX;
						buf[i*3].data.accelMilliG.accelY = data.data.accelMilliG.accelY;
						buf[i*3].data.accelMilliG.accelZ = data.data.accelMilliG.accelZ;
					}
					HReadM = m_pSensorManagerEvents->ReadMagSensor(data);
					if (S_OK == HReadM)
					{
						buf[i*3+2].data.magFieldMilliGauss.magFieldX = data.data.magFieldMilliGauss.magFieldX;
						buf[i*3+2].data.magFieldMilliGauss.magFieldY = data.data.magFieldMilliGauss.magFieldY;
						buf[i*3+2].data.magFieldMilliGauss.magFieldZ = data.data.magFieldMilliGauss.magFieldZ;
					}
					i++;
					printf("\b=>");
				}
			}
			else
			{
				HReadA = m_pSensorManagerEvents->ReadAcceSensor(data);
				if (S_OK == HReadA)
				{
					buf[i*3].data.accelMilliG.accelX = data.data.accelMilliG.accelX;
					buf[i*3].data.accelMilliG.accelY = data.data.accelMilliG.accelY;
					buf[i*3].data.accelMilliG.accelZ = data.data.accelMilliG.accelZ;
				}
				HReadM = m_pSensorManagerEvents->ReadMagSensor(data);
				if (S_OK == HReadM)
				{
					buf[i*3+2].data.magFieldMilliGauss.magFieldX = data.data.magFieldMilliGauss.magFieldX;
					buf[i*3+2].data.magFieldMilliGauss.magFieldY = data.data.magFieldMilliGauss.magFieldY;
					buf[i*3+2].data.magFieldMilliGauss.magFieldZ = data.data.magFieldMilliGauss.magFieldZ;
				}
				i++;
				printf("\b=>");
			}
			Sleep(30);
		}
		printf("\n");
		//filt data
		//Filter(buf, SAMPLES);
		if (S_OK == HReadG)
		{
			file_content.calibration.gyroyx = buf[1].data.gyroMilliDegreesPerSecond.gyroX;
			file_content.calibration.gyroyy = buf[1].data.gyroMilliDegreesPerSecond.gyroY;
			file_content.calibration.gyroyz = buf[1].data.gyroMilliDegreesPerSecond.gyroZ;
		}
		if (S_OK == HReadA)
		{
			file_content.calibration.acclyx = buf[0].data.accelMilliG.accelX;
			file_content.calibration.acclyy = buf[0].data.accelMilliG.accelY;
			file_content.calibration.acclyz = buf[0].data.accelMilliG.accelZ;
		}
		/*
		//calibrate magnatic
		CalibrateZ(buf, SAMPLES, offsetx, offsety, offsetz);
		*/
		//delete []buf;

	}
#endif
	printf("The 5th step is completed\n");

	Calibrated = 1;
	file_content.config.calibrated = 1;
	//WriteDataToFile(AGM_SENSOR_FILE_NAME,&file_content,sizeof(file_content));

	printf("Done.\n");
	return 0;

lable:
	printf("End.press any key to continue\n");
	getchar();
	return -1;
}
#endif

#if 1
int main(int argc, const char *argv[])
{
	char buf[10] = {1,2,3,4};
	ssize_t n = 4;


	sprintf(buf,"%s","abcde");
	WriteDateToFile("./caliData.txt",buf,n);

	return 0;
}
#endif
