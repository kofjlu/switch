#include <rtthread.h>
#include <rtdevice.h>
#include "rdcommon.h"

void sw_Lcd2McuProc(uint8 *_pcDate);
typedef void (*cbk_SerialRecv)(uint8 *_cBuf);
cbk_SerialRecv cbk_Serialsz_rev = sw_Lcd2McuProc; //串口接收回调

//串口设备按长度接收中断
#define MSG_SIZE 12
#define USART_NAME "uart3"
#define SIZE_DATE 2//不定长数据的数据长度所在的索引
static struct rt_semaphore rxSem;
static rt_device_t pSerial;
static struct rt_messagequeue stmq;
static uint8 msg_pool[1024] = {0};

/**
 * @brief  rd_usart_sz_send
 * @note   对外的串口发送函数
 * @param  *_pcDate: 发送的内容
 * @param  _iLen: cBuf的长度
 * @retval None
 */
void rd_usart_sz_send(uint8 *_pcDate, int _iLen)
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
            if (RT_NULL != cbk_Serialsz_rev)
            {
                cbk_Serialsz_rev(cBuf);
            }
        }
        rt_thread_delay(50);
    }
}

static void serial_thread_entry(void *parameter)
{
    char ch;
    char cBuf[MSG_SIZE] = {0};
    int iCnt = 0;
    int iTotalLen = 0;
    while (1)
    {
        while (rt_device_read(pSerial, -1, &ch, 1) != 1)
        {
            rt_sem_take(&rxSem, RT_WAITING_FOREVER);
        }
        cBuf[iCnt++] = ch;
        if (iCnt == SIZE_DATE+1)
        {
            if(cBuf[SIZE_DATE] + SIZE_DATE+1 <= MSG_SIZE)
            {
                iTotalLen = cBuf[SIZE_DATE] + SIZE_DATE+1;
            }
        }
        if (iCnt >= iTotalLen && iTotalLen > 0)
        {
            rt_mq_send(&stmq, &cBuf, MSG_SIZE);
            iCnt = 0;
            iTotalLen = 0;
        }
    }
}

//static int Usart_test(int argc, char**argv)
static int Usart_test(void)
{
    int iRet = RT_EOK;
    char cUsartname[RT_NAME_MAX] = {0};
/*     if (argc == 2)
    {
        rt_strncpy(cUsartname, argv[1], RT_NAME_MAX);
    }
    else
    { */
        rt_strncpy(cUsartname, USART_NAME, RT_NAME_MAX);
/*     } */
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
    rt_thread_t pThread0 = rt_thread_create("SerialTx", serial_thread_recv, RT_NULL, 8192, 0, 10);
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
    rt_thread_t pThread = rt_thread_create("SerialRx", serial_thread_entry, RT_NULL, 1024, 25, 10);
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
INIT_APP_EXPORT(Usart_test);