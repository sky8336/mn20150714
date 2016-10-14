
// ************
// * INCLUDES *
// ************

#ifndef MAG_AKM_AK09911_H_
#define MAG_AKM_AK09911_H_

// *********************
// * MACRO DEFINITIONS *
// *********************

#define AK09911_STATUS_SUCCESS 0
#define AK09911_STATUS_FAILED 1

typedef char unsigned u8;
typedef short unsigned u16;
typedef short s16;
typedef int ak09911_status;

// I2C addresses of the sensor
#define AK09911_SENSOR_ADDRESS_1 0x18

#define AK09911_SUPPORTED_FREQUENCIES {0}

#define AK09911_FUSE_ASAX           0x60
#define AK09911_FUSE_ASAY           0x61
#define AK09911_FUSE_ASAZ           0x62

#define AK09911_MODE_POWERDOWN      0x00
#define AK09911_MODE_SNG_MEASURE    0x01
#define AK09911_MODE_CONTINUOUS_10HZ      0x02
#define AK09911_MODE_CONTINUOUS_20HZ      0x04
#define AK09911_MODE_CONTINUOUS_50HZ      0x06
#define AK09911_MODE_CONTINUOUS_100HZ      0x08
#define AK09911_MODE_FUSE_ACCESS    0x1F

#define AK0991X_CNTL2_POWER_DOWN			0x00
#define AK0991X_CNTL2_SNG_MEASURE			0x01
#define AK0991X_CNTL2_CONT_MEASURE_MODE1	0x02         //10 hz
#define AK0991X_CNTL2_CONT_MEASURE_MODE2	0x04         //20 hz
#define AK0991X_CNTL2_CONT_MEASURE_MODE3	0x06         //50 hz
#define AK0991X_CNTL2_CONT_MEASURE_MODE4	0x08         // 100 Hz
#define AK0991X_CNTL2_SELF_TEST				0x10
#define AK0991X_CNTL2_FUSE_ACCESS			0x1F

#define AK09911_ST1_DRDY	(0x1 << 0)

#define AK09911_ST2_HOFL	(0x1 << 3)

#define AK0991X_WIA1			0x00
#define AK0991X_WIA2			0x01
#define AK0991X_INFO1			0x02
#define AK0991X_INFO2			0x03
#define AK0991X_ST1				0x10
#define AK0991X_HXL				0x11
#define AK0991X_HXH				0x12
#define AK0991X_HYL				0x13
#define AK0991X_HYH				0x14
#define AK0991X_HZL				0x15
#define AK0991X_HZH				0x16
#define AK0991X_TMPS			0x17
#define AK0991X_ST2				0x18
#define AK0991X_CNTL1			0x30
#define AK0991X_CNTL2			0x31
#define AK0991X_CNTL3			0x32
#define AK0991X_FUSE_ASAX			0x60
#define AK0991X_FUSE_ASAY			0x61
#define AK0991X_FUSE_ASAZ			0x62

#define AK0991X_WIA1_COMP_CODE	(0x48)
#define AK09912_WIA2_DEVICE_ID	(0x04)
#define AK09911_WIA2_DEVICE_ID	(0x05)
#define AK09916_WIA2_DEVICE_ID	(0x09)

// *********************
// * STRUCT DEFINITION *
// *********************
typedef struct _ak09911_private_data
{
	//TODO: Put any global variables that you want to use for your uDriver in this struct.
	//Global variables outside of this private_data struct are at risk of being overwritten if you have multiple instances of the same sensor, unless they are protected by a semaphore or other means of value locking.
	//You may add more variables to this struct, and it may remain empty if no data is needed.
	u8 asax;
	u8 asay;
	u8 asaz;
}ak09911_private_data;

typedef struct {
	s16 x;/**<magn X  data*/
	s16 y;/**<magn Y  data*/
	s16 z;/**<magn Z  data*/
}ak09911_xyz_t;


// ********************************
// * FUNCTION FORWARD DECLARATION *
// ********************************

ak09911_status ak09911_read_data(ak09911_xyz_t* data);
ak09911_status ak09911_self_test(void);
ak09911_status ak09911_set_report_interval(u16* report_interval);
void ak09911_init(void);

#endif
