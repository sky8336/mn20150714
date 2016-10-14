
#include "i2c.h"
#include "ak09911.h"

static ak09911_private_data ak09911_privite_data;
static const char AKM_AK09911_DEBUG_STR[] = "AK09911";
static uint32_t I2C_Timeout = 1000;    /*<! Value of Timeout when I2C communication fails */

static ak09911_status ak09911_i2c_write(u8 addr, u8* data, u8 len)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    status = HAL_I2C_Mem_Write( &hi2c1, AK09911_SENSOR_ADDRESS_1, ( uint16_t )addr, I2C_MEMADD_SIZE_8BIT, data, len, I2C_Timeout );
                                
    if( status != HAL_OK )
    {
      return AK09911_STATUS_FAILED;
    }
    else
    {
      return AK09911_STATUS_SUCCESS;
    }
}

static ak09911_status ak09911_i2c_read(u8 addr, u8* data, u8 len)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    status = HAL_I2C_Mem_Read( &hi2c1, AK09911_SENSOR_ADDRESS_1, ( uint16_t )addr, I2C_MEMADD_SIZE_8BIT, data, len, I2C_Timeout );
                                
    if( status != HAL_OK )
    {
      return AK09911_STATUS_FAILED;
    }
    else
    {
      return AK09911_STATUS_SUCCESS;
    }
}

#define ak09911_log(...)

static int ak09911_power_mode(u16 *interval)
{
	u8 tmp;
	ak09911_status ret;
	u16 tmp_interval = *interval;

	tmp = AK0991X_CNTL2_POWER_DOWN;
	ret = ak09911_i2c_write(AK0991X_CNTL2, &tmp, 1);
	if(ret != AK09911_STATUS_SUCCESS){
		ak09911_log("[%s] set power down mode failed\n", AKM_AK09911_DEBUG_STR);
        return AK09911_STATUS_FAILED;
	}

	if (tmp_interval >= 100) {
		tmp = AK0991X_CNTL2_CONT_MEASURE_MODE1; // 10Hz
		*interval = 100;
	} else if (tmp_interval >= 50) {
		tmp = AK0991X_CNTL2_CONT_MEASURE_MODE2; // 20Hz
		*interval = 50;
	} else if (tmp_interval >= 20) {
		tmp = AK0991X_CNTL2_CONT_MEASURE_MODE3; // 50Hz
		*interval = 20;
	} else if (tmp_interval > 0) {
		tmp = AK0991X_CNTL2_CONT_MEASURE_MODE4; // 100Hz
		*interval = 10;
	} else {
		tmp = AK0991X_CNTL2_POWER_DOWN;
		*interval = 0;
		return AK09911_STATUS_SUCCESS;
	}

	return ak09911_i2c_write(AK0991X_CNTL2, &tmp, 1);
}

static ak09911_status ak09911_get_fuse_data(void)
{
	ak09911_status ret;
	u8 tmpval[3];
	ak09911_private_data *private_data = &ak09911_privite_data;

	tmpval[0] = AK09911_MODE_FUSE_ACCESS;
	ret = ak09911_i2c_write(AK0991X_CNTL2, tmpval, 1);
	if(ret != AK09911_STATUS_SUCCESS){
		ak09911_log("%s %d: [%s] change to fuse mode failed\n",__func__,__LINE__,AKM_AK09911_DEBUG_STR);
        return AK09911_STATUS_FAILED;
	}

	ret = ak09911_i2c_read(AK09911_FUSE_ASAX, tmpval, 3);
	if(ret != AK09911_STATUS_SUCCESS)
	{
		ak09911_log("%s %d: [%s] read ASA failed \n",__func__,__LINE__,AKM_AK09911_DEBUG_STR);
        return AK09911_STATUS_FAILED;
	}

	private_data->asax = tmpval[0];
	private_data->asay = tmpval[1];
	private_data->asaz = tmpval[2];

	tmpval[0] = AK0991X_CNTL2_POWER_DOWN;
	ret = ak09911_i2c_write(AK0991X_CNTL2, tmpval, 1);
	if(ret != AK09911_STATUS_SUCCESS){
		ak09911_log("[%s] set power down mode failed\n", AKM_AK09911_DEBUG_STR);
        return AK09911_STATUS_FAILED;
	}

	ak09911_log("[%s] ASA:%d %d %d \n",AKM_AK09911_DEBUG_STR, private_data->asax, private_data->asay, private_data->asaz);

	return AK09911_STATUS_SUCCESS;
}

static ak09911_status compass_ak0991x_validate(void)
{
	ak09911_status ret;
	u8 buf[2];

	ret = ak09911_i2c_read(AK0991X_WIA1, buf, 2);
	if (ret != AK09911_STATUS_SUCCESS)
		return ret;

	if (buf[0] != AK0991X_WIA1_COMP_CODE) {
		ak09911_log("%s %d: [%s] Wrong Company ID:%d \n",__func__,__LINE__,AKM_AK09911_DEBUG_STR, buf[0]);
		return AK09911_STATUS_FAILED;
	}

	if (buf[1] != AK09916_WIA2_DEVICE_ID) {
		ak09911_log("%s %d: [%s] Wrong Device ID:%d \n",__func__,__LINE__,AKM_AK09911_DEBUG_STR, buf[1]);
		return AK09911_STATUS_FAILED;
	}

	return AK09911_STATUS_SUCCESS;
}

void ak09911_init(void)
{
	ak09911_status ret = AK09911_STATUS_SUCCESS;


	ret = compass_ak0991x_validate();
	if(ret != AK09911_STATUS_SUCCESS){
		ak09911_log("%s %d: [%s] ak09911 doesn't exsist \n",__func__,__LINE__,AKM_AK09911_DEBUG_STR);
		return;
	}

	ret = ak09911_get_fuse_data();
	if(ret != AK09911_STATUS_SUCCESS){
		ak09911_log("%s %d: [%s] get asa data error \n",__func__,__LINE__,AKM_AK09911_DEBUG_STR);
		return;
	}

	ak09911_log("%s create successfully \n",AKM_AK09911_DEBUG_STR);

}

ak09911_status ak09911_read_data(ak09911_xyz_t *data)
{
	ak09911_private_data *private_data = &ak09911_privite_data;
	ak09911_status ret = AK09911_STATUS_SUCCESS;

	u8 buf[9];
	s16 *s16Buff = (s16*)&buf[1];

	ret = ak09911_i2c_read (AK0991X_ST1, buf, 9);
	if (ret != AK09911_STATUS_SUCCESS)
	{
		ak09911_log("[%s] read data error! \n", AKM_AK09911_DEBUG_STR);
		return AK09911_STATUS_FAILED;
	}

	//for continuous mode, we don't care about the data is ready or not.
/*
	if (!(buf[0] & AK09911_ST1_DRDY)) {
		ak09911_log("[%s] data not ready, status=0x%x\n", AKM_AK09911_DEBUG_STR, *st1);
		return SC_STATUS_NO_DRDY;
	}
*/

	/************************************************
	 * 1uT = 10mG, 1uT = 10 * 1000 uG
	 * H(uT) = H * (ASA/128 + 1) * 0.6
	 * H(uG) = H * (ASA/128 + 1) * 10000 * 0.6
	 * H(uG) = H * 6000 * ASA / 128 + H * 6000
	 */
        /*
	data->x = s16Buff[0] * (private_data->asax) * 47 + s16Buff[0] * 6000;//(15/100)*10000 uT ->uG
	data->y = s16Buff[1] * (private_data->asay) * 47 + s16Buff[1] * 6000;
	data->z = s16Buff[2] * (private_data->asaz) * 47 + s16Buff[2] * 6000;  */
        //ak09916's sensitivity is 0.15uT(1.5mG)
	data->x = s16Buff[0]*3/2;
	data->y = s16Buff[1]*3/2;
	data->z = s16Buff[2]*3/2;

	if (buf[8] & AK09911_ST2_HOFL) {
		ak09911_log("[%s] data overflow! \n", AKM_AK09911_DEBUG_STR);
		return AK09911_STATUS_FAILED;
	}

	return ret;
}

/******************************************************************************
 Description:
	configures the sensor in the best way that will support the requested report interval

 Input:
	ctx				- pointer to the sensor's context
	report_interval	- the new report interval to support.
					  when report interval = 0, sensor should shut down

 Return:
	integer value corresponding to success (0) or failure (non-zero value)
 ******************************************************************************/
ak09911_status ak09911_set_report_interval(u16* report_interval)
{
	//extract the private data if you plan to use it in this function
	//intel_motion_magnetometer_3d_akm_ak09911_private_data *private_data = (intel_motion_magnetometer_3d_akm_ak09911_private_data *)udriver_get_private_data(ctx);
	ak09911_status ret = AK09911_STATUS_SUCCESS;

	//Set the best match frequency your sensor supports and return it in the report_interval parameter.
	//This is mandatory for all sensors in order to know the exact frequency the sensor chose and better query sample for algorithms.
	ret = ak09911_power_mode(report_interval);
	if(ret != AK09911_STATUS_SUCCESS){
		ak09911_log("[%s] set power mode failed\n", AKM_AK09911_DEBUG_STR);
        return ret;
	}

	return ret;
}

