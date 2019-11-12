/*
 * at_proc.c
 *
 *  Created on: Sep 5, 2019
 *      Author: Teddy
 */

#include "at_proc.h"
#include "string.h"
#include "stdlib.h"
//#include "stm32f1xx_hal_uart.h"

struct queue WizFi_Queue;
struct us_data u2_data;
uint8_t WIFI_SSID[50]="Teddy_AP";
uint8_t WIFI_PW[50]="12345678";
sock_sta sock_status;
char sock_count = 0;
uint8_t sock_Con_Sta[5];
struct Recv_data SPI_Recv_data[5];
extern UART_HandleTypeDef huart1;
void Set_WIFI_DATA(uint8_t *SSID, uint8_t *PW)
{
	memcpy(WIFI_SSID, SSID, strlen((char*)SSID));
	memcpy(WIFI_PW, PW, strlen((char*)PW));
}
#if 1	//temp data test uart2
void Get_WIFI_DATA(uint8_t *SSID, uint8_t *PW)
{
	memcpy(SSID, WIFI_SSID, strlen((char*)WIFI_SSID));
	memcpy(PW, WIFI_PW, strlen((char*)WIFI_PW));
}
void set_Socket_status(sock_sta status)
{
	sock_status = status;
}
sock_sta get_Socket_status(void)
{
	return sock_status;
}
void init_U2_data(void)
{
	memset(&u2_data, 0, sizeof(u2_data));
}
void input_U2_data(uint8_t data)
{
	u2_data.RX_Buffer[u2_data.index++] = data;
	if(data =='\r')
		u2_data.flag = 1;
}
uint8_t U2_flag(void)
{
	uint8_t u2_data_flag = u2_data.flag;
	if(u2_data_flag)
	{
		u2_data.flag = 0;
		u2_data.RX_Buffer[u2_data.index] = 0;
		printf("u2[%d]%s\r\n",u2_data.index, u2_data.RX_Buffer);
		data_Proc(1, 0, u2_data.index, u2_data.RX_Buffer);
		u2_data.index = 0;
	}
	else
	{
		data_Proc(0, 0, 0, 0);
	}
	
	return u2_data_flag;
}
#endif
//queue function
void Init_Queue(void)
{
	memset(&WizFi_Queue, 0, sizeof(WizFi_Queue));
}
uint8_t Queue_Full(void)
{
	if(WizFi_Queue.head + 1 >= MAX_BUFF)
	{
		if(WizFi_Queue.tail == 0)
			return 1;
	}
	else
	{
		if((WizFi_Queue.head + 1) == WizFi_Queue.tail)
			return 1;
	}
	return 0;
}
uint16_t Queue_Empty(void)
{
	if(WizFi_Queue.head >= WizFi_Queue.tail)
		return WizFi_Queue.head - WizFi_Queue.tail;
	else
		return MAX_BUFF - WizFi_Queue.tail + WizFi_Queue.head;
}
void EnQueue(uint8_t input)
{
	if(WizFi_Queue.head + 1 >= MAX_BUFF)
		WizFi_Queue.head = 0;

	if(Queue_Full())
		return;

	WizFi_Queue.data[WizFi_Queue.head++] = input;
}
uint8_t DeQueue(void)
{
	if(WizFi_Queue.tail + 1 >= MAX_BUFF)
		WizFi_Queue.tail = 0;

	if(Queue_Empty() == 0)
		return 0xFF;

	return WizFi_Queue.data[WizFi_Queue.tail++];
}

// connect status data process
int SPI_Input_Data_Proc(uint16_t Len, uint8_t *Data)
{
	static uint8_t sock_num = 0;
	switch(get_Socket_status())
	{
		case CONN_IDLE_MODE:
		if((Len == 11)&&(isdigit(Data[0])))
		{
			if((strncmp(Data + 2, "CONNECT", 7)== 0)&&((Data[0]-'0') < 5))
			{
				sock_count++;
				sock_Con_Sta[Data[0]-'0'] = 1;
				set_Socket_status(CONN_MODE);
			}
		}
		break;
		case CONN_MODE:
		if(strncmp(Data,"+IPD",4))
		{
			//receive mode
			printf("sock %d[%c]", (Data[7] - '0'), Data[7]);
			sock_num = (Data[7] - '0') % 5;
			set_Socket_status(RECV_MODE);
		}
		else if((Len == 11)&&(isdigit(Data[0])))
		{
			//add connection
			if((strncmp(Data + 2, "CONNECT", 7)== 0)&&((Data[0]-'0') < 5))
			{
				sock_count++;
				sock_Con_Sta[Data[0]-'0'] = 1;
			}
		}
		else if((Len == 10)&&(isdigit(Data[0])))
		{
			//del connection
			if((strncmp(Data + 2, "CLOSED", 6)== 0)&&((Data[0]-'0') < 5))
			{
				sock_count--;
				sock_Con_Sta[Data[0]-'0'] = 0;
				if(sock_count <= 0)
				{
					set_Socket_status(CONN_IDLE_MODE);
					return 0;
				}
			}
		}
		else
		{
			/* invalid */
		}
		
		break;
		case RECV_MODE:
		memcpy(SPI_Recv_data[sock_num].data[SPI_Recv_data[sock_num].count++], Data, Len);
		printf("sock[%d],Data[%d]L[%d]:%s\r\n",sock_num, SPI_Recv_data[sock_num].count-1, Len, SPI_Recv_data[sock_num].data[SPI_Recv_data[sock_num].count-1]);
		if(SPI_Recv_data[sock_num].count > 4)
		{
			SPI_Recv_data[sock_num].count = 0;
		}
		set_Socket_status(CONN_MODE);
		break;
		default :
		break;
	}
	return 0;
}

//
uint8_t delay_count(uint16_t *time1, uint16_t *time2, uint16_t set_time)
{
	*time1 += 1;
	if(*time1 > 60000)
	{
		//delay count action
		*time2 += 1;
		*time1 = 0;
		if(*time2 > set_time)
		{
			*time2 = 0;
			return 1;
		}
	}
	return 0;
}
uint8_t match_ok(uint8_t *data, uint16_t len)
{
	uint16_t cnt = 0;
	for(cnt=0; cnt < len; cnt++)
	{
		if((data[cnt]=='O') && (data[cnt+1] == 'K'))
		{
				return 1;
		}
	}
	return 0;
}
uint8_t match_wificon(uint8_t *data, uint16_t len)
{
	uint16_t cnt = 0;
	for(cnt=0; cnt < len; cnt++)
	{
		if((data[cnt]=='W') && (data[cnt+1] == 'I'))
		{
			if(strncmp(data+cnt, "WIFI GOT IP", 11) == 0)
				return 1;
		}
	}
	return 0;
}
uint8_t match_str(uint8_t *data, uint16_t len, uint8_t *M_data)
{
	uint16_t cnt = 0;
	for(cnt=0; cnt < len; cnt++)
	{
		if(data[cnt]==M_data[0])
		{
			if(strncmp(data+cnt, M_data, strlen(M_data)) == 0)
				return 1;
		}
	}
	return 0;
}
int Check_init(void)
{
	uint16_t wait_time = 0, cnt = 0;
	uint8_t temp_buf[100], temp_index = 0;
	//while(--wait_time > 0)
	//send_U_message("AT\r\n", 4);
	while(SPI_RECV_Proc() == 0)
	{
		wait_time++;
		if(wait_time >60000)
		{
			wait_time=0;
			cnt++;
			printf("cnt=%d\r\n", cnt);
			if(cnt>5)
			{
				break;
			}
		}
		SPI_RECV_Proc();
	}
	printf("recv OK\r\n");
	while(Queue_Empty())
	{
		SPI_RECV_Proc();
		temp_buf[temp_index++] = DeQueue();
	}
	temp_buf[temp_index] = 0;
	printf("recv[%d]%s \r\n", temp_index, temp_buf);
	if(match_str(temp_buf, strlen(temp_buf), "ready\r\n"))
	{
		return 1;
	}
	return 0;
}
int AT_CMD_send(uint8_t *cmd, enum cmd_send_type type, uint8_t sock, uint16_t val, uint8_t *S_data)
{
	int len = 0;
	uint8_t cmd_buff[256];
	if(type == none)
	{
		len = sprintf((char*)cmd_buff,"AT+%s=%d\r\n", cmd, val);
	}
	else if(type == noneval)
	{
		len = sprintf((char*)cmd_buff,"AT+%s\r\n", cmd);
	}
	else if(type == CUR_int)
	{
		len = sprintf((char*)cmd_buff,"AT+%s_CUR=%d\r\n", cmd, val);
	}
	else if(type == DEF_int)
	{
		len = sprintf((char*)cmd_buff,"AT+%s_DEF=%d\r\n", cmd, val);
	}
	else if(type == CUR_str)
	{
		len = sprintf((char*)cmd_buff,"AT+%s_CUR=%s\r\n", cmd, S_data);
	}
	else if(type == DEF_str)
	{
		len = sprintf((char*)cmd_buff,"AT+%s_DEF=%s\r\n", cmd, S_data);
	}
	else if(type == none_str)
	{
		len = sprintf((char*)cmd_buff,"AT+%s=%s\r\n", cmd, S_data);
	}
	else
	{
		len = sprintf((char*)cmd_buff,"AT+%s=%d,%d\r\n", cmd, sock, val);
	}
	printf("send[%d]%s\r\n", len, cmd_buff);
	send_U_message(0, cmd_buff, len);
	return len;
}
int AT_CMD_Proc(uint8_t *cmd, enum cmd_send_type type, uint8_t sock, uint16_t val, uint8_t *S_data, uint8_t *re_data, uint16_t time)
{
	static uint8_t req = 0, retry = 0, pre_len = 0;
	static uint16_t Proc_cnt = 0, Proc_cnt1 = 0;
	static uint8_t temp_buf[512];
	static uint16_t temp_index = 0;
	uint16_t data_len = 0;
	if(req == 0)
	{
		AT_CMD_send(cmd, type, sock, val, S_data);
		req = 1;
		Proc_cnt = 0;
		Proc_cnt1 = 0;
		temp_index = 0;
	}
	else if(req == 1)
	{
		SPI_RECV_Proc();
		if((delay_count(&Proc_cnt, &Proc_cnt1, time))|(Queue_Empty() > 0))
			req = 2;
	}
	else
	{
		data_len = Queue_Empty();
		//if((data_len > 0) &&(data_len == pre_len))
		if(data_len > 0)
		{
			while(Queue_Empty())
			{
				temp_buf[temp_index++] = DeQueue();
			}
			temp_buf[temp_index] = 0;
			//printf("CMD recv[%d]%s \r\n", temp_index, temp_buf);
			if(match_str(temp_buf, strlen(temp_buf), re_data))
			{
				req = 0;
				Proc_cnt = 0;
				Proc_cnt1 = 0;
				//printf("send complete[%d]\r\n", req);
				return 1;
			}
			else
			{
				//error action
				req = 1;
				retry++;
				if(retry > 5)
					return -1;
			}
		}
		else
		{
			pre_len = data_len;
		}
		if(data_len == 0)
		{
			req=1;
			retry++;
			if(retry > 5)
				return -1;
		}
	}
	return 0;
}
int Recv_Proc(FuncPtr func, uint16_t time)
{
	static uint16_t Proc_cnt = 0, Proc_cnt1 = 0, pre_len = 0;
	uint8_t temp_buf[512];
	uint16_t temp_index = 0, data_len = 0;

	
	if(delay_count(&Proc_cnt, &Proc_cnt1, time))
	{
		//delay count action
		data_len = Queue_Empty();
		if((data_len > 0) &&(data_len == pre_len))
		{
			while(Queue_Empty())
			{
				temp_buf[temp_index++] = DeQueue();
			}
			temp_buf[temp_index] = 0;
			printf("recv[%d]%s \r\n", temp_index, temp_buf);
			if(func(temp_buf, temp_index))
			{
				Proc_cnt = 0;
				Proc_cnt1 = 0;
				return 1;
			}
			else
			{
				//error action
				pre_len = 0;
			}
		}
		else
		{
			pre_len = data_len;
		}
	}
	return 0;
}
int AT_Connect_Proc(void)
{
	static uint8_t seq = 0;
	switch(seq)
	{
	case 0:		//CWMODE_CUR = 1
		if(AT_CMD_Proc("CWMODE", CUR_int, 0, 1, 0, "OK", 3))
			seq = 2;
		break;
	case 1:		//CWLAP
		if(AT_CMD_Proc("CWLAP", noneval, 0, 0, 0, "OK", 100))
			seq++;
		break;
	case 2:		//CWJAP
		if(AT_CMD_Proc("CWJAP", CUR_str, 0, 0, "\"Teddy_AP\",\"12345678\"", "WIFI GOT IP", 1000))
			seq++;
		break;
	case 3:
		if(AT_CMD_Proc("CIPSTART", none_str, 0, 0, "\"TCP\",\"192.168.0.2\",3000", "CONNECT", 100))
			return 1;
		break;
	}
	return 0;
}
int AT_trans_Proc(void)
{
	static uint8_t seq = 0;
	switch(seq)
	{
	case 0:		//CWMODE_CUR = 1
		if(AT_CMD_Proc("CWMODE", CUR_int, 0, 1, 0, "OK", 3))
			seq = 2;
		break;
	case 1:		//CWLAP
		if(AT_CMD_Proc("CWLAP", noneval, 0, 0, 0, "OK", 100))
			seq++;
		break;
	case 2:		//CWJAP
		if(AT_CMD_Proc("CWJAP", CUR_str, 0, 0, "\"Teddy_AP\",\"12345678\"", "WIFI GOT IP", 1000))
			seq++;
		break;
	case 3:
		if(AT_CMD_Proc("CIPMODE", none, 0, 1, 0, "OK", 3))
		{
			seq++;
		}
		break;
	case 4:
		if(AT_CMD_Proc("CIPSTART", none_str, 0, 0, "\"TCP\",\"192.168.0.2\",3000", "CONNECT", 100))
			seq++;
		break;
	case 5:
		if(AT_CMD_Proc("CIPSEND", noneval, 0, 0, 0, "\r\n>", 3))
		{
			seq = 0;
			return 1;
		}
		break;
	}
	return 0;
}
int AT_Server_Open_Proc(void)
{
	static uint8_t seq = 0;
	extern char AP_CON_DATA[30]; 
	switch(seq)
	{
	case 0:		//CWMODE_CUR = 1
		if(AT_CMD_Proc("CWMODE", CUR_int, 0, 1, 0, "OK", 3))
			seq = 2;
		break;
	case 1:		//CWLAP
		if(AT_CMD_Proc("CWLAP", noneval, 0, 0, 0, "OK", 100))
			seq++;
		break;
	case 2:		//CWJAP
		if(AT_CMD_Proc("CWJAP", CUR_str, 0, 0, AP_CON_DATA, "OK", 1000))
			seq++;
		break;
	case 3:
		if(AT_CMD_Proc("CIPMUX", none, 0, 1, 0, "OK", 10))
			seq++;
		break;
	case 4:
		if(AT_CMD_Proc("CIPSERVER", none_soc, 1, 5001, 0, "OK", 100))
			#if 1
			seq++;
			#else
			seq =0;
			return 1;
			#endif
		break;
	case 5:
		if(AT_CMD_Proc("CIFSR", noneval, 0, 0, 0, "OK", 10))
			seq = 0;
			set_Socket_status(CONN_IDLE_MODE);
			return 1;
		break;
	}
	return 0;
}
int AT_SEND_Proc(uint8_t *data, uint16_t len)
{
	static uint8_t send_seq = 0, recv_retry = 0;
	static uint16_t wait_time = 0;
	uint8_t temp_buf[512], temp_index = 0;
	uint16_t data_len = len, data_shift = 0;
	switch(send_seq)
	{
	case 0:
		//printf("send start\r\n");
		send_seq++;
		break;
	case 1:
		if(AT_CMD_Proc("CIPSEND", none, 0, len, 0, "> ", 3) == 1)
		{
			send_seq++;
			recv_retry = 0;
			wait_time = 0;
		}
		break;
	case 2:
		//send_U_message(0, data, len);
		//printf("message send \r\n");
		while (data_len > 0)
		{
			if(data_len > 500)
			{
				send_U_message(1, data + data_shift, 500);
				data_shift += 500;
				data_len = data_len - 500;
			}
			else
			{
				send_U_message(1, data + data_shift, data_len);
				data_len = 0;
			}
		}

		
		send_seq++;
		break;
	case 3:
		SPI_RECV_Proc();
		#if 0
		wait_time++;
		if(wait_time > 20000)
		{
			printf("send receive fail \r\n");
			//send_seq = 0;
			spi_init();
			//return -1;
		}
		#endif
		if(Queue_Empty()>0)
			send_seq++;
		break;
	case 4:
		while(Queue_Empty())
		{
			temp_buf[temp_index++] = DeQueue();
		}
		temp_buf[temp_index] = 0;
		//printf("recv[%d]%s \r\n", temp_index, temp_buf);
		if(match_str(temp_buf, strlen(temp_buf), "SEND OK"))
		{
			send_seq = 0;
			return 1;
		}
		send_seq--;
		recv_retry++;
		if(recv_retry > 5)
		{
			return -1;
		}
		break;
	}
	return 0;
}
int AT_SEND_Proc1(uint8_t *data, uint16_t len)
{
	uint16_t data_len = len, data_shift = 0;
	while (data_len > 0)
	{
		if(data_len > 500)
		{
			send_U_message(1, data + data_shift, 500);
			data_shift += 500;
			data_len = data_len - 500;
		}
		else
		{
			send_U_message(1, data + data_shift, data_len);
			data_len = 0;
		}
	}
	return 0;
}
int AT_AirKiss_Proc(void)
{
	static uint8_t seq = 0;
	switch(seq)
	{
	case 0:		//CWMODE_CUR = 1
		if(AT_CMD_Proc("CWMODE", CUR_int, 0, 1, 0, "OK", 10))
			seq++;
		break;
	case 1:		//CWLAP
		if(AT_CMD_Proc("CWSTARTSMART", none, 0, 2, 0, "OK", 100))
			seq++;
		break;
	case 2:		//CWJAP
		if(Recv_Proc(AirKissConnect, 3))
		{
			printf("AirKiss Success !!\r\n");
			seq++;
		}
		break;
	case 3:
		if(AT_CMD_Proc("CIFSR", noneval, 0, 0, 0, "OK", 10))
			seq++;
		break;
	case 4:
		if(AT_CMD_Proc("CIPMUX", none, 0, 1, 0, "OK", 10))
			seq++;
		break;
	case 5:
		if(AT_CMD_Proc("CIPSERVER", none_soc, 1, 5001, 0, "OK", 100))
			seq++;
		break;
	case 6:
	#if 0
		if(Recv_Proc(RecvDataPars, 3))
		{
			printf("data recieve !!\r\n");
			seq++;
		}
		#endif
		return 1;
		break;
	}
	return 0;
}
int data_Proc(uint8_t mode, uint8_t sock, uint16_t val, uint8_t *S_data)
{
	uint8_t status = 0;
	if(mode)
	{
		//send mode
#if 0
		if(Queue_Empty() > 0)
			return 0;
#endif
		while(status == 0)
		{
			status = AT_CMD_Proc("CIPSEND", none_soc, sock, val, 0, "OK", 10);
		}
		if(status == 1)
		{
			send_U_message(1, S_data, val);
			S_data[val] = 0;
			printf("send data[%d]%s\r\n",val, S_data);
			return 1;
		}
		return 0;
	}
	else
	{
		//recv mode
		if(Recv_Proc(RecvDataPars, 3))
		{
			//printf("data recieve !!\r\n");
			return 1;
		}
		return 0;
	}
	return 1;
}

int AirKissConnect(uint8_t *data, uint16_t len)
{
	uint16_t cnt = 0;
	for(cnt=0; cnt < len; cnt++)
	{
		if((data[cnt]=='c') && (data[cnt+1] == 'o'))
		{
			if(memcmp(&data[cnt], "connected wifi", strlen("connected wifi")) == 0)
			{
				//data[len] = 0;
				//printf("Airkiss recv[%d][%s]\r\n",sizeof("connected wifi"), &data[cnt]);
				return 1;
			}
				
		}
	}
	return 0;
}

int RecvDataPars(uint8_t *data, uint16_t len)
{
	uint16_t cnt = 0;
	char *ptr;
	int data1, data2;
	for(cnt=0; cnt < len; cnt++)
	{
		if(memcmp(&data[cnt], "+IPD,", strlen("+IPD,")) == 0)
		{
			cnt = cnt+strlen("+IPD,");
			data[len] = 0;
			ptr = strtok(&data[cnt],",");
			if(ptr == NULL)
			{
				//single
				ptr = strtok((char *)&data[cnt],":");
				data1 = atoi(ptr);
				printf("LEN:%d, str:%s \r\n", data1, ptr);
				return 1;
			}
			else
			{
				//multi 
				data2 = atoi(ptr);
				ptr = strtok(NULL,":");
				data1 = atoi(ptr);
				ptr = strtok(NULL,":");

				printf("socket: %d LEN:%d data:%s \r\n", data2, data1, &data[len - data1]);
				return 1;
			}
			
		}
	}
	return 0;
}
uint8_t U2_test(void)
{
	uint8_t u2_data_flag = u2_data.flag;
	if(u2_data_flag)
	{
		u2_data.flag = 0;
		u2_data.RX_Buffer[u2_data.index] = 0;
		printf("u2[%d]%s\r\n",u2_data.index, u2_data.RX_Buffer);
		// if(strncmp(u2_data.RX_Buffer, "change", 5) == 0)
		// {
		// 	USART1_UART_change();
		// }
		send_U_message(0, u2_data.RX_Buffer, u2_data.index);
		u2_data.index = 0;
	}
	else
	{
		data_Proc(0, 0, 0, 0);
	}
	
	return u2_data_flag;
}
void USART1_UART_change(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

