/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "ctype.h"
#include "at_proc.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define SPI_RX_DESC_NUM			10

#define SPI_REG_TIMEOUT			200
#define SPI_TIMEOUT				300		// 200mS

#define SPI_REG_INT_STTS		0x06
#define SPI_REG_RX_DAT_LEN		0x02
#define SPI_REG_TX_BUFF_AVAIL	0x03
#define SPI_CMD_RX_DATA			0x10
#define SPI_CMD_TX_CMD			0x91
#define SPI_CMD_TX_DATA			0x90
#define SPI_CS_OFF				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define SPI_CS_ON				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)
#define SPI_INT_STTS			HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)


#define debug 					0
#define debug1 					0
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t RX_BUFFER[512];
uint8_t SPI_TX_BUFF[1048];
uint8_t SPI_RX_BUFF[1048];
uint8_t RX_Flag = 0;
uint16_t RX_Index= 0;
uint8_t Spi_rx_flag = 0;
//user change AP  data and Server port
char AP_CON_DATA[30] = "\"Teddy_AP\",\"12345678\"";
uint16_t Server_Port = 5001;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
uint8_t rxData;

int _write(int fd, char *str, int len)
{
	for(int i=0; i<len; i++)
	{
		HAL_UART_Transmit(&huart2, (uint8_t *)&str[i], 1, 0xFFFF);
	}
	return len;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

    /*
        This will be called once data is received successfully,
        via interrupts.
    */

     /*
       loop back received data
     */
     HAL_UART_Receive_IT(&huart2, &rxData, 1);
     //HAL_UART_Transmit(&huart2, &rxData, 1, 1000);
     RX_BUFFER[RX_Index++] = rxData;
     if(rxData == '\n')
     {
    	 RX_Flag = 1;
     }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_8)
	{
		Spi_rx_flag = 1;
    //SPI_RECV_Proc();
	}
}
void send_U_message(uint8_t type, uint8_t *data, uint16_t len)
{
  int i;
  //HAL_UART_Transmit(&huart1, data, len, 0xFFFF);
  //read TX_BUFF_AVAIL
  uint8_t temp_CMD, retry = 0, err = 0, dum = 0xFF, dum2=0x00, rx_temp[2];
  uint16_t spi_rx_len = 0, TX_len;
  memset(SPI_TX_BUFF,0, sizeof(SPI_TX_BUFF));
  SPI_CS_OFF;
  temp_CMD = SPI_REG_TX_BUFF_AVAIL;
  
  while((spi_rx_len != 0xffff) && (0 == (spi_rx_len & 0x02)))
  {
    retry++;
    HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[0], 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[1], 1, 10);
    spi_rx_len = rx_temp[0] | (rx_temp[1] << 8);
    
    if(retry > SPI_TIMEOUT)
    {
      printf("SPI CMD timeout\r\n");
      retry = 0;
      err = 1;
      break;
    }
  }
  SPI_CS_ON;
  TX_len = len + 2;
  if(TX_len % 4)
  {
    TX_len = ((TX_len + 3)/4) << 2;
    //TX_len = (TX_len + 3) & 0xFFFC;
  }
  //printf("SPI_REG_TX_BUFF_AVAIL[%d]\r\n", spi_rx_len);
#if debug
  printf("SPI_REG_TX_BUFF_AVAIL[%d]\r\n", spi_rx_len);
  printf("TX Hex1 : ");
  for(i=0; i<RX_Index; i++)
  {
    printf("[%d]%0.2X ",i, RX_BUFFER[i]);
  }
  printf("\r\n");
#endif
  //printf("trans[%d][%d] : %s\r\n", len, TX_len, data);
  //printf("trans[%d][%d]\r\n", len, TX_len);
  if(err==0)
  {
    SPI_CS_OFF;
    if(type)
    {
      temp_CMD = SPI_CMD_TX_DATA;
    }
    else
    {
      temp_CMD = SPI_CMD_TX_CMD;
    }
    HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
    memcpy(SPI_TX_BUFF , &len, sizeof(len));
    memcpy(SPI_TX_BUFF + 2, data, len);
    HAL_SPI_TransmitReceive(&hspi1, SPI_TX_BUFF, RX_BUFFER, TX_len, 10);

    SPI_CS_ON;
    //printf("%d [%0.2X %0.2X]%s \r\n",TX_len, SPI_TX_BUFF[0], SPI_TX_BUFF[1], SPI_TX_BUFF + 2);
#if debug
    printf("TX Hex2 : ");
    for(i=0; i<TX_len; i++)
    {
      printf("[%d]%0.2X ",i, SPI_TX_BUFF[i]);
    }
    printf("\r\n");
#endif
  }
}
int SPI_RECV_Proc(void)
{
  uint8_t temp_CMD, dum = 0xFF, dum2=0x00,rx_temp[2]={0,0};
  uint16_t spi_rx_len = 0;
  uint16_t i;
  
#if debug1
  printf("SPI Interrupt input\r\n");
#endif
  SPI_CS_OFF;
  temp_CMD = SPI_REG_INT_STTS;
  HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
  HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[0], 1, 10);
  HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[1], 1, 10);
  spi_rx_len = rx_temp[0] | (rx_temp[1] << 8);
  SPI_CS_ON;
#if 0
  printf("SPI_REG_INT_STTS[%d]\r\n", spi_rx_len);
#endif
  if((spi_rx_len != 0xffff) && (spi_rx_len & 0x01))
  {
    SPI_CS_OFF;
    temp_CMD = SPI_REG_RX_DAT_LEN;
    HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[0], 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[1], 1, 10);
    spi_rx_len = rx_temp[0] | (rx_temp[1] << 8);
    SPI_CS_ON;
  }
#if debug1
    printf("SPI_REG_RX_DAT_LEN[%d]\r\n", spi_rx_len);
#endif
  if(spi_rx_len > 0)
  {
    SPI_CS_OFF;
    temp_CMD = SPI_CMD_RX_DATA;
    HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, SPI_RX_BUFF, spi_rx_len, 10);
    SPI_RX_BUFF[spi_rx_len] = 0;
    #if 0
    for(i=0; i<spi_rx_len; i++)
    {
      EnQueue(SPI_RX_BUFF[i]);
    }
    #endif
    SPI_CS_ON;
    #if 1 //teddy 191111
    if(get_Socket_status() == 0)
    {
      for(i=0; i<spi_rx_len; i++)
      {
        EnQueue(SPI_RX_BUFF[i]);
      }
    }
    else
    {
      SPI_Input_Data_Proc(spi_rx_len, SPI_RX_BUFF);
    }
    
    #else
    for(i=0; i<spi_rx_len; i++)
    {
      EnQueue(SPI_RX_BUFF[i]);
    }
    #endif
#if debug1
    printf("RX Hex : ");
    for(i=0; i<spi_rx_len; i++)
    {
      printf("[%d]%0.2X ",i, SPI_RX_BUFF[i]);
    }
    printf("\r\n");
#endif
    printf("RX[%d]:[%s]\r\n", spi_rx_len, SPI_RX_BUFF);
    //printf("RX[%d]\r\n", spi_rx_len);
    return 1;
  }
  return 0;
}
int SPI_RECV_Proc1(void)
{
  uint8_t temp_CMD, dum = 0xFF, dum2=0x00,rx_temp[2];
  uint16_t spi_rx_len = 0;
  uint16_t i;
  
#if debug1
  printf("SPI Interrupt input\r\n");
#endif
  SPI_CS_OFF;
  temp_CMD = SPI_REG_INT_STTS;
  HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
  HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[0], 1, 10);
  HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[1], 1, 10);
  spi_rx_len = rx_temp[0] | (rx_temp[1] << 8);
  SPI_CS_ON;
#if debug1
  printf("SPI_REG_INT_STTS[%d]\r\n", spi_rx_len);
#endif
  if((spi_rx_len != 0xffff) && (spi_rx_len & 0x01))
  {
    SPI_CS_OFF;
    temp_CMD = SPI_REG_RX_DAT_LEN;
    HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[0], 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, &rx_temp[1], 1, 10);
    spi_rx_len = rx_temp[0] | (rx_temp[1] << 8);
    SPI_CS_ON;
  }
#if debug1
    printf("SPI_REG_RX_DAT_LEN[%d]\r\n", spi_rx_len);
#endif
  if(spi_rx_len > 0)
  {
    SPI_CS_OFF;
    temp_CMD = SPI_CMD_RX_DATA;
    HAL_SPI_TransmitReceive(&hspi1, &temp_CMD, &dum2, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, &dum, SPI_RX_BUFF, spi_rx_len, 10);
    SPI_RX_BUFF[spi_rx_len+1] = 0;
    #if 1
    for(i=0; i<spi_rx_len; i++)
    {
      EnQueue(SPI_RX_BUFF[i]);
    }
    SPI_CS_ON;
    #endif
#if debug1
    printf("RX Hex : ");
    for(i=0; i<spi_rx_len; i++)
    {
      printf("[%d]%0.2X ",i, SPI_RX_BUFF[i]);
    }
    printf("\r\n");
#endif
    printf("RX data[%d]:%s \r\n", spi_rx_len, SPI_RX_BUFF);
    //send_U_message(1,SPI_RX_BUFF,spi_rx_len);
    AT_SEND_Proc1(SPI_RX_BUFF, spi_rx_len);
    //printf("RX data[%d]\r\n", spi_rx_len);
    return 1;
  }
  return 0;
}
int check_US_cmd(int status)
{
  int cnt;
  for(cnt=0; cnt<RX_Index; cnt++)
  {
    if(isalpha(RX_BUFFER[cnt]))
    {
      if(strncmp(RX_BUFFER + cnt, "CONNECT", 6) == 0)
      {
        return 1;
      }
      else if(strncmp(RX_BUFFER + cnt, "TRANS CONNECT", 12) == 0)
      {
        return 2;
      }
      else if(strncmp(RX_BUFFER + cnt, "TRANS LOOFBACK", 12) == 0)
      {
        //printf("START Trans Mode loof Back\r\n");
        printf("not supported!!\r\n");
        //return 5;
      }
      else if(strncmp(RX_BUFFER + cnt, "SEND TRANS DATA", 15) == 0)
      {
        return 4;
      }
      else if(strncmp(RX_BUFFER + cnt, "SEND DATA", 9) == 0)
      {
        return 3;
      }
      else if(strncmp(RX_BUFFER + cnt, "Server open", 11) == 0)
      {
        return 6;
      }
      else if(strncmp(RX_BUFFER + cnt, "exit", 4) == 0)
      {
        printf("exit\r\n");
        return 0;
      }
    }
  }
  return status;
}
void main_proc(int *main_seq)
{
  static uint16_t cnt = 0;
  int res = 0;
  uint8_t TEST_DATA[2001]="012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
  switch(*main_seq)
  {
    case 1:
      if(AT_Connect_Proc() == 1)
      {
        //complete
        printf("Connect Success !!\r\n");
        *main_seq = 0;
      }
      cnt = 0;
    break;
    case 2:
      if(AT_trans_Proc() == 1)
      {
        //complete
        printf("Connect Success !!\r\n");
        *main_seq = 0;
      }
      cnt = 0 ;
    break;
    case 3:
    #if 1 //once
      if(cnt<1048)
      {
        res = AT_SEND_Proc(TEST_DATA, 1000);
        if(res< 0)
        {
        printf("SPI SEND Fail[%d]\r\n", cnt);
        cnt--;
        }
        else if(res == 1)
        {
        //printf("SPI SEND OK[%d]\r\n", cnt);
        cnt++;
        }
      }
      else
      {
        res = AT_SEND_Proc(TEST_DATA, 75);
        if(res < 0)
        {
          printf("SPI SEND Fail[%d]\r\n", cnt);
          cnt--;
        }
        else if(res == 1)
        {
        //printf("SPI SEND OK[%d]\r\n", cnt);
        printf("SPI SEND COMPLETE[%d]\r\n", cnt);
        cnt++;
        *main_seq =0;
        }
      }    
    #endif
    break;
    case 4:
    if(cnt<2097)
    {
      send_U_message(1, TEST_DATA, 500);
      cnt++;
    }
    else
    {
      send_U_message(1, TEST_DATA, 75);      
      cnt++;

		  printf("SPI SEND OK[%d]\r\n", cnt);
		  *main_seq =0;
      cnt = 0;
    }
    #if 0
    if(cnt<1048)
    //if(cnt<524)
    {
      res = AT_SEND_Proc1(TEST_DATA, 1000);
      if(res< 0)
      {
        printf("SPI SEND Fail[%d]\r\n", cnt);
        cnt--;
      }
      else if(res == 1)
      {
        //printf("SPI SEND OK[%d]\r\n", cnt);
        cnt++;
      }
    }
    else
    {
      res = AT_SEND_Proc1(TEST_DATA, 75);
    	if(res < 0)
    	{
			printf("SPI SEND Fail[%d]\r\n", cnt);
			cnt--;
      }
      else if(res == 1)
      {
        printf("SPI SEND OK[%d]\r\n", cnt);
        printf("SPI SEND COMPLETE[%d]\r\n", cnt);
        cnt++;
        *main_seq =0;
        cnt = 0;
      }
    }
    #endif
    break;
    case 5:
    break;
    case 6: 
    if(AT_Server_Open_Proc())
    {
      printf("Connect and Server Open Success !!\r\n");
      *main_seq = 0;
    }
    break;
    default :
    break;
  }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  int main_seq = 0;
  int temp_set_seq = 1;
  unsigned int temp_delay = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, &rxData, 1);
  printf("Hello !!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  //Check_init();
  printf("WizFi360 Ready !!\r\n");
  while(temp_delay++<60000);
  temp_delay = 0;
  while (1)
  {
    if(RX_Flag)
    {
    	main_seq = check_US_cmd(main_seq);
    	RX_Index = 0;
    	RX_Flag  = 0;
		  //memset(RX_BUFFER, 0, sizeof(RX_BUFFER));
    }
    #if 0
    if(main_seq == 5)
	  {
		  SPI_RECV_Proc1();
      Spi_rx_flag = 0;
	  }
	  else
    {
      if(Spi_rx_flag||SPI_INT_STTS)
      {

        SPI_RECV_Proc();
        Spi_rx_flag = 0;
      }
    }
    #else
    
    if(Spi_rx_flag||(SPI_INT_STTS == 0))
    {
      if(main_seq == 5)
      {
        SPI_RECV_Proc1();
        Spi_rx_flag = 0;
      }
      else
      SPI_RECV_Proc();

      Spi_rx_flag = 0;
    }
    
    #endif
    main_proc(&main_seq);
    if(temp_set_seq)
    {
#if 0
    	temp_delay++;
    	if(temp_delay > 30000)
    	{
    		temp_delay=0;
			  temp_set_seq = 0;
			  main_seq = 6;
    	}
#endif
    	if(Queue_Empty()>0)
    	{
    		temp_set_seq = 0;
		  main_seq = 6;
    	}
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */
void spi_init(void)
{
  MX_SPI1_Init();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
