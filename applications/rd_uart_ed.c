#include <rtthread.h>
#include <rtdevice.h>
#include "rdcommon.h"

void sw_NetWorkProc(uint8 *_pcDate);
typedef void (*cbk_SerialRecv)(uint8 *_cBuf);
cbk_SerialRecv cbk_Serialed_rev = sw_NetWorkProc; //串口接收回调

//串口设备按结尾数据中断
#define MSG_SIZE 128 //串口缓冲区大小
#define USART_NAME "uart4" //串口设备名称
#define TX_THREAD_NAME "SerialTx" //串口发送线程名称
#define RX_THREAD_NAME "SerialRx" //串口接收线程名称
#define END_FLAG 0x0A //结尾数据为0x0D时为一包数据
#define END_FLAG2 0x0D //结尾数据为0x0D时为一包数据
static struct rt_semaphore rxSem;
static rt_device_t pSerial;
static struct rt_messagequeue stmq;
static uint8 msg_pool[1024] = {0};

/**
 * @brief  rd_usart_ed_send
 * @note   对外的串口发送函数
 * @param  *_pcDate: 发送的内容
 * @param  _iLen: cBuf的长度
 * @retval None
 */
void rd_usart_ed_send(uint8 *_pcDate, int _iLen)
{
    rt_device_write(pSerial, 0, _pcDate, _iLen);
}

static rt_err_t rx_Input(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rxSem);
    return RT_EOK;
}

static void serial_thread_recv(void *parameter)
{
    uint8 cBuf[MSG_SIZE] = {0};
    while (1)
    {
        if (rt_mq_recv(&stmq, &cBuf, sizeof(cBuf), RT_WAITING_FOREVER) == RT_EOK)
        {
            //rt_device_write(pSerial, 0, cBuf, MSG_SIZE);
            if (RT_NULL != cbk_Serialed_rev)
            {
                cbk_Serialed_rev(cBuf);
            }          
        }
        rt_thread_delay(50);
    }
}

extern int g_MAC_flag;
extern int g_NET_flag;
static void serial_thread_entry(void *parameter)
{
    char ch;
    char cBuf[MSG_SIZE] = {0};
    int iCnt = 0;
    while (1)
    {
        while (rt_device_read(pSerial, -1, &ch, 1) != 1)
        {
            rt_sem_take(&rxSem, RT_WAITING_FOREVER);
        }
        cBuf[iCnt++] = ch;
        //rt_kprintf("%c\r\n", ch);
        if (ch == 0x61 &&(g_MAC_flag == 1 || g_NET_flag == 1))
        {
            //rt_kprintf("recv a\r\n");
            rd_usart_ed_send("a", 1);
        }
        else if (ch == END_FLAG || ch == END_FLAG2)//此处逻辑可按照实际结尾数据情况修改
        {
            rt_mq_send(&stmq, &cBuf, MSG_SIZE);
            rt_memset(cBuf, 0, MSG_SIZE);
            iCnt = 0;
        }
    }
}

static int Usart_init(void)
{
    int iRet = RT_EOK;
    char cUsartname[RT_NAME_MAX] = {0};
    rt_strncpy(cUsartname, USART_NAME, RT_NAME_MAX);
    pSerial = rt_device_find(cUsartname);
    if (!pSerial)
    {
        rt_kprintf("find %s failed!\n", cUsartname);
        iRet = RT_ERROR;
        goto END;
    }
    rt_sem_init(&rxSem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    iRet = rt_mq_init(&stmq, "mqt", msg_pool, MSG_SIZE, sizeof(msg_pool), RT_IPC_FLAG_FIFO);
    if (iRet != RT_EOK)
    {
        rt_kprintf("messagequeue init failed!");
        goto END;
    }
    rt_thread_t pThread0 = rt_thread_create(TX_THREAD_NAME, serial_thread_recv, RT_NULL, 8192, 25, 10);
    if (pThread0 != RT_NULL)
    {
        rt_thread_startup(pThread0);
    }
    else
    {
        iRet = RT_ERROR;
        goto END;
    }

    rt_device_open(pSerial, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(pSerial, rx_Input);
    rt_thread_t pThread = rt_thread_create(RX_THREAD_NAME, serial_thread_entry, RT_NULL, 1024, 25, 10);
    if (pThread != RT_NULL)
    {
        rt_thread_startup(pThread);
    }
    else
    {
        iRet = RT_ERROR;
    }
END:
    return iRet;
}
INIT_APP_EXPORT(Usart_init);