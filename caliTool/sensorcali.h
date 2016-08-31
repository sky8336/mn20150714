#define  ULONG  unsigned long
#define AGM_SENSOR_FILE_NAME	"./sensor_calibration_AGM.bin"

typedef struct _FUSION_CONFIG
{
	ULONG calibrated;
	ULONG spen;
	ULONG shake_th;
	ULONG shake_shock;
	ULONG shake_quiet;
	ULONG accl_tolarence;
	ULONG gyro_tolarence;
	ULONG magn_tolarence;
	ULONG stableMS;
} FUSION_CONFIG, *PFUSION_CONFIG;

typedef struct _FUSION_CALIBRATION
{
	ULONG magnx;
	ULONG magny;
	ULONG magnz;
	ULONG magnxnx;
	ULONG magnxny;
	ULONG magnxnz;
	ULONG magnyux;
	ULONG magnyuy;
	ULONG magnyuz;
	ULONG magnxsx;
	ULONG magnxsy;
	ULONG magnxsz;
	ULONG magnydx;
	ULONG magnydy;
	ULONG magnydz;
	ULONG magnnx;
	ULONG magnny;
	ULONG magnnz;
	ULONG magnsx;
	ULONG magnsy;
	ULONG magnsz;
	ULONG acclx;
	ULONG accly;
	ULONG acclz;
	ULONG acclzx;//horizontal
	ULONG acclzy;
	ULONG acclzz;
	ULONG acclyx;//vertical
	ULONG acclyy;
	ULONG acclyz;
	ULONG gyrox;
	ULONG gyroy;
	ULONG gyroz;
	ULONG gyrozx;//horizontal
	ULONG gyrozy;
	ULONG gyrozz;
	ULONG gyroyx;//vertical
	ULONG gyroyy;
	ULONG gyroyz;
	ULONG alscurve[20];
	ULONG als_multiplier;
} FUSION_CALIBRATION, *PFUSION_CALIBRATION;

typedef struct _SENSOR_CALIBRATION
{
	FUSION_CONFIG config;
	FUSION_CALIBRATION calibration;
} SENSOR_CALIBRATION, *PSENSOR_CALIBRATION;

