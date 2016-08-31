
typedef struct _SENSOR_DATA {
    union {
        /*! \struct accelMilliG 
        *  \brief Accelerometer sensor data.
        * */
        struct {
            int accelX; /*!< \brief X axis data */
            int accelY; /*!< \brief Y axis data */
            int accelZ; /*!< \brief Z axis data */
        } accelMilliG;

        struct {
            int gyroX; /*!< \brief X axis data */
            int gyroY; /*!< \brief Y axis data */
            int gyroZ; /*!< \brief Z axis data */
        } gyroMilliDegreesPerSecond;

        struct {
            int magFieldX; /*!< \brief X axis data */
            int magFieldY; /*!< \brief Y axis data */
            int magFieldZ; /*!< \brief Z axis data */
        }magFieldMilliGauss;
	struct{
	    unsigned int illuminance;
	} alsLux;
        struct {
            int x;
            int y;
            int z;
            int n;
        } data;
    }data;
}SENSOR_DATA_T;
