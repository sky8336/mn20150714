#include "main.h"
#include "Dsh_ipc.h"

#define TIME_OUT 10000
#define DSH_POOL_TAG 'DSHS'
static int64_t numbers = 0;

pdsh_handle Dsh_init(void)
{
	pdsh_handle pDsh_handle = 
		(pdsh_handle)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(dsh), DSH_POOL_TAG);

	

    if (pDsh_handle) 
    {
        // ZERO it!
        SAFE_FILL_MEM (pDsh_handle, sizeof (dsh), 0);
    }
    else 
    {
        //  No memory
        return NULL;
    }

	return pDsh_handle;
	
}

void transmit_data(char* buf, uint16_t length)
{
	debug("TRANSMIT_DATA");
	print_bytes(buf, length);
	HAL_GPIO_WritePin(AP_INT_GPIO_Port,AP_INT_Pin,GPIO_PIN_RESET);
	//__disable_irq();
	HAL_SPI_Transmit(&hspi2,buf,length,TIME_OUT);
	//__enable_irq();
	HAL_GPIO_WritePin(AP_INT_GPIO_Port,AP_INT_Pin,GPIO_PIN_SET);
}

void transmit_resp(pdsh_handle pDsh_handle)
{
	//send head w/o DMA
	HAL_GPIO_WritePin(AP_INT_GPIO_Port,AP_INT_Pin,GPIO_PIN_RESET);
	debug("TRANSMIT_HEADER");
	print_bytes((char *)&pDsh_handle->head_buff, sizeof(struct frame_head));
	debug("TRANSMIT_RESP");
	print_bytes((char *)pDsh_handle->resp_buff, pDsh_handle->resp_frame_length);

	HAL_GPIO_WritePin(AP_INT_GPIO_Port,AP_INT_Pin,GPIO_PIN_RESET);
	//__disable_irq();
	HAL_SPI_Transmit(&hspi2,(uint8_t *)&pDsh_handle->head_buff, 
		sizeof(struct frame_head) + pDsh_handle->resp_frame_length , TIME_OUT);
	//__enable_irq();
	HAL_GPIO_WritePin(AP_INT_GPIO_Port,AP_INT_Pin,GPIO_PIN_SET);
	
	#if 0
	HAL_SPI_Transmit(&hspi2,(uint8_t *)&pDsh_handle->head_buff, 
		sizeof(struct frame_head), TIME_OUT);
	HAL_GPIO_WritePin(GPIOA,AP_INT_Pin,GPIO_PIN_SET);
	//send resp_ack w/ DMA
	HAL_GPIO_WritePin(GPIOA,AP_INT_Pin,GPIO_PIN_RESET);
	
	HAL_GPIO_WritePin(GPIOA,AP_INT_Pin,GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2,(uint8_t *)&pDsh_handle->resp_buff, 
		pDsh_handle->resp_frame_length, TIME_OUT);
	HAL_GPIO_WritePin(GPIOA,AP_INT_Pin,GPIO_PIN_SET);
	//HAL_SPI_Transmit_DMA(&hspi2,(uint8_t *)&pDsh_handle->resp_buff, 
	//	pDsh_handle->resp_frame_length);
        wTXState = BUFF_FULL;
	#endif
}

void parse_ia_cmd(pdsh_handle pDsh_handle)
{
	//parse cmd packet
	uint8_t i = 0;
	pDsh_handle->status_ = STA_RESP_ACK;
	pDsh_handle->ret_err = E_SUCCESS;
	struct sensor_cfg_param *cfg_para = NULL;
	stream_cfg_t *pcfg_t = NULL;

	debug("GET_IA_CMD");
	print_bytes((char *)&pDsh_handle->ia_buff, sizeof(struct ia_cmd));
	
	switch(pDsh_handle->ia_buff.cmd_id){
	case CMD_CFG_STREAM:
		cfg_para = (struct sensor_cfg_param *)pDsh_handle->ia_buff.param;
		debug("CMD_COFNIG_STREAM for sensor %d", pDsh_handle->ia_buff.sensor_id);
		pcfg_t = &pDsh_handle->stream_sensor[pDsh_handle->ia_buff.sensor_id];
		pcfg_t->isStream = 1;
		pcfg_t->cfg.bit_cfg = cfg_para->bit_cfg;
		pcfg_t->cfg.buff_delay = cfg_para->buff_delay;
		pcfg_t->cfg.sample_freq = cfg_para->sample_freq;
		
		pDsh_handle->streaming_mode = 1;
		for(i = 1; i < 8; i++ ) 
			pDsh_handle->stream_sensor[i].isStream = 1;
		
	break;

	case CMD_STOP_STREAM:
		debug("CMD_STOP_STREAM");
		pDsh_handle->streaming_mode = 0;
		break;
	default:
		pDsh_handle->status_ = STA_IDLE;
	}		
}

void resp_ack(pdsh_handle pDsh_handle)
{
	struct cmd_resp *presp = NULL;

	debug("built ACK package");
	presp = (struct cmd_resp*)pDsh_handle->resp_buff;
	struct resp_cmd_ack *presp_ack = (struct resp_cmd_ack*)presp->buf;
	presp->tran_id = pDsh_handle->ia_buff.tran_id;
	presp->type = RESP_CMD_ACK;
	presp->sensor_id = 0;
	presp->data_len = sizeof(struct resp_cmd_ack);
	presp_ack->cmd_id = pDsh_handle->ia_buff.cmd_id;
	presp_ack->ret = pDsh_handle->ret_err;

	pDsh_handle->resp_frame_length = presp->data_len + RESP_SIZE;
	
	fill_frame_head(&pDsh_handle->head_buff, pDsh_handle->resp_frame_length);
	transmit_resp(pDsh_handle);
	
	pDsh_handle->status_ = STA_PROCESS;
}

void fill_frame_head(void* buf, uint16_t length)
{
	debug("fill FRAME_HEAD");
	
	struct frame_head *pHEAD = (struct frame_head*)buf;
	pHEAD->length = length;
	pHEAD->sign = LBUF_CELL_SIGN;
}

void process_cmd(pdsh_handle pDsh_handle)
{
//prepare data for RESP
	switch(pDsh_handle->ia_buff.cmd_id)
	{
	case CMD_CFG_STREAM:
		pDsh_handle->resp_type = RESP_STREAMING;
		pDsh_handle->status_ = STA_PREPARE_DATA;
		break;
	case CMD_GET_STATUS:
		
		break;
	case CMD_STOP_STREAM:
		pDsh_handle->status_ = STA_IDLE;
		break;
	default:
		pDsh_handle->status_ = STA_IDLE;
	}
}

void prepare_data(pdsh_handle pDsh_handle)
{
	//TODO: add single mode
	int i = 0;
        int counts = 0;
	char* pBuff = pDsh_handle->resp_buff;
	struct cmd_resp *presp_head = NULL;
	struct frame_head *pHead = NULL;
	pSENSOR_DATA raw = NULL;

	numbers++;	
	for(i = 0; i < SENSOR_ID_MAX; i++ ) {
		if(pDsh_handle->stream_sensor[i].isStream == 1){		
			pHead = (struct frame_head*)pBuff;
			pBuff += HEAD_FRAME_SIZE;
			presp_head = (struct cmd_resp*)pBuff;
			pBuff += RESP_SIZE;
			raw = (pSENSOR_DATA)pBuff;
			pBuff += sensor_t_size[i];

			presp_head->sensor_id = i;
			presp_head->tran_id = pDsh_handle->ia_buff.tran_id;
            presp_head->type = RESP_STREAMING;
			presp_head->data_len = sensor_t_size[i];

			switch(presp_head->sensor_id) {
					case SENSOR_ACCELEROMETER:
						debug("prepare ACC data");
						raw->ts = pDsh_handle->accel.ts;
						raw->accel.x= pDsh_handle->accel.x;
						raw->accel.y = pDsh_handle->accel.y;
						raw->accel.z = pDsh_handle->accel.z;

						break;
					case SENSOR_GYRO:
						debug("prepare GYRO data");
						raw->ts = pDsh_handle->gyro.ts;
						raw->gyro.x = pDsh_handle->gyro.x;
						raw->gyro.y = pDsh_handle->gyro.y;
						raw->gyro.z = pDsh_handle->gyro.z;
						break;
					case SENSOR_COMP:
						debug("prepare COMPASS data");
						raw->ts = pDsh_handle->magn.ts;
						raw->compass.x = pDsh_handle->magn.x;
						raw->compass.y = pDsh_handle->magn.y;
						raw->compass.z = pDsh_handle->magn.z;
						break;
					case SENSOR_ACC_RAW:
						debug("prepare ACC_RAW data");
						raw->ts = pDsh_handle->accel_raw.ts;
						raw->accel.x= pDsh_handle->accel_raw.x;
						raw->accel.y = pDsh_handle->accel_raw.y;
						raw->accel.z = pDsh_handle->accel_raw.z;

						break;
					case SENSOR_GYRO_RAW:
						debug("prepare GYRO_RAW data");
						raw->ts = pDsh_handle->gyro_raw.ts;
						raw->gyro.x = pDsh_handle->gyro_raw.x;
						raw->gyro.y = pDsh_handle->gyro_raw.y;
						raw->gyro.z = pDsh_handle->gyro_raw.z;
						break;
					case SENSOR_COMP_RAW:
						debug("prepare COMPASS_RAW data");
						raw->ts = pDsh_handle->magn_raw.ts;
						raw->compass.x = pDsh_handle->magn_raw.x;
						raw->compass.y = pDsh_handle->magn_raw.y;
						raw->compass.z = pDsh_handle->magn_raw.z;
						break;
					case SENSOR_ROT:
						debug("prepare ROTATION matrix");
						raw->ts = numbers;

						for(counts = 0; counts < 9; counts++)
						raw->rot[counts] = pDsh_handle->rot.matrix[counts];

					
    				default:
      					break;
				}
			fill_frame_head(pHead, pBuff-(char*)presp_head);
			}
		}
		debug("prepare data done counts %d", numbers);
		pDsh_handle->resp_frame_length = pBuff - pDsh_handle->resp_buff;
		print_bytes((char *)pDsh_handle->resp_buff, pDsh_handle->resp_frame_length);
		
}
