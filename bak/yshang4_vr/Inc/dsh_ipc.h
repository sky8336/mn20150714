/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __DSH_IPC_H
#define __DSH_IPC_H

#include "stdint.h"
/*discrete sensor hub IPC protocol*/

/*
1. host2dsh command with priority (high to low)
	1.1 CMD_CFG_STREAM
	1.2 CMD_STOP_STREAM
	1.3 CMD_GET_STATUS
	1.4 CMD_IA_NOTIFY 
2. dsh2host response
	2.1 RESP_CMD_ACK
	2.2 RESP_STREAMING
	2.3 PREP_GET_STATUS 
*/

//host2dsh IPC CMD: 
typedef enum {
  CMD_CFG_STREAM        = 3,
  CMD_STOP_STREAM       = 4,
  CMD_GET_STATUS        = 11,
  CMD_IA_NOTIFY         = 15
}cmd_type_t;

/*
general format: [uint8_t tran_id][uint8_t cmd_id][uint8_t sensor_id][uint8_t param[] ]
*/
#define CMD_PARAM_MAX_SIZE ((uint16_t)13)
__packed struct ia_cmd {
        uint8_t tran_id;   // normally 0 
        uint8_t cmd_id;    // 3 for CMD_CFG_STREAM
        uint8_t sensor_id; // refer to 2.3 sensor_id definition 
        char param[CMD_PARAM_MAX_SIZE];
};

/*1.1 CMD_CFG_STREAM 
format: [uint8_t tran_id][uint8_t cmd_id = 3][uint8_t sensor_id][struct sensor_cfg]
usage: host side send this command to DSH to configure sensor stream behavior
*/

__packed struct sensor_cfg_param {
	uint16_t sample_freq; /* HZ */   // maximum sensor frequency 
	uint16_t buff_delay; /* max time(ms) for data bufferring */  // normally 0 :for VR
	uint16_t bit_cfg;    // keep blank 
};

//Note: DSH needs ACK to host after process CMD_CFG_STREAM, refer to DSH2HOST IPC format RESP_CMD_ACK

/*1.2 CMD_STOP_STREAM 
format: [uint8_t tran_id][uint8_t 4][uint8_t sensor_id]
usage: host send this command to DSH to stop a running sensor. 
*/

//Note: DSH needs ACK to host after process CMD_STOP_STREAM, refer to DSH2HOST IPC format RESP_CMD_ACK 

/*1.3 CMD_GET_STATUS 
format: [uint8_t tran_id][uint8_t 11][struct get_status_param]
usage: host send this command to query status sensor table from DSH, 
	   including sensor name, sensor ID, max frequency, running status etc
	   each bit of snr_bitmask is corresponding to a senosr
	   0xFFFFFFFFFFFFFFFF will cover all possible sensors in the system
The get status result packet format refer to following dsh2host protocol  
*/
__packed struct get_status_param {
	uint32_t snr_bitmask;
};

/*1.4 CMD_IA_NOTIFY 
format: [uint8_t tran_id][uint8_t cmd_id=15][sensor_id=0][struct cmd_ia_notify_param]
usage: host send this command to DSH to sync timestamp
*/

#define IA_NOTIFY_TIMESTAMP_SYNC  ((uint8_t)0x3)

typedef int64_t timestamp_t;
__packed struct cmd_ia_notify_param {
	uint8_t id;  // IA_NOTIFY_TIMESTAMP_SYNC 0x3 
	int64_t linux_base_ns; 
};


//dsh2host IPC CMD: 
//[uint16_t sign][uint16_t packet_len][uint8_t trans_id][uint8_t cmd_type][uint8_t sensor_id][uint16_t sensor_data_len][n byte data]
typedef enum {
  RESP_CMD_ACK          = 0,
  RESP_STREAMING        = 3,
  RESP_GET_STATUS       = 11
}resp_type_t;

#define LBUF_CELL_SIGN ((uint16_t)0x4853)
#define LBUF_DISCARD_SIGN ((uint16_t)0x4944)

struct frame_head {
	uint16_t sign;    //LBUF_CELL_SIGN for valid packet and LBUF_DISCARD_SIGN for invalid packet
	uint16_t length;  // total packet length including frame_head and cmd_resp
};

__packed struct cmd_resp {
        uint8_t tran_id;  // leave 0
        uint8_t type;     // cmd_type, 0=RESP_CMD_ACK, 3=RESP_STREAMING, 11=RESP_GET_STATUS 
        uint8_t sensor_id; // refer to 2.4 sensor_id definition 
        uint16_t data_len; // length of [n byte data]
        char buf[0];
};

/*2.1 RESP_CMD_ACK 
format: [uint16_t sign][uint16_t packet_len][uint8_t trans_id][uint8_t cmd_type=0][uint8_t sensor_id=0][uint16_t sensor_data_len=5][struct resp_cmd_ack]
usage: dsh send this ACK to confirm received and whether the command parsing is correct or not 
note: All commands except "CMD_IA_NOTIFY" need send ACK from dsh to host 
*/
__packed struct resp_cmd_ack {
 uint8_t cmd_id; // which cmd ACKed in this packet
 int32_t ret;   // ret code
 uint8_t extra[0];  // leave 0
};

// ret code 
#define E_SUCCESS       ((int)(0))
#define E_GENERAL		((int)(-1))
#define E_NOMEM			((int)(-2))
#define E_PARAM			((int)(-3))
#define E_BUSY			((int)(-4))
#define E_HW			((int)(-5))
#define E_NOSUPPORT		((int)(-6))
#define E_RPC_COMM		((int)(-7))
#define E_LPE_COMM		((int)(-8))
#define E_CMD_ASYNC		((int)(-9))
#define E_CMD_NOACK		((int)(-10))
#define E_LBUF_COMM		((int)(-11))

/*2.2 RESP_STREAMING 
format: [uint16_t sign][uint16_t packet_len][uint8_t trans_id][uint8_t cmd_type=3][uint8_t sensor_id][uint16_t sensor_data_len][u64 timestamp][n byte sensor data]
usage: dsh send sensor data packet to host
Note: The timestamp is the last data sample timestamp, can be blank for VR 
*/

/* data format of each sensor type */

typedef struct _xyz_data {
	int16_t x; 
	int16_t y; 
	int16_t z;
}XYZ_DATA_T;

__packed struct accel_data {
	int64_t ts;  // timestamp for this sample
	int16_t x;     // x-axis value
	int16_t y;     // y-axis value
	int16_t z;     // z-axis value 
};

__packed struct gyro_raw_data {
	int64_t ts;
	int16_t x;
	int16_t y;
	int16_t z;
};

__packed struct compass_raw_data {
	int64_t ts;
	int16_t x;
	int16_t y;
	int16_t z;
};

__packed struct rot_raw_data {
	int64_t ts;
	int16_t matrix[9];
};

typedef __packed struct _sensor_data_type {
	int64_t ts;
	union
	{
		XYZ_DATA_T accel;
		XYZ_DATA_T gyro;
		XYZ_DATA_T compass;
		int16_t rot[9];
	};
}SENSOR_DATA_T, *pSENSOR_DATA;

/*2.3 RESP_GET_STATUS 
format: [uint16_t packet_len][uint8_t trans_id][uint8_t cmd_type=11][uint8_t sensor_id][uint16_t sensor_data_len][n byte sensor data]
usage: dsh send this sensors description to host for sensor enumeration 
Note: This is not a single response – there would be N packets and each for a dedicated sensor. 
*/

#define LINK_AS_CLIENT		(0)
#define LINK_AS_MONITOR		(1)
#define LINK_AS_REPORTER	(2)
__packed struct link_info {
	uint8_t sid;
	uint8_t ltype;
	uint16_t rpt_freq;
};
#define SNR_NAME_MAX_LEN 6

__packed struct snr_info {
	uint8_t sid;  // must be filled
	uint8_t status; //0
	uint16_t freq; // must be filled
	uint16_t data_cnt;  // 0
	uint16_t bit_cfg;   // 0
	uint16_t priv;      // 0
	uint16_t attri;     // 0

	uint16_t freq_max;   // must be filled
	char name[SNR_NAME_MAX_LEN];  // must be filled 

	uint8_t health;     // 0
	uint8_t link_num;   // fill with 0
	struct link_info linfo[0];  // leave blank  
};
#define SNR_INFO_SIZE(sinfo) (sizeof(struct snr_info) \
		+ sinfo->link_num * sizeof(struct link_info))
#define SNR_INFO_MAX_SIZE 256


// sensor_id definition 
typedef enum {
    SENSOR_INVALID = -1,
	SENSOR_ACCELEROMETER = 1,
	SENSOR_GYRO,
	SENSOR_COMP,
	SENSOR_ROT,
	SENSOR_ACC_RAW,
	SENSOR_GYRO_RAW,
	SENSOR_COMP_RAW,
	SENSOR_BARO,
	SENSOR_ALS,
	SENSOR_PROXIMITY,
	SENSOR_TC,
	SENSOR_LPE,
	SENSOR_ACCELEROMETER_SEC,
	SENSOR_GYRO_SEC,
	SENSOR_COMP_SEC,
	SENSOR_ALS_SEC,
	SENSOR_PROXIMITY_SEC,
	SENSOR_BARO_SEC,

	SENSOR_ACTIVITY,
	SENSOR_GS,
	SENSOR_GESTURE_FLICK,

	SENSOR_ROTATION_VECTOR,
	SENSOR_GRAVITY,
	SENSOR_LINEAR_ACCEL,
	SENSOR_ORIENTATION,
	SENSOR_CALIBRATION_COMP,
	SENSOR_CALIBRATION_GYRO,
	SENSOR_9DOF,
	SENSOR_PEDOMETER,
	SENSOR_MAG_HEADING,
	SENSOR_SHAKING,
	SENSOR_MOVE_DETECT,
	SENSOR_STAP,
	SENSOR_PAN_TILT_ZOOM,
	SENSOR_LIFT_VERTICAL, /*invalid sensor, leave for furture using*/
	SENSOR_DEVICE_POSITION,
	SENSOR_STEPCOUNTER,
	SENSOR_STEPDETECTOR,
	SENSOR_SIGNIFICANT_MOTION,
	SENSOR_GAME_ROTATION_VECTOR,
	SENSOR_GEOMAGNETIC_ROTATION_VECTOR,
	SENSOR_6DOFAG,
	SENSOR_6DOFAM,
	SENSOR_LIFT,
	SENSOR_DTWGS,
	SENSOR_GESTURE_HMM,
	SENSOR_GESTURE_EARTOUCH,
	SENSOR_PEDESTRIAN_DEAD_RECKONING,
	SENSOR_INSTANT_ACTIVITY,
	SENSOR_DIRECTIONAL_SHAKING,
	SENSOR_GESTURE_TILT,
	SENSOR_GESTURE_SNAP,
	SENSOR_PICKUP,
	SENSOR_TILT_DETECTOR,

	SENSOR_BIST,

	SENSOR_EVENT,
	SENSOR_MAX
} psh_sensor_t;

static uint16_t sensor_t_size[] = 
{0, sizeof(struct accel_data), sizeof(struct gyro_raw_data),
sizeof(struct compass_raw_data), sizeof(struct rot_raw_data),
sizeof(struct accel_data), sizeof(struct gyro_raw_data),
sizeof(struct compass_raw_data), };

// sensor name map table
struct sensor_name {
        char name[SNR_NAME_MAX_LEN + 1];
};

static struct sensor_name sensor_type_to_name_str[SENSOR_MAX] = {
	{"ACCEL"}, {"GYRO"}, {"COMPS"}, {"BARO"}, {"ALS_P"}, {"PS_P"}, {"TERMC"}, {"LPE_P"},
	{"ACC1"}, {"GYRO1"}, {"COMP1"}, {"ALS1"}, {"PS1"}, {"BARO1"}, {"PHYAC"}, {"GSSPT"},
	{"GSFLK"}, {"RVECT"}, {"GRAVI"}, {"LACCL"}, {"ORIEN"}, {"COMPC"}, {"GYROC"}, {"9DOF"},
	{"PEDOM"}, {"MAGHD"}, {"SHAKI"}, {"MOVDT"}, {"STAP"}, {"PZOOM"}, {"LTVTL"}, {"DVPOS"},
	{"SCOUN"}, {"SDET"}, {"SIGMT"}, {"6AGRV"}, {"6AMRV"}, {"6DOFG"}, {"6DOFM"}, {"LIFT"},
	{"DTWGS"}, {"GSPX"}, {"GSETH"}, {"PDR"}, {"ISACT"}, {"DSHAK"}, {"GTILT"}, {"GSNAP"},
	{"PICUP"}, {"TILTD"}, {"BIST"}, {"EVENT"}
};


enum {
  BUFF_EMPTY,
  BUFF_TRANSFER,
  BUFF_FULL,
  BUFF_IN_USE
};



typedef __packed struct stream_cfg_param {
  uint8_t isStream;
  uint8_t isUpdate;
  struct sensor_cfg_param cfg;
}stream_cfg_t;

#define SENSOR_ID_MAX  9
#define HEAD_FRAME_SIZE sizeof(struct frame_head)
#define RESP_SIZE sizeof(struct cmd_resp)
#define RESP_BUFF_SIZE 512

typedef enum{
	STA_IDLE = 0,
	STA_WAIT_CMD,
	STA_PARSE_CMD,
	STA_RESP_ACK,
	STA_PROCESS,
	STA_PREPARE_DATA,
	STA_COMMIT_DATA
}status_t;

typedef struct _dsh 
{
  struct ia_cmd ia_buff;
  struct frame_head head_buff;

  //double buffer for cmd_resp
  char resp_buff[RESP_BUFF_SIZE];
      
  int ret_err;
  uint16_t resp_frame_length;
  resp_type_t resp_type;
  status_t status_;
  
  psh_sensor_t single_sensor;
  
  uint8_t streaming_mode;
  //TODO: Change to list?
  stream_cfg_t stream_sensor[SENSOR_ID_MAX];
  
  uint32_t snr_bitmask;
  int64_t host_base_ns;
  uint16_t max_freq;  

  struct accel_data accel;
  struct gyro_raw_data gyro;
  struct compass_raw_data magn;
  struct rot_raw_data rot;

  struct accel_data accel_raw;
  struct gyro_raw_data gyro_raw;
  struct compass_raw_data magn_raw;
}dsh, *pdsh_handle;



void transmit_resp(pdsh_handle pDsh_handle);
void parse_ia_cmd(pdsh_handle pDsh_handle);
void resp_ack(pdsh_handle pDsh_handle);
void prepare_data(pdsh_handle pDsh_handle);
void fill_frame_head(void* buf, uint16_t length);
void transmit_data(char* buf, uint16_t length);
void process_cmd(pdsh_handle pDsh_handle);
pdsh_handle Dsh_init(void);
#endif
