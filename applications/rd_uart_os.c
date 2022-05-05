#include <rtthread.h>
#include <rtdevice.h>
#include "rdcommon.h"

#define USART_NAME "uart1" //串口设备名称
static rt_device_t pSerial;

/**
 * @brief  rd_usart_os_send
 * @note   对外的串口发送函数
 * @param  *_pcDate: 发送的内容
 * @param  _iLen: cBuf的长度
 * @retval None
 */
void rd_usart_os_send(uint8 *_pcDate, int _iLen)
{
    rt_device_write(pSerial, 0, _pcDate, _iLen);
}

static int Usart_os_init(void)
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
    rt_device_open(pSerial, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
END:
    return iRet;
}
INIT_APP_EXPORT(Usart_os_init);