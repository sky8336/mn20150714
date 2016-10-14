/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 *
 * COPYRIGHT(c) 2016 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include <string.h> /* strlen */
#include <stdio.h>  /* sprintf */
#include <math.h>   /* trunc */
#include "main.h"

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"


/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

bma2x2_t bmi055DevAcc;      // structure holding ic2/spi interface function pointers (acc part)
bmg160_t bmi055DevGyro;     // structure holding ic2/spi interface function pointers (gyro part)
static unsigned long current_ts = 0;
static unsigned long pre_tsa = 0;
static unsigned long pre_tsg = 0;
static unsigned long pre_tsm = 0;
static void fusion();
/* USER CODE END PV */

static pIvhAlgo Algo_Handle = NULL;
static pdsh_handle pDsh_handle = NULL;

//sensor data
static bma2x2_xyz_t accel;
static bmg160_xyz_t gyro;
static ak09911_xyz_t magn;
static int rot[9];
static short unsigned mag_odr;

ak09911_status ak_sta;
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void Bmi055_init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static void HandleAlgoNotifation(ULONG SensorType, void* data)
{
	return;
}

static void fusion()
{
	current_ts = HAL_GetTick();
	if((current_ts - pre_tsa) > ONE_KHZ_TIME){
		bma2x2_read_accel_xyz(&accel);
		accel.x = accel.x*98/100;
		accel.y = accel.y*98/100;
		accel.z = accel.z*98/100;

		pDsh_handle->accel_raw.x = accel.x;
		pDsh_handle->accel_raw.y = accel.y;
		pDsh_handle->accel_raw.z = accel.z;
		pDsh_handle->accel_raw.ts = current_ts;

		Alg_UpdateAccelerometerData(Algo_Handle,
				accel.x,accel.y,accel.z,current_ts);
		Alg_GetAccelerometer(Algo_Handle,&pDsh_handle->accel);
		pre_tsa = current_ts;
	}

	if((current_ts - pre_tsg) > ONE_KHZ_TIME){
		bmg160_get_data_XYZ(&gyro);
		gyro.x *= 61;
		gyro.y *= 61;
		gyro.z *= 61;

		pDsh_handle->gyro_raw.x = gyro.x;
		pDsh_handle->gyro_raw.y = gyro.y;
		pDsh_handle->gyro_raw.z = gyro.z;
		pDsh_handle->gyro_raw.ts = current_ts;
		Alg_UpdateGyrometerData(Algo_Handle,
				gyro.x,gyro.y,gyro.z,current_ts);
		Alg_GetGyrometer(Algo_Handle,&pDsh_handle->gyro);
		Alg_GetOrientationMatrix(Algo_Handle, &pDsh_handle->rot);
		pre_tsg = current_ts;
	}

	if((current_ts - pre_tsm) > ONE_HUN_HZ_TIME){
		ak09911_read_data(&magn);

		pDsh_handle->magn_raw.x = magn.x;
		pDsh_handle->magn_raw.y = magn.y;
		pDsh_handle->magn_raw.z = magn.z;
		pDsh_handle->magn_raw.ts = current_ts;

		Alg_UpdateMagnetometerData(Algo_Handle,
				magn.x,magn.y,magn.z,current_ts);
		Alg_GetMagnetometer(Algo_Handle,&pDsh_handle->magn);

		pre_tsm = current_ts;
		//Alg_GetEuler(Algo_Handle, pDsh_handle->rot);
	}
	/*
	   if(++counts&&0x3ff == 0){
	   sprintf( dataOut, "[%5d,%5d,%5d], ts =%d \n", 
	   pDsh_handle->rot[0],pDsh_handle->rot[1],pDsh_handle->rot[2], current_ts);
//sprintf( dataOut, "[%5d,%5d,%5d],[%5d,%5d,%5d],[%5d,%5d,%5d], ts =%
d \n", rot[0],rot[1],rot[2],rot[3],rot[4],rot[5],rot[6],rot[7],rot[8],ts);
HAL_UART_Transmit( &huart2, ( uint8_t * )dataOut, strlen( dataOut ), 
5000 );
}
*/
}

void print_bytes(char* ptr, int len)
{

	int i = 0;
	for(i = 0; i<len; i++)
		printf("%02x ", ptr[i]);
	printf("\n\r");
}
/* USER CODE END 0 */



int main(void)
{

	/* USER CODE BEGIN 1 */
	HAL_StatusTypeDef read_status = HAL_OK;
	unsigned long sensor_input = IVH_PHYSICAL_SENSOR_ACCELEROMETER3D|IVH_PHYSICAL_SENSOR_GYROSCOPE3D|IVH_PHYSICAL_SENSOR_MAGNETOMETER3D;
	unsigned long sensor_output = 0;
	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_I2C3_Init();
	MX_SPI2_Init();
	MX_SPI3_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USB_OTG_FS_PCD_Init();

	/* USER CODE BEGIN 2 */
	debug("Begin Sensor Hub");
	Bmi055_init();

	ak09911_init();
	mag_odr = 10;
	ak09911_set_report_interval(&mag_odr);
	Algo_Handle = Alg_Init(HandleAlgoNotifation, sensor_input, &sensor_output);
	pDsh_handle = Dsh_init();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		//polling sensor data
		fusion();

		switch(pDsh_handle->status_){
		case STA_IDLE:
			//debug("STA_IDLE");
			read_status = HAL_SPI_Receive(&hspi2,(uint8_t *)&pDsh_handle->ia_buff, 
					sizeof(struct ia_cmd),1);
			switch(read_status) {
			case HAL_OK:
				pDsh_handle->status_ = STA_PARSE_CMD;
				break;
			case HAL_TIMEOUT:
				pDsh_handle->status_ = STA_WAIT_CMD;
				break;
			default:
				pDsh_handle->status_ = STA_WAIT_CMD;
				break;				
			}
			break;
		case STA_WAIT_CMD:
			//waiting for cmd except streaming mode
			if(pDsh_handle->streaming_mode == 1)
				pDsh_handle->status_ = STA_PREPARE_DATA;
			else
				pDsh_handle->status_ = STA_IDLE;
			break;
		case STA_PARSE_CMD:
			debug("parse_cmd");
			parse_ia_cmd(pDsh_handle);
			break;
		case STA_RESP_ACK:
			debug("resp_ack");
			resp_ack(pDsh_handle);
			break;
		case STA_PROCESS:
			debug("process");
			//two flow of next state
			process_cmd(pDsh_handle);
			break;
		case STA_PREPARE_DATA:
			debug("prepare_data");
			//if(wTXState == BUFF_EMPTY){
			prepare_data(pDsh_handle);
			pDsh_handle->status_ = STA_COMMIT_DATA;
			//	pDsh_handle->status_ = STA_COMMIT_DATA;
			//	wTXState = BUFF_FULL;
			//	}
			//else
			//	pDsh_handle->status_ = STA_IDLE;
			break;
		case STA_COMMIT_DATA:
			//HAL_Delay(500);
			debug("commit_data");
			transmit_data(pDsh_handle->resp_buff,pDsh_handle->resp_frame_length);
			pDsh_handle->status_ = STA_IDLE;
			break;
		}
		//HAL_Delay(500);

	}

	mag_odr = 0;
	ak09911_set_report_interval(&mag_odr); //power down
	/* USER CODE END 3 */


}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 96;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
		|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void Bmi055_init(void)
{
	/************************* Initialization section ******************************/
	bmi055DevAcc.bus_read = bstdr_burst_read;	// assign bus read function for bst devices
	bmi055DevAcc.bus_write = bstdr_burst_write;	// assign bus write function for bst devices
	bmi055DevAcc.delay = bstdr_ms_delay;	// assign delay function
	bmi055DevAcc.dev_addr = 0x1A;       // set acc dev addr as 0x1A for SPI chip select

	bmi055DevGyro.bus_read = bstdr_burst_read;	// assign bus read function for bst devices
	bmi055DevGyro.bus_write = bstdr_burst_write;	// assign bus write function for bst devices
	bmi055DevGyro.delay = bstdr_ms_delay;	// assign delay function
	bmi055DevGyro.dev_addr = 0x1B;      // set gyro dev addr as 0x1B for SPI chip select

	bma2x2_init(&bmi055DevAcc);     // initialize the device: acceleration sensor part
	bmg160_init(&bmi055DevGyro);    // initialize the device: gyro part

	/************************* Configuration section: accel ******************************/
	bma2x2_set_power_mode(BMA2x2_MODE_NORMAL);
	bma2x2_set_bw(BMA2x2_BW_500HZ);
	bma2x2_set_range(BMA2x2_RANGE_2G);

	bmi055DevAcc.delay(10);         // Wait for device to settle down with new config

	/************************* Configuration section: gyro ******************************/
	bmg160_set_power_mode(BMG160_MODE_NORMAL);
	bmi055DevGyro.delay(50);
	bmg160_set_bw(BMG160_BW_230_HZ);
	bmg160_set_range(BMG160_RANGE_2000);

	bmi055DevGyro.delay(50);        // Wait for device to settle down with new config
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */

}

#endif

/**
 * @}
 */ 

/**
 * @}
 */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
