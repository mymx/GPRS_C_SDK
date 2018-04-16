#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

#include "api_os.h"
#include "api_debug.h"
#include "api_event.h"
#include <api_socket.h>
#include "api_network.h"
#include "api_hal_uart.h"




#define MAIN_TASK_STACK_SIZE    (2048 * 2)
#define MAIN_TASK_PRIORITY      0
#define MAIN_TASK_NAME          "Main Test Task"

#define SECOND_TASK_STACK_SIZE    (2048 * 2)
#define SECOND_TASK_PRIORITY      1
#define SECOND_TASK_NAME          "Second Test Task"

#define SERVER_IP   "nan5508.nat123.net"
#define SERVER_PORT 49616

#define DNS_DOMAIN  "nan5508.nat123.net"
#define RECEIVE_BUFFER_MAX_LENGTH 200

int socketFd = -1;
uint8_t buffer1[RECEIVE_BUFFER_MAX_LENGTH];
int socketFd2 = -1;
int receivedDataCount = -1;

static HANDLE mainTaskHandle = NULL;
static HANDLE secondTaskHandle = NULL;
static bool flag = false;


uint8_t rxbuffer[100];
void OnUart1ReceivedData(UART_Callback_Param_t param)
{
    memset(rxbuffer,0,sizeof(rxbuffer));
    uint32_t len = UART_Read(UART1,rxbuffer,param.length,200);
    Trace(1,"uart1 received data,length:%d,read:%d,data:%s",param.length,len,rxbuffer);
}

void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_NO_SIMCARD:
            Trace(10,"!!NO SIM CARD%d!!!!",pEvent->param1);
            break;

        case API_EVENT_ID_SYSTEM_READY:
            Trace(1,"system initialize complete");
            break;

        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
            Trace(2,"network register success");
            Network_StartAttach();
            break;

        case API_EVENT_ID_NETWORK_ATTACHED:
            Trace(2,"network attach success");
            Network_PDP_Context_t context = {
                .apn        ="cmnet",
                .userName   = ""    ,
                .userPasswd = ""
            };
            Network_StartActive(context);
            break;

        case API_EVENT_ID_NETWORK_ACTIVATED:
            Trace(2,"network activate success");
            flag = true;
            
            memset(buffer1,0,sizeof(buffer1));
            DNS_Status_t dnsRet = DNS_GetHostByName(DNS_DOMAIN,buffer1);
            if(dnsRet == DNS_STATUS_OK)
            {
                Trace(2,"DNS get ip address from domain success(return),domain:%s,ip:%s",DNS_DOMAIN,buffer1);
            }
            else if(dnsRet == DNS_STATUS_ERROR)
            {
                Trace(2,"DNS get ip address error(return)!!!");
            }
            //Start connect tcp server
            socketFd = Socket_TcpipConnect(TCP,buffer1,SERVER_PORT);
            Trace(2,"connect tcp server,socketFd:%d",socketFd);
            break;


        case API_EVENT_ID_SOCKET_CONNECTED:
            if(pEvent->param1 == socketFd)
            {
                socketFd2 = Socket_TcpipConnect(TCP,buffer1,SERVER_PORT);
                Trace(2,"connected tcp server,socketFd:%d",socketFd2);
            }
            Socket_TcpipWrite(pEvent->param1,"hello...test string\n",strlen("hello...test string\n"));
            Trace(2,"socket %d send %d bytes data to server:%s",pEvent->param1, strlen("hello...test string\n"),"hello...test string\n");
            break;

        case API_EVENT_ID_SOCKET_SENT:
        {
            int fd = pEvent->param1;
            Trace(2,"socket %d send data complete",fd);
            break;
        }
        /*
        case API_EVENT_ID_SOCKET_RECEIVED:
        {
            int fd = pEvent->param1;
            int length = pEvent->param2>RECEIVE_BUFFER_MAX_LENGTH?RECEIVE_BUFFER_MAX_LENGTH:pEvent->param2;
            memset(buffer1,0,sizeof(buffer1));
            length = Socket_TcpipRead(fd,buffer1,length);
            Trace(2,"socket %d received %d bytes data:%s",fd,length,buffer1);
            Socket_TcpipWrite(fd,buffer1,length);
            Trace(2,"send received data to server");
            
            if(++receivedDataCount > 2000)
            {
                Trace(2,"socket received %d times, now try to close socket",receivedDataCount);
                Socket_TcpipClose(socketFd);
                Socket_TcpipClose(socketFd2);
            }
            break;
        }
        case API_EVENT_ID_SOCKET_CLOSED:
        {
            int fd = pEvent->param1;
            Trace(2,"socket %d closed",fd);
            break;
        }
        case API_EVENT_ID_SOCKET_ERROR:
        {
            int fd = pEvent->param1;
            Trace(2,"socket %d error occurred,cause:%d",fd,pEvent->param2);
            break;
        }
        case API_EVENT_ID_DNS_SUCCESS:
            Trace(2,"DNS get ip address from domain success(event),domain:%s,ip:%s",pEvent->pParam1,pEvent->pParam2);
            break;

        case API_EVENT_ID_DNS_ERROR:
            Trace(2,"DNS get ip address error(event)!!!");
            break;
*/
        default:
            break;
/*

        case API_EVENT_ID_NETWORK_CELL_INFO:
        {
            uint8_t number = pEvent->param1;
            Network_Location_t* location = pEvent->pParam1;
            Trace(2,"network cell infomation,serving cell number:1, neighbor cell number:%d",number-1);
            
            for(int i=0;i<number;++i)
            {
                Trace(2,"cell %d info:%d%d%d,%d%d%d,%d,%d,%d,%d,%d,%d",i,
				location[i].sMcc[0], location[i].sMcc[1], location[i].sMcc[2], 
				location[i].sMnc[0], location[i].sMnc[1], location[i].sMnc[2],
				location[i].sLac, location[i].sCellID, location[i].iBsic,
                location[i].iRxLev, location[i].iRxLevSub, location[i].nArfcn);
            }
            break;
        }
        */
        
    }
}

void SecondTask(void *pData)
{

    #if 0
    char ip[16];
    while(1)
    {
        if(flag)
        {
            if(Network_GetIp(ip, sizeof(ip)))
            {
                Trace(1,"network local ip:%s",ip);
            }
            else
            {
                Trace(1,"network get local ip address fail");
            }
            for(int i=0;i<10;++i)
            {
                if(!Network_GetCellInfoRequst())
                {
                    Trace(1,"network get cell info fail");
                }
                OS_Sleep(5000);
            }

            flag = false;
        }
        OS_Sleep(1000);
    }
    #endif

    UART_Config_t config = {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity   = UART_PARITY_NONE,
        .rxCallback = OnUart1ReceivedData,
    };
    uint32_t times = 0;
    UART_Init(UART1,config);
    config.rxCallback = NULL;
    UART_Init(UART2,config);

    while(1)
    {
        uint8_t temp[20];
        uint8_t buffer[50];

        sprintf(temp,"hello:%d,1234567890 1234567890\n", ++times);
        UART_Write(UART1,temp,strlen(temp)+1);
        Trace(1,"UART_Write:%s %p",temp, &times);
        memset(buffer,0,sizeof(buffer));
        uint32_t readLen = UART_Read(UART2,buffer,10,3000);
        Trace(1,"UART_Read uart2,readLen:%d,data:%s",readLen,buffer);
        OS_Sleep(200);
    }
}

void MainTask(void *pData)
{
    API_Event_t* event=NULL;
    

    secondTaskHandle = OS_CreateTask(SecondTask,
        NULL, NULL, SECOND_TASK_STACK_SIZE, SECOND_TASK_PRIORITY, 0, 0, SECOND_TASK_NAME);

    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}

void go_Main(void)
{
    mainTaskHandle = OS_CreateTask(MainTask ,
        NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    OS_SetUserMainHandle(&mainTaskHandle);
}