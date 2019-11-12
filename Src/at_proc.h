/*
 * at_proc.h
 *
 *  Created on: Sep 5, 2019
 *      Author: Teddy
 */

#ifndef AT_PROC_H_
#define AT_PROC_H_
#include "main.h"

#define CWMODE 			"CWMODE"
#define CWLAP			"CWLAP"
#define CWJAP			"CWJAP"
#define RESP_OK			"OK\r\n"
#define MAX_BUFF		1024
typedef struct us_data
{
	char RX_Buffer[1024];
	uint8_t flag;
	uint16_t index;

}us_data_t;
typedef enum cmd_send_type
{
	none,
	noneval,
	CUR_int,
	DEF_int,
	CUR_str,
	DEF_str,
	none_str,
	none_soc
}cmd_send_type_t;

typedef struct queue
{
	uint16_t head;
	uint16_t tail;
	uint8_t data[MAX_BUFF];
}queue_t;

typedef struct AP_data
{
	char SSID[15];
	char PW[15];
}AP_data_t;

typedef struct Con_proco
{
	uint8_t proco;
	uint8_t ip[4];
	uint16_t port;
}Con_proco_t;

typedef enum sock_sta_t
{
	IDLE_MODE,
	CMD_MODE,
	DATA_MODE,
	CONN_IDLE_MODE,
	CONN_MODE,
	RECV_MODE
}sock_sta;

typedef struct Recv_data
{
	uint8_t count;
	uint8_t data[5][50];
}Recv_data_t;

typedef (*FuncPtr)(int, int);

//queue function
void Init_Queue(void);
uint8_t Queue_Full(void);
uint16_t Queue_Empty(void);
void EnQueue(uint8_t input);
uint8_t DeQueue(void);
//send uart data
void send_U_message(uint8_t type, uint8_t *data, uint16_t len);
int SPI_RECV_Proc(void);

/////////////////////////
void set_Socket_status(sock_sta status);
sock_sta get_Socket_status(void);
////
int AT_trans_Proc(void);
int AT_Connect_Proc(void);
int AT_AirKiss_Proc(void);
int AT_Server_Open_Proc(void);
int data_Proc(uint8_t mode, uint8_t sock, uint16_t val, uint8_t *S_data);
int AT_CMD_Proc(uint8_t *cmd, enum cmd_send_type type, uint8_t sock, uint16_t val, uint8_t *S_data, uint8_t *re_data, uint16_t time);
int AT_CMD_send(uint8_t *cmd, enum cmd_send_type type, uint8_t sock, uint16_t val, uint8_t *S_data);
int Recv_Proc(FuncPtr func, uint16_t time);
void Set_WIFI_DATA(uint8_t *SSID, uint8_t *PW);
void Get_WIFI_DATA(uint8_t *SSID, uint8_t *PW);
uint8_t delay_count(uint16_t *time1, uint16_t *time2, uint16_t set_time);
uint8_t match_ok(uint8_t *data, uint16_t len);
int AirKissConnect(uint8_t *data, uint16_t len);
int RecvDataPars(uint8_t *data, uint16_t len);
int AT_SEND_Proc(uint8_t *data, uint16_t len);
int AT_SEND_Proc1(uint8_t *data, uint16_t len);
int Check_init(void);
void spi_init(void);

#if 1	//temp data test uart2
void init_U2_data(void);
void input_U2_data(uint8_t data);
uint8_t U2_flag(void);
#endif

#if 1 //test
uint8_t U2_test(void);
void USART1_UART_change(void);
#endif
#endif /* AT_PROC_H_ */
