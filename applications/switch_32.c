#include "switch_32.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <dfs_posix.h>
#include "config.h"

#define FOUR_VERSION //此宏控制是否为第四套，注释掉宏表示前三套，否则为第四套

const uint16 MainButtonList[2][MAIN_BUTTON_MAX] = {{ONEBY1_BUTTON01, ONEBY1_BUTTON02, ONEBY1_BUTTON03, ONEBY1_BUTTON04, 
                                                    ONEBY1_BUTTON05, ONEBY1_BUTTON06, ONEBY1_BUTTON07, ONEBY1_BUTTON08,
                                                    ONEBY1_BUTTON09, ONEBY1_BUTTON10, ONEBY1_BUTTON11, ONEBY1_BUTTON12,
                                                    ONEBY1_BUTTON13, ONEBY1_BUTTON14, ONEBY1_BUTTON15, ONEBY1_BUTTON16,
                                                    ONEBY1_BUTTON17, ONEBY1_BUTTON18, ONEBY1_BUTTON19, ONEBY1_BUTTON20,
                                                    ONEBY1_BUTTON21, ONEBY1_BUTTON22, ONEBY1_BUTTON23, ONEBY1_BUTTON24,
                                                    ONEBY1_BUTTON25, ONEBY1_BUTTON26, ONEBY1_BUTTON27, ONEBY1_BUTTON28,
                                                    ONEBY1_BUTTON29, ONEBY1_BUTTON30, ONEBY1_BUTTON31, ONEBY1_BUTTON32},
                                                    {MAIN_BUTTON_1_EX, MAIN_BUTTON_2_EX, MAIN_BUTTON_3_EX, MAIN_BUTTON_4_EX,
                                                    MAIN_BUTTON_5_EX, MAIN_BUTTON_6_EX, MAIN_BUTTON_7_EX, MAIN_BUTTON_8_EX,
                                                    MAIN_BUTTON_9_EX, MAIN_BUTTON_10_EX, MAIN_BUTTON_11_EX, MAIN_BUTTON_12_EX,
                                                    MAIN_BUTTON_13_EX, MAIN_BUTTON_14_EX, MAIN_BUTTON_15_EX, MAIN_BUTTON_16_EX,
                                                    MAIN_BUTTON_17_EX, MAIN_BUTTON_18_EX, MAIN_BUTTON_19_EX, MAIN_BUTTON_20_EX,
                                                    MAIN_BUTTON_21_EX, MAIN_BUTTON_22_EX, MAIN_BUTTON_23_EX, MAIN_BUTTON_24_EX,
                                                    MAIN_BUTTON_25_EX, MAIN_BUTTON_26_EX, MAIN_BUTTON_27_EX, MAIN_BUTTON_28_EX,
                                                    MAIN_BUTTON_29_EX, MAIN_BUTTON_30_EX, MAIN_BUTTON_31_EX, MAIN_BUTTON_32_EX}};
static TDeviceParam g_stDevicePara = {0};//存放设备参数的全局
static uint16 g_CLKDate[32] = {0};//存放32路开关状态的全局
static char cBuf[56] = {0x66, 0x66, 0x66, 0x66, 0x01, 0x01, 0x00, 0x00,
                        0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x77, 0x77, 0x77, 0x77};//另一台设备的协议格式
static int g_iSame = 0;//点击全部相同前是否选择通道
static int g_iBlackTim = 0;

//外部接口无需关心实现
void rd_usart_ed_send(uint8 *_pcDate, int _iLen);//对外网口发送
void rd_usart_sz_send(uint8 *_pcDate, int _iLen);//发送给串口屏
void rd_usart_os_send(uint8 *_pcDate, int _iLen);//另一台设备发送给32路开关

int TXU4(int argc, char**argv)
{
    char cDate[128] = {0};
    sprintf(cDate, "%s\n", argv[1]);
    rd_usart_ed_send(cDate, strlen(cDate));
}
MSH_CMD_EXPORT(TXU4, uart4);

/**
 * @brief  sw_Big2Little
 * @note   大小端转换接口
 * @param  *_pcDate: LCD协议buf
 * @param  _iLen: _pcDate长度
 * @param  *_pOutPut: MCU协议buf
 * @param  _iOlen: _pOutPut长度
 * @param  _flag: LCD2MCU表示屏幕协议转MCU MCU2LCD表示MCU转LCD
 * @retval 
 */
static int sw_Big2Little(uint8 *_pcDate, int _iLen, void *_pOutPut, int _iOlen, EFlagBL _flag)
{
    int iRet = 0;
    if (LCD2MCU == _flag)
    {
        Tlcd2mcu *stDateR = (Tlcd2mcu *)_pOutPut;
        rt_memcpy(stDateR->ucDate, _pcDate, sizeof(Tlcd2mcu));
        stDateR->usHead = RD_BIG_LITTLE_SWAP16(stDateR->usHead);
        stDateR->stAddr.usaddr = RD_BIG_LITTLE_SWAP16(stDateR->stAddr.usaddr);
        stDateR->ucValue = RD_BIG_LITTLE_SWAP16(stDateR->ucValue);
    }
    else if (MCU2LCD == _flag)
    {
        Tmcu2lcd *stDateS = (Tmcu2lcd *)_pOutPut;
        stDateS->usHead = RD_BIG_LITTLE_SWAP16(stDateS->usHead);
        stDateS->stAddr.usaddr = RD_BIG_LITTLE_SWAP16(stDateS->stAddr.usaddr);
        stDateS->stDateEx.stIcon.usicon = RD_BIG_LITTLE_SWAP16(stDateS->stDateEx.stIcon.usicon);
        rt_memcpy(_pcDate, stDateS->ucDate, sizeof(Tmcu2lcd));
    }
    else
    {
        iRet = -1;
    }
    return iRet;
}

static void sw_SetBeepblink(void)
{
    rt_pin_write(BEEP, PIN_HIGH);
    rt_thread_mdelay(500);
    rt_pin_write(BEEP, PIN_LOW);
}

static void sw_CLKData2UartDate(void)
{
    
}

/**
 * @brief  sw_McuSendMsg2Lcd
 * @note   Mcu向屏幕发送消息
 * @param  *_stMcu2lcd: 要发送的结构体
 * @retval None
 */
static void sw_McuSendMsg2Lcd(Tmcu2lcd *_stMcu2lcd)
{
    int i = 0;
    uint8 cBuf[sizeof(Tmcu2lcd)] = {0};
    (void)sw_Big2Little(cBuf, sizeof(Tmcu2lcd), _stMcu2lcd, sizeof(Tmcu2lcd), MCU2LCD);
/*     rt_kprintf("%s[%d]_stMcu2lcd->ucSize = %d\t", __func__, __LINE__, _stMcu2lcd->ucSize);
    for (i = 0; i < sizeof(Tmcu2lcd); i++)
    {
        rt_kprintf("%o\t", cBuf[i]);
    }
    rt_kprintf("\r\n"); */
    rd_usart_sz_send(cBuf, _stMcu2lcd->ucSize + 3);
}

/**
 * @brief  sw_CLK1WriteByte2SW
 * @note   前16路产生脉冲
 * @param  _iDate: 一般情况下为g_CLKDate全局
 * @retval None
 */
static void sw_CLK1WriteByte2SW(uint16 _iDate)
{
    int i = 0;
    //rt_pin_write(RCLK1, PIN_LOW);
    for (i = 0; i < CLK_COUNT_MAX; i++)
    {
        if (((_iDate>>i)&0x0001))
        {
            rt_pin_write(IN1, PIN_LOW);
        }
        rt_pin_write(CLK1, PIN_HIGH);
        rt_pin_write(CLK1, PIN_LOW);
        rt_pin_write(IN1, PIN_HIGH);
        rt_hw_us_delay(1);
    }
    //rt_pin_write(RCLK1, PIN_HIGH);
    rt_hw_us_delay(4);
}

/**
 * @brief  sw_CLK2WriteByte2SW
 * @note   后16路产生脉冲
 * @param  _iDate: 一般情况下为g_CLKDate全局
 * @retval None
 */
static void sw_CLK2WriteByte2SW(uint16 _iDate)
{
    int i = 0;
    //rt_pin_write(RCLK2, PIN_LOW);
    for (i = 0; i < CLK_COUNT_MAX; i++)
    {
        if (((_iDate>>i)&0x0001))
        {
            rt_pin_write(IN2, PIN_LOW);
        }
        rt_pin_write(CLK2, PIN_HIGH);
        rt_pin_write(CLK2, PIN_LOW);
        rt_pin_write(IN2, PIN_HIGH);
        rt_hw_us_delay(1);
    }
    //rt_pin_write(RCLK2, PIN_HIGH);
    rt_hw_us_delay(4);
}

/**
 * @brief  sw_SetLEDState
 * @note   设置告警界面LED
 * @param  _eLedAddr: LED控件地址
 * @param  _eLedState: LED状态
 * @retval None
 */
static void sw_SetLEDState(ELabelAddr _eLedAddr, ELedState _eLedState)
{
    Tmcu2lcd stMcu2lcd = {0};
    stMcu2lcd.usHead = FRAME_HEAD;
    stMcu2lcd.ucCmd = CMD_WRITE;
    stMcu2lcd.ucSize = 0x05;
    stMcu2lcd.stAddr.Addr = _eLedAddr;
    stMcu2lcd.stAddr.tail = 0;
    stMcu2lcd.stDateEx.stIcon.usicon = _eLedState;
    sw_McuSendMsg2Lcd(&stMcu2lcd);
}

/**
 * @brief  sw_SendCLKDate
 * @note   发送32路脉冲数据
 * @retval None
 */
static void sw_SendCLKDate()
{
    int i = 0;
    rt_pin_write(RCLK1, PIN_LOW);
    rt_pin_write(RCLK2, PIN_LOW);
    for (i = 0; i < 32; i++)
    {
        if (i < 16)
        {
            sw_CLK1WriteByte2SW(g_CLKDate[i]);
        }
        else
        {
            sw_CLK2WriteByte2SW(g_CLKDate[i]);
        }
    }
    rt_pin_write(RCLK1, PIN_HIGH);
    rt_pin_write(RCLK2, PIN_HIGH);
}

/**
 * @brief  sw_get_DeviceParam
 * @note   获取g_stDevicePara的指针
 * @retval TDeviceParam*
 */
static TDeviceParam* sw_get_DeviceParam(void)
{
    return &g_stDevicePara;
}

/**
 * @brief  sw_set_DeviceParam
 * @note   更新参数值到全局m_iDeviceAddr
 * @param  *_stDevicePara: 配置参数结构体
 * @retval None
 */
static void sw_set_DeviceParam(TDeviceParam *_stDevicePara)
{
    rt_memcpy(&g_stDevicePara, _stDevicePara, sizeof(TDeviceParam));
}

/**
 * @brief  sw_default_DeviceParam
 * @note   设备参数恢复默认值
 * @retval None
 */
static void sw_default_DeviceParam(void)
{
    TDeviceParam *pstDevicePara = sw_get_DeviceParam();
    pstDevicePara->stNetParam.m_iIPaddr01 = 192;
    pstDevicePara->stNetParam.m_iIPaddr02 = 168;
    pstDevicePara->stNetParam.m_iIPaddr03 = 0;
    pstDevicePara->stNetParam.m_iIPaddr04 = 178;
    pstDevicePara->stNetParam.m_iNetmask01 = 255;
    pstDevicePara->stNetParam.m_iNetmask02 = 255;
    pstDevicePara->stNetParam.m_iNetmask03 = 255;
    pstDevicePara->stNetParam.m_iNetmask04 = 0;
    pstDevicePara->stNetParam.m_iNetgate01 = 192;
    pstDevicePara->stNetParam.m_iNetgate02 = 168;
    pstDevicePara->stNetParam.m_iNetgate03 = 0;
    pstDevicePara->stNetParam.m_iNetgate04 = 1;
    pstDevicePara->stNetParam.m_iPCaddr01 = 192;
    pstDevicePara->stNetParam.m_iPCaddr02 = 168;
    pstDevicePara->stNetParam.m_iPCaddr03 = 0;
    pstDevicePara->stNetParam.m_iPCaddr04 = 68;
    pstDevicePara->stNetParam.m_iPCport = 4001;
    pstDevicePara->stNetParam.m_iDeviceport = 3502;
    pstDevicePara->stNetParam.m_iMode = MODE_TCPSER;
    pstDevicePara->m_iBitRate = BIT_RATE_9600;
    pstDevicePara->m_iParityCheck = BIT8_ON_CHECK;
    pstDevicePara->m_iSleepTim = 2;
    pstDevicePara->m_iMode = CTRL_REMOTE;
    pstDevicePara->m_iDeviceAddr = 255;
    sw_set_DeviceParam(pstDevicePara);
}

/**
 * @brief  Cfg_SetDevicepara
 * @note   设置设备参数
 * @param  *_pstDevicePara: 参数结构体
 * @retval None
 */
static void Cfg_SetDevicepara(TDeviceParam *_pstDevicePara)
{
    int fd = 0;
    char buf[128];
    fd = open(CONFIG_NAME, O_WRONLY);
    sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", 
        _pstDevicePara->stNetParam.m_iIPaddr01,
        _pstDevicePara->stNetParam.m_iIPaddr02,
        _pstDevicePara->stNetParam.m_iIPaddr03,
        _pstDevicePara->stNetParam.m_iIPaddr04,
        _pstDevicePara->stNetParam.m_iNetmask01,
        _pstDevicePara->stNetParam.m_iNetmask02,
        _pstDevicePara->stNetParam.m_iNetmask03,
        _pstDevicePara->stNetParam.m_iNetmask04,
        _pstDevicePara->stNetParam.m_iNetgate01,
        _pstDevicePara->stNetParam.m_iNetgate02,
        _pstDevicePara->stNetParam.m_iNetgate03,
        _pstDevicePara->stNetParam.m_iNetgate04,
        _pstDevicePara->stNetParam.m_iPCaddr01,
        _pstDevicePara->stNetParam.m_iPCaddr02,
        _pstDevicePara->stNetParam.m_iPCaddr03,
        _pstDevicePara->stNetParam.m_iPCaddr04,
        _pstDevicePara->stNetParam.m_iPCport,
        _pstDevicePara->stNetParam.m_iDeviceport,
        _pstDevicePara->stNetParam.m_iMode,
        _pstDevicePara->m_iBitRate,
        _pstDevicePara->m_iParityCheck,
        _pstDevicePara->m_iSleepTim,
        _pstDevicePara->m_iMode,
        _pstDevicePara->m_iDeviceAddr);
    //rt_kprintf("%s\r\n", buf);
    write(fd, buf, strlen(buf));
    fsync(fd);
    close(fd);
}

/**
 * @brief  Cfg_GetDevicepara
 * @note   获取设备参数
 * @param  *_pstDevicePara: 设备参数出参
 * @retval None
 */
static void Cfg_GetDevicepara(TDeviceParam *_pstDevicePara)
{
    int fd = 0;
    int Gate01, Gate02, Gate03, Gate04 = 0;
    char buf[128];
    fd = open(CONFIG_NAME, O_RDONLY);
    read(fd, buf, sizeof(buf));
    //rt_kprintf("%s\r\n", buf);
    sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", 
        &_pstDevicePara->stNetParam.m_iIPaddr01,
        &_pstDevicePara->stNetParam.m_iIPaddr02,
        &_pstDevicePara->stNetParam.m_iIPaddr03,
        &_pstDevicePara->stNetParam.m_iIPaddr04,
        &_pstDevicePara->stNetParam.m_iNetmask01,
        &_pstDevicePara->stNetParam.m_iNetmask02,
        &_pstDevicePara->stNetParam.m_iNetmask03,
        &_pstDevicePara->stNetParam.m_iNetmask04,
        &_pstDevicePara->stNetParam.m_iNetgate01,
        &_pstDevicePara->stNetParam.m_iNetgate02,
        &_pstDevicePara->stNetParam.m_iNetgate03,
        &_pstDevicePara->stNetParam.m_iNetgate04,
        &_pstDevicePara->stNetParam.m_iPCaddr01,
        &_pstDevicePara->stNetParam.m_iPCaddr02,
        &_pstDevicePara->stNetParam.m_iPCaddr03,
        &_pstDevicePara->stNetParam.m_iPCaddr04,
        &_pstDevicePara->stNetParam.m_iPCport,
        &_pstDevicePara->stNetParam.m_iDeviceport,
        &_pstDevicePara->stNetParam.m_iMode,
        &_pstDevicePara->m_iBitRate,
        &_pstDevicePara->m_iParityCheck,
        &_pstDevicePara->m_iSleepTim,
        &_pstDevicePara->m_iMode,
        &_pstDevicePara->m_iDeviceAddr);
    close(fd);
}

/**
 * @brief  sw_ConfigPack
 * @note   配置界面更新屏幕显示值(除端口号通用)
 * @param  _iKey: 控件值
 * @param  _iValue: 要显示的值
 * @retval None
 */
static void sw_ConfigPack(uint16 _iKey, uint16 _iValue)
{
    Tmcu2lcd stMcu2lcd = {0};
    stMcu2lcd.usHead = FRAME_HEAD;
    stMcu2lcd.ucCmd = CMD_WRITE;
    stMcu2lcd.ucSize = 0x05;
    stMcu2lcd.stAddr.Addr = _iKey;
    stMcu2lcd.stDateEx.stIcon.usicon = _iValue;
    sw_McuSendMsg2Lcd(&stMcu2lcd);
}

/**
 * @brief  sw_ConfigPort
 * @note   配置界面更新屏幕显示值(端口号特殊处理)
 * @param  _iKey: 控件值
 * @param  _iValue: 要显示的值
 * @retval None
 */
static void sw_ConfigPort(uint16 _iKey, uint16 _iValue)
{
    Tmcu2lcd stMcu2lcd = {0};
    stMcu2lcd.usHead = FRAME_HEAD;
    stMcu2lcd.ucCmd = CMD_WRITE;
    stMcu2lcd.ucSize = 0x07;
    stMcu2lcd.stAddr.Addr = _iKey;
    stMcu2lcd.stDateEx.stvalue = 0;
    stMcu2lcd.stDateEx.stvaluex = _iValue;
    sw_McuSendMsg2Lcd(&stMcu2lcd);
}

/**
 * @brief  sw_UpdateDevicePara
 * @note   将全局变量的值更新给屏幕
 * @retval None
 */
static void sw_UpdateDevicePara(void)
{
    TDeviceParam *pstDeviceParam = sw_get_DeviceParam();
    sw_ConfigPort(EDIT_REMOTE_PORT,RD_BIG_LITTLE_SWAP16(pstDeviceParam->stNetParam.m_iPCport));
    sw_ConfigPort(EDIT_LOCAL_PORT,RD_BIG_LITTLE_SWAP16(pstDeviceParam->stNetParam.m_iDeviceport));
    sw_ConfigPack(EDIT_LOCAL_IP01,pstDeviceParam->stNetParam.m_iIPaddr01);
    sw_ConfigPack(EDIT_LOCAL_IP02,pstDeviceParam->stNetParam.m_iIPaddr02);
    sw_ConfigPack(EDIT_LOCAL_IP03,pstDeviceParam->stNetParam.m_iIPaddr03);
    sw_ConfigPack(EDIT_LOCAL_IP04,pstDeviceParam->stNetParam.m_iIPaddr04);
    sw_ConfigPack(EDIT_NETMASK01,pstDeviceParam->stNetParam.m_iNetmask01);
    sw_ConfigPack(EDIT_NETMASK02,pstDeviceParam->stNetParam.m_iNetmask02);
    sw_ConfigPack(EDIT_NETMASK03,pstDeviceParam->stNetParam.m_iNetmask03);
    sw_ConfigPack(EDIT_NETMASK04,pstDeviceParam->stNetParam.m_iNetmask04);
    sw_ConfigPack(EDIT_NETGATE01,pstDeviceParam->stNetParam.m_iNetgate01);
    sw_ConfigPack(EDIT_NETGATE02,pstDeviceParam->stNetParam.m_iNetgate02);
    sw_ConfigPack(EDIT_NETGATE03,pstDeviceParam->stNetParam.m_iNetgate03);
    sw_ConfigPack(EDIT_NETGATE04,pstDeviceParam->stNetParam.m_iNetgate04);
    sw_ConfigPack(EDIT_REMOTE_IP01,pstDeviceParam->stNetParam.m_iPCaddr01);
    sw_ConfigPack(EDIT_REMOTE_IP02,pstDeviceParam->stNetParam.m_iPCaddr02);
    sw_ConfigPack(EDIT_REMOTE_IP03,pstDeviceParam->stNetParam.m_iPCaddr03);
    sw_ConfigPack(EDIT_REMOTE_IP04,pstDeviceParam->stNetParam.m_iPCaddr04);
    sw_ConfigPack(EDIT_MODE,pstDeviceParam->stNetParam.m_iMode);
    sw_ConfigPack(EDIT_CTRL,pstDeviceParam->m_iMode);
    sw_ConfigPack(EDIT_DEVICE_ADDR,pstDeviceParam->m_iDeviceAddr);
    sw_ConfigPack(EDIT_SLEEP_TIME,pstDeviceParam->m_iSleepTim);
    sw_ConfigPack(EDIT_PARITY_CHECK,pstDeviceParam->m_iParityCheck);
    sw_ConfigPack(EDIT_BIT_RATE,pstDeviceParam->m_iBitRate);
}

/**
 * @brief  sw_GetEventType
 * @note   协议预处理转换为不同的事件类型
 * @param  _uiCmd: 命令码
 * @retval EEventType 事件类型
 */
static EEventType sw_GetEventType(uint16 _uiCmd)
{
    EEventType eEvevtType = CMD_NULL;
    switch (_uiCmd)
    {
    case MAIN_BUTTON_1:
    case MAIN_BUTTON_2:
    case MAIN_BUTTON_3:
    case MAIN_BUTTON_4:
    case MAIN_BUTTON_5:
    case MAIN_BUTTON_6:
    case MAIN_BUTTON_7:
    case MAIN_BUTTON_8:
    case MAIN_BUTTON_9:
    case MAIN_BUTTON_10:
    case MAIN_BUTTON_11:
    case MAIN_BUTTON_12:
    case MAIN_BUTTON_13:
    case MAIN_BUTTON_14:
    case MAIN_BUTTON_15:
    case MAIN_BUTTON_16:
    case MAIN_BUTTON_17:
    case MAIN_BUTTON_18:
    case MAIN_BUTTON_19:
    case MAIN_BUTTON_20:
    case MAIN_BUTTON_21:
    case MAIN_BUTTON_22:
    case MAIN_BUTTON_23:
    case MAIN_BUTTON_24:
    case MAIN_BUTTON_25:
    case MAIN_BUTTON_26:
    case MAIN_BUTTON_27:
    case MAIN_BUTTON_28:
    case MAIN_BUTTON_29:
    case MAIN_BUTTON_30:
    case MAIN_BUTTON_31:
    case MAIN_BUTTON_32:
        eEvevtType = CMD_MAIN_BUTTON;
        break;
    case SUB_BUTTON_GROUP:
        eEvevtType = CMD_SUB_BUTTON;
        break;
    case SUB_BUTTON_ALL:
        eEvevtType = CMD_SUB_ALL;
        break;
    case SUB_BUTTON_OK:
        eEvevtType = CMD_SUB_SAVE;
        break;
    case 0x261:
        eEvevtType = CMD_ONEBY1;
        break;
    case ONEBY1_OK:
        eEvevtType = CMD_ONEBY1_OK;
        break;
    case CONFIG_DEFAULT:
        eEvevtType = CMD_CONFIG_DEFAULT;
        break;
    case CONFIG_BACK:
        eEvevtType = CMD_CONFIG_OK;
        break;
    case EDIT_LOCAL_IP01:
    case EDIT_LOCAL_IP02:
    case EDIT_LOCAL_IP03:
    case EDIT_LOCAL_IP04:
    case EDIT_NETMASK01:
    case EDIT_NETMASK02:
    case EDIT_NETMASK03:
    case EDIT_NETMASK04:
    case EDIT_REMOTE_IP01:
    case EDIT_REMOTE_IP02:
    case EDIT_REMOTE_IP03:
    case EDIT_REMOTE_IP04:
    case EDIT_NETGATE01:
    case EDIT_NETGATE02:
    case EDIT_NETGATE03:
    case EDIT_NETGATE04:
    case EDIT_SLEEP_TIME:
    case EDIT_DEVICE_ADDR:
    case EDIT_MODE:
    case EDIT_CTRL:
    case EDIT_BIT_RATE:
    case EDIT_PARITY_CHECK:
        eEvevtType = CMD_CONFIG_ITEM;
        break;
    case MAIN_BUTTON_SETTING:
        eEvevtType = CMD_IN_CONIFG;
        break;
    case EDIT_LOCAL_PORT:
    case EDIT_REMOTE_PORT:
        eEvevtType = CMD_SET_PORT;
        break;
    default:
        break;
    }
}

/**
 * @brief  sw_ConfigItemProc
 * @note   将屏幕下发的配置值更新给全局
 * @param  _iAddr: 控件地址
 * @param  _iValue: 控件值
 * @retval None
 */
static void sw_ConfigItemProc(uint16 _iAddr, uint16 _iValue)
{
    TDeviceParam *pstDeviceParam = sw_get_DeviceParam();
    switch (_iAddr)
    {
    case EDIT_LOCAL_IP01:
        pstDeviceParam->stNetParam.m_iIPaddr01 = _iValue;
        break;
    case EDIT_LOCAL_IP02:
        pstDeviceParam->stNetParam.m_iIPaddr02 = _iValue;
        break;
    case EDIT_LOCAL_IP03:
        pstDeviceParam->stNetParam.m_iIPaddr03 = _iValue;
        break;
    case EDIT_LOCAL_IP04:
        pstDeviceParam->stNetParam.m_iIPaddr04 = _iValue;
        break;
    case EDIT_NETMASK01:
        pstDeviceParam->stNetParam.m_iNetmask01 = _iValue;
        break;
    case EDIT_NETMASK02:
        pstDeviceParam->stNetParam.m_iNetmask02 = _iValue;
        break;
    case EDIT_NETMASK03:
        pstDeviceParam->stNetParam.m_iNetmask03 = _iValue;
        break;
    case EDIT_NETMASK04:
        pstDeviceParam->stNetParam.m_iNetmask04 = _iValue;
        break;
    case EDIT_REMOTE_IP01:
        pstDeviceParam->stNetParam.m_iPCaddr01 = _iValue;
        break;
    case EDIT_REMOTE_IP02:
        pstDeviceParam->stNetParam.m_iPCaddr02 = _iValue;
        break;
    case EDIT_REMOTE_IP03:
        pstDeviceParam->stNetParam.m_iPCaddr03 = _iValue;
        break;
    case EDIT_REMOTE_IP04:
        pstDeviceParam->stNetParam.m_iPCaddr04 = _iValue;
        break;
    case EDIT_NETGATE01:
        pstDeviceParam->stNetParam.m_iNetgate01 = _iValue;
        break;
    case EDIT_NETGATE02:
        pstDeviceParam->stNetParam.m_iNetgate02 = _iValue;
        break;
    case EDIT_NETGATE03:
        pstDeviceParam->stNetParam.m_iNetgate03 = _iValue;
        break;
    case EDIT_NETGATE04:
        pstDeviceParam->stNetParam.m_iNetgate04 = _iValue;
        break;
    case EDIT_SLEEP_TIME:
        pstDeviceParam->m_iSleepTim = _iValue;
        break;
    case EDIT_DEVICE_ADDR:
        pstDeviceParam->m_iDeviceAddr = _iValue;
        break;
    case EDIT_MODE:
        pstDeviceParam->stNetParam.m_iMode = _iValue;
        break;
    case EDIT_CTRL:
        pstDeviceParam->m_iMode = _iValue;
        break;
    case EDIT_BIT_RATE:
        pstDeviceParam->m_iBitRate = _iValue;
        break;
    case EDIT_PARITY_CHECK:       
        pstDeviceParam->m_iParityCheck = _iValue; 
        break;   
    case EDIT_LOCAL_PORT:
        pstDeviceParam->stNetParam.m_iDeviceport = _iValue;
        break;
    case EDIT_REMOTE_PORT:
        pstDeviceParam->stNetParam.m_iPCport = _iValue;
        break;
    default:
        break;
    }
}

int g_MAC_flag = 0;
int g_NET_flag = 0;

static void sw_SetNetpara2module(TDeviceParam *_pstDeviceParam)
{
    char cBuf[1024] = {0};
    g_NET_flag = 1;
    rd_usart_ed_send("+++", strlen("+++"));
    rt_thread_mdelay(100);
    sprintf(cBuf, "AT+WANN=static,%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d\n",
        _pstDeviceParam->stNetParam.m_iIPaddr01,
        _pstDeviceParam->stNetParam.m_iIPaddr02,
        _pstDeviceParam->stNetParam.m_iIPaddr03,
        _pstDeviceParam->stNetParam.m_iIPaddr04,
        _pstDeviceParam->stNetParam.m_iNetmask01,
        _pstDeviceParam->stNetParam.m_iNetmask02,
        _pstDeviceParam->stNetParam.m_iNetmask03,
        _pstDeviceParam->stNetParam.m_iNetmask04,
        _pstDeviceParam->stNetParam.m_iNetgate01,
        _pstDeviceParam->stNetParam.m_iNetgate02,
        _pstDeviceParam->stNetParam.m_iNetgate03,
        _pstDeviceParam->stNetParam.m_iNetgate04);
    rd_usart_ed_send(cBuf, strlen(cBuf));
    rt_thread_mdelay(SEC);
    rt_memset(cBuf, 0, sizeof(cBuf));
    sprintf(cBuf, "AT+SOCKPORT=%d\n", _pstDeviceParam->stNetParam.m_iDeviceport);
    rd_usart_ed_send(cBuf, strlen(cBuf));
    rt_thread_mdelay(SEC);
    rt_memset(cBuf, 0, sizeof(cBuf));
    if (_pstDeviceParam->stNetParam.m_iMode == 0)
    {
        sprintf(cBuf, "AT+SOCK=TCPS,%d.%d.%d.%d,%d\n", 
            _pstDeviceParam->stNetParam.m_iPCaddr01,
            _pstDeviceParam->stNetParam.m_iPCaddr02,
            _pstDeviceParam->stNetParam.m_iPCaddr03,
            _pstDeviceParam->stNetParam.m_iPCaddr04,
            _pstDeviceParam->stNetParam.m_iPCport);
    }
    else if (_pstDeviceParam->stNetParam.m_iMode == 2)
    {
        sprintf(cBuf, "AT+SOCK=TCPC,%d.%d.%d.%d,%d\n", 
            _pstDeviceParam->stNetParam.m_iPCaddr01,
            _pstDeviceParam->stNetParam.m_iPCaddr02,
            _pstDeviceParam->stNetParam.m_iPCaddr03,
            _pstDeviceParam->stNetParam.m_iPCaddr04,
            _pstDeviceParam->stNetParam.m_iPCport);
    }
    else if (_pstDeviceParam->stNetParam.m_iMode == 1)
    {
        sprintf(cBuf, "AT+SOCK=UDPC,%d.%d.%d.%d,%d\n", 
            _pstDeviceParam->stNetParam.m_iPCaddr01,
            _pstDeviceParam->stNetParam.m_iPCaddr02,
            _pstDeviceParam->stNetParam.m_iPCaddr03,
            _pstDeviceParam->stNetParam.m_iPCaddr04,
            _pstDeviceParam->stNetParam.m_iPCport);
    }
    rd_usart_ed_send(cBuf, strlen(cBuf));
    rt_thread_mdelay(SEC);
    rt_memset(cBuf, 0, sizeof(cBuf));
    rd_usart_ed_send("AT+Z\n", strlen("AT+Z\n"));
    g_NET_flag = 0;
}
            
/**
 * @brief  sw_Lcd2McuProc
 * @note   处理屏幕协议的回调
 * @param  *_pcDate: 屏幕协议数组
 * @retval None
 */
void sw_Lcd2McuProc(uint8 *_pcDate)
{
    const uint16 Main2Out[MAIN_BUTTON_MAX] = {MAIN_BUTTON_1, MAIN_BUTTON_2, MAIN_BUTTON_3, MAIN_BUTTON_4, 
                                              MAIN_BUTTON_5, MAIN_BUTTON_6, MAIN_BUTTON_7, MAIN_BUTTON_8,
                                              MAIN_BUTTON_9, MAIN_BUTTON_10, MAIN_BUTTON_11, MAIN_BUTTON_12,
                                              MAIN_BUTTON_13, MAIN_BUTTON_14, MAIN_BUTTON_15, MAIN_BUTTON_16,
                                              MAIN_BUTTON_17, MAIN_BUTTON_18, MAIN_BUTTON_19, MAIN_BUTTON_20,
                                              MAIN_BUTTON_21, MAIN_BUTTON_22, MAIN_BUTTON_23, MAIN_BUTTON_24,
                                              MAIN_BUTTON_25, MAIN_BUTTON_26, MAIN_BUTTON_27, MAIN_BUTTON_28,
                                              MAIN_BUTTON_29, MAIN_BUTTON_30, MAIN_BUTTON_31, MAIN_BUTTON_32};
    const uint16 Main2In[2][SUB_LIST_MAX] = {{SUB_INPUT_1, SUB_INPUT_2, SUB_INPUT_3, SUB_INPUT_4,
                                              SUB_INPUT_5, SUB_INPUT_6, SUB_INPUT_7, SUB_INPUT_8, SUB_CLOSE},
                                             {SELECT_INPUT1, SELECT_INPUT2, SELECT_INPUT3, SELECT_INPUT4,
                                              SELECT_INPUT5, SELECT_INPUT6, SELECT_INPUT7, SELECT_INPUT8, SELECT_CLOSE0}};

    const uint16 Oneby1[MAIN_BUTTON_MAX] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16};

    Tlcd2mcu stLcd2mcu = {0};
    Tmcu2lcd stMcu2lcd = {0};

    static uint16 iOutputChn = 0;
    static uint16 iInputChn = 0;
    static Tmcu2lcdEx stmcu2lcdEx[MAIN_BUTTON_MAX] = {0};
    static int iIndex = 0;
    int i = 0;
    int j = 0;
    stMcu2lcd.usHead = FRAME_HEAD;
    stMcu2lcd.ucCmd = CMD_WRITE;

    (void)sw_Big2Little(_pcDate, sizeof(Tlcd2mcu), (void *)&stLcd2mcu, sizeof(Tlcd2mcu), LCD2MCU);
    if (stLcd2mcu.usHead != FRAME_HEAD)
    {
        //rt_kprintf("%s[%d]Head[%o] Error!\r\n", __func__, __LINE__, stLcd2mcu.usHead);
        goto END;
    }
    if (stLcd2mcu.ucCmd != CMD_READ)
    {
        //rt_kprintf("%s[%d]Cmd[%o] Error!\r\n", __func__, __LINE__, stLcd2mcu.ucCmd);
        goto END;
    }
    g_iBlackTim = 0;
    //rt_kprintf("%s[%d]%o, %o, %o, %o, %o\r\n", __func__, __LINE__, stLcd2mcu.ucCmd, stLcd2mcu.ucSize, stLcd2mcu.stAddr.Addr, stLcd2mcu.ucValue, stLcd2mcu.ucValueEx);
    if(stLcd2mcu.ucSize != 8)//通用协议（非设置端口的情况）
    {
        EEventType eEvevtType = sw_GetEventType(stLcd2mcu.stAddr.Addr);
        stMcu2lcd.ucSize = 0x05;
        //rt_kprintf("%s[%d]eEvevtType = %d\r\n", __func__, __LINE__, eEvevtType);
        if (CMD_MAIN_BUTTON == eEvevtType)//按下主    ONEBY1_BUTTON01 = 0x400,
        {
            //rt_kprintf("Golbal:%d\n", stLcd2mcu.stAddr.Addr);
            for (i = 0; i < MAIN_BUTTON_MAX; i++)
            {
                if(stLcd2mcu.stAddr.Addr == Main2Out[i])
                {
                    iIndex = i;
                }
            }
            iOutputChn = stLcd2mcu.stAddr.Addr - 0x100;
            // rt_kprintf("%s[%d]%s[%d]iOutputChn = %o\r\n", __func__, __LINE__, iOutputChn);
        }
        else if (CMD_SUB_BUTTON == eEvevtType)//副菜单点输入按钮
        {   
            //rt_kpCMD_SET_PORTrintf("Golbal:%d\n", stLcd2mcu.ucValue);
            for (i = 0; i < SUB_LIST_MAX; i++)
            {   
                if (stLcd2mcu.ucValue == Main2In[0][i])
                {
                    g_CLKDate[iIndex] = Main2In[1][i];
                    stMcu2lcd.stDateEx.stIcon.usicon = i+1;
                }
            }
            iInputChn = stLcd2mcu.ucValue - 0x100;
            stMcu2lcd.usHead = FRAME_HEAD;
            stMcu2lcd.stAddr.usaddr = 0x1b10;
            //rt_kprintf("%s[%d]iInputChn = %d\r\n", __func__, __LINE__, iInputChn);
            sw_McuSendMsg2Lcd(&stMcu2lcd);
            g_iSame = 1;
        }
        else if (CMD_SUB_SAVE == eEvevtType)//副菜单点击返回（保存）
        {
            if (stLcd2mcu.ucValue == 0x0112)
            {
                stMcu2lcd.stAddr.Addr = iOutputChn;
                stMcu2lcd.stDateEx.stIcon.Icon = iInputChn;
                //rt_kprintf("in = %d, out = %d", iInputChn, iOutputChn);
                sw_McuSendMsg2Lcd(&stMcu2lcd);
                sw_SendCLKDate();
            }
            else if (stLcd2mcu.ucValue == 0x0113)
            {
                
            }
            stMcu2lcd.usHead = FRAME_HEAD;
            stMcu2lcd.stAddr.usaddr = 0x1b10;
            stMcu2lcd.stDateEx.stIcon.usicon = 11;
            sw_McuSendMsg2Lcd(&stMcu2lcd);
        }
        else if (CMD_SUB_ALL == eEvevtType)//副菜单点击全部相同
        {
            if (g_iSame != 0)
            {
                for (i = 0; i < MAIN_BUTTON_MAX; i++)
                {
                    stmcu2lcdEx[i].m_stMcu2Lcd.usHead = FRAME_HEAD;
                    stmcu2lcdEx[i].m_stMcu2Lcd.ucCmd = CMD_WRITE;
                    stmcu2lcdEx[i].m_stMcu2Lcd.ucSize = 0x05;
                    stmcu2lcdEx[i].m_stMcu2Lcd.stAddr.Addr = MainButtonList[1][i] - 0x100;
                    stmcu2lcdEx[i].m_stMcu2Lcd.stAddr.tail = 0;
                    stmcu2lcdEx[i].m_stMcu2Lcd.stDateEx.stIcon.Icon = iInputChn;
                    stmcu2lcdEx[i].m_stMcu2Lcd.stDateEx.stIcon.head = 0;
                    //rt_kprintf("iInputChn = %d\t", iInputChn);
                    g_CLKDate[i] = Main2In[1][iInputChn - 1];
                    //rt_kprintf("g_CLKDate[%d] = %d\n", i, g_CLKDate[i]);
                    sw_McuSendMsg2Lcd(&(stmcu2lcdEx[i].m_stMcu2Lcd));
                }
                sw_SendCLKDate();
                g_iSame = 0;
            }
            stMcu2lcd.usHead = FRAME_HEAD;
            stMcu2lcd.stAddr.usaddr = 0x1b10;
            stMcu2lcd.stDateEx.stIcon.usicon = 11;
            sw_McuSendMsg2Lcd(&stMcu2lcd);
        }
        else if (CMD_ONEBY1 == eEvevtType)//一对一界面点击按钮
        {
            for (i = 0; i < MAIN_BUTTON_MAX; i++)
            {
                stmcu2lcdEx[i].m_iEnable = stLcd2mcu.ucValue;
                stmcu2lcdEx[i].m_stMcu2Lcd.usHead = FRAME_HEAD;
                stmcu2lcdEx[i].m_stMcu2Lcd.ucCmd = CMD_WRITE;
                stmcu2lcdEx[i].m_stMcu2Lcd.ucSize = 0x05;
                stmcu2lcdEx[i].m_stMcu2Lcd.stAddr.Addr = MainButtonList[1][i] - 0x100;
                stmcu2lcdEx[i].m_stMcu2Lcd.stAddr.tail = 0;
                stmcu2lcdEx[i].m_stMcu2Lcd.stDateEx.stIcon.usicon = (i%(SUB_LIST_MAX - 1) + 1);
/*                 if (1 == stLcd2mcu.ucValue)
                { */
                g_CLKDate[Oneby1[i]] = Main2In[1][i%(SUB_LIST_MAX - 1)];
                //rt_kprintf("g_CLKDate[%d] = %d\n", Main2Out[i], g_CLKDate[i]);
/*                 } */
                sw_McuSendMsg2Lcd(&(stmcu2lcdEx[i].m_stMcu2Lcd));
            }
            stMcu2lcd.usHead = FRAME_HEAD;
            stMcu2lcd.stAddr.usaddr = 0x1b10;
            stMcu2lcd.stDateEx.stIcon.usicon = 11;
            sw_McuSendMsg2Lcd(&stMcu2lcd);
            sw_SendCLKDate();
        }
        else if (CMD_CONFIG_DEFAULT == eEvevtType)
        {
            if(stLcd2mcu.ucValue == 1)
            {
                //rt_kprintf("Default!\r\n");
                sw_default_DeviceParam();
                sw_UpdateDevicePara();
            }
        }
        else if (CMD_CONFIG_OK == eEvevtType)
        {
            TDeviceParam *pstDeviceParam = sw_get_DeviceParam();
            Cfg_SetDevicepara(pstDeviceParam);
            //sw_UpdateDevicePara();
/*             g_MAC_flag = 1;
            rd_usart_ed_send("+++", strlen("+++"));
            rt_thread_mdelay(200);
            rd_usart_ed_send("AT+MAC\n", strlen("AT+MAC\n"));
            rt_thread_mdelay(200);
            rd_usart_ed_send("AT+ENTM\n", strlen("AT+ENTM\n"));
            rt_thread_mdelay(SEC);
            g_MAC_flag = 0; */
            sw_SetNetpara2module(pstDeviceParam);
        }
        else if (CMD_CONFIG_ITEM == eEvevtType)
        {
            sw_ConfigItemProc(stLcd2mcu.stAddr.Addr, stLcd2mcu.ucValue);
            sw_UpdateDevicePara();
        }
        else if (CMD_IN_CONIFG == eEvevtType)
        {
            sw_UpdateDevicePara();
        }   
    }
    else//设置端口的协议
    {
        EEventType eEvevtType = sw_GetEventType(stLcd2mcu.stAddr.Addr);
        if (CMD_SET_PORT == eEvevtType)
        {
            sw_ConfigItemProc(stLcd2mcu.stAddr.Addr, RD_BIG_LITTLE_SWAP16(stLcd2mcu.ucValueEx));
            sw_UpdateDevicePara();
        }
    }
END:
    return;
}

/**
 * @brief  sw_NetWorkProc
 * @note   网口协议处理回调
 * @param  *_pcDate: 网口协议数组
 * @retval None
 */
void sw_NetWorkProc(uint8 *_pcDate)
{
    rt_kprintf("%s\r\n", _pcDate);
    Tmcu2lcd stMcu2lcd = {0};
    TMAC lMACValue;

/*     rt_kprintf("%o, %o, %o, %o, %o, %o, %o, %o, %o, %o, %o, %o\r\n", _pcDate[0],
    _pcDate[1],
    _pcDate[2],
    _pcDate[3],
    _pcDate[4],
    _pcDate[5],
    _pcDate[6],
    _pcDate[7],
    _pcDate[8],
    _pcDate[9],
    _pcDate[10],
    _pcDate[11]); */
    const uint16 Oneby1[MAIN_BUTTON_MAX] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16};
    const uint16 Main2In[2][SUB_LIST_MAX] = {{SUB_INPUT_1, SUB_INPUT_2, SUB_INPUT_3, SUB_INPUT_4,
                                              SUB_INPUT_5, SUB_INPUT_6, SUB_INPUT_7, SUB_INPUT_8, SUB_CLOSE},
                                             {SELECT_INPUT1, SELECT_INPUT2, SELECT_INPUT3, SELECT_INPUT4,
                                              SELECT_INPUT5, SELECT_INPUT6, SELECT_INPUT7, SELECT_INPUT8, SELECT_CLOSE0}};
    char section[512] = {0};
    int iOutputChn = 0;
    int iInputChn[32] = {0};
    int i = 0;
    int iDevAddr = g_stDevicePara.m_iDeviceAddr;
    char *p = RT_NULL;
    char cModeonReq[16] = {0};
    char cModeoffReq[16] = {0};
    char cModeonRsp[16] = {0};
    char cModeoffRsp[16] = {0};
    char cSChnHead[16] = {0};
    char cMChnHead[16] = {0};
    char cRemoteMode[16] = {0};
    char cStatus[16] = {0};
    char cDformat[32] = {0};
    char cMformat[256] = {0};
    (void)sprintf(cModeonReq, CTRL_MODEON_REQ, iDevAddr);
    (void)sprintf(cModeoffReq, CTRL_MODEOFF_REQ, iDevAddr);
    (void)sprintf(cModeonRsp, CTRL_MODEON_RSP, iDevAddr);
    (void)sprintf(cModeoffRsp, CTRL_MODEOFF_RSP, iDevAddr);
    (void)sprintf(cSChnHead, SIGNLE_CHN_HEAD_REQ, iDevAddr);
    (void)sprintf(cMChnHead, MLIU_CHN_HEAD_REQ, iDevAddr);
    (void)sprintf(cRemoteMode, GET_REMOTE_MODE_REQ, iDevAddr);
    (void)sprintf(cStatus, GET_STATUS_REQ, iDevAddr);
    (void)sprintf(cDformat, "<%d%s", iDevAddr, SIGNLE_CHN_FORMAT);
    (void)sprintf(cMformat, "<%d%s", iDevAddr, MLIU_CHN_FORMAT);
    //rt_kprintf("========\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", cModeonReq, cModeoffReq, cModeonRsp, cModeoffRsp, cSChnHead, cMChnHead, cRemoteMode, cStatus);
    //rt_kprintf("%s[%d]%s\r\n", __func__, __LINE__, _pcDate);
    if (0 == rt_strcmp(_pcDate, cModeonReq))
    {
        rd_usart_ed_send(cModeonRsp, sizeof(cModeonRsp));
        sw_SetBeepblink();
    }
    else if (0 == rt_strcmp(_pcDate, cModeoffReq))
    {
        rd_usart_ed_send(cModeoffRsp, sizeof(cModeoffRsp));
        sw_SetBeepblink();
    }
    else if ((p = rt_strstr(_pcDate, cSChnHead)) && (p != RT_NULL))
    {
        stMcu2lcd.usHead = FRAME_HEAD;
        stMcu2lcd.ucSize = 0x05;
        stMcu2lcd.ucCmd = CMD_WRITE;
        sscanf(_pcDate, cDformat, &iOutputChn, &iInputChn[0]);
        rt_strcpy(section, _pcDate);
        p = rt_strstr(section, "\r");
        *p = '\n';
        rd_usart_ed_send(section, rt_strlen(section));
        cBuf[40] = (char)iOutputChn;
        cBuf[48] = (char)iInputChn[0];
        //rt_kprintf("CMD:[%s]", _pcDate);
        stMcu2lcd.stAddr.Addr = MainButtonList[1][iOutputChn - 1] - 0x100;
        stMcu2lcd.stDateEx.stIcon.Icon = iInputChn[0];
        sw_McuSendMsg2Lcd(&stMcu2lcd);
        //rt_kprintf("[%d]out = %d, in = %d\r\n", __LINE__, cBuf[40], cBuf[48]);
        rd_usart_os_send(cBuf, sizeof(cBuf));
        g_CLKDate[Oneby1[iOutputChn - 1]] = Main2In[1][iInputChn[0] - 1];
        sw_SendCLKDate();
        sw_SetBeepblink();
    }
    else if ((p = rt_strstr(_pcDate, cMChnHead)) && (p != RT_NULL))
    {
        stMcu2lcd.usHead = FRAME_HEAD;
        stMcu2lcd.ucSize = 0x05;
        stMcu2lcd.ucCmd = CMD_WRITE;
        sscanf(_pcDate, cMformat, &iInputChn[0], &iInputChn[1], &iInputChn[2], &iInputChn[3],
                                         &iInputChn[4], &iInputChn[5], &iInputChn[6], &iInputChn[7],
                                         &iInputChn[8], &iInputChn[9], &iInputChn[10], &iInputChn[11],
                                         &iInputChn[12], &iInputChn[13], &iInputChn[14], &iInputChn[15],
                                         &iInputChn[16], &iInputChn[17], &iInputChn[18], &iInputChn[19],
                                         &iInputChn[20], &iInputChn[21], &iInputChn[22], &iInputChn[23],
                                         &iInputChn[24], &iInputChn[25], &iInputChn[26], &iInputChn[27],
                                         &iInputChn[28], &iInputChn[29], &iInputChn[30], &iInputChn[31]);
        rt_strcpy(section, _pcDate);
        p = rt_strstr(section, "\r");
        *p = '\n';
        rd_usart_ed_send(section, rt_strlen(section));        
        //rt_kprintf("CMD:[%s]", _pcDate);
        for (i = 0; i < 32; i++)
        {
            cBuf[40] = i + 1;
            cBuf[48] = (char)iInputChn[i];
            stMcu2lcd.stAddr.Addr = MainButtonList[1][i] - 0x100;
            stMcu2lcd.stDateEx.stIcon.Icon = iInputChn[i];
            sw_McuSendMsg2Lcd(&stMcu2lcd);
            //rt_kprintf("[%d]out = %d, in = %d\r\n", __LINE__, cBuf[40], cBuf[48]);
            rd_usart_os_send(cBuf, sizeof(cBuf));
#ifdef FOUR_VERSION
            rt_thread_mdelay(1000);
#endif
            g_CLKDate[Oneby1[i]] = Main2In[1][iInputChn[i] - 1];
        }
        sw_SendCLKDate();
        sw_SetBeepblink();
    }
    else if ((p = rt_strstr(_pcDate, cRemoteMode)) && (p != RT_NULL))
    {
        if (CTRL_REMOTE == g_stDevicePara.m_iMode)
        {
            snprintf(section, rt_strlen(section), "%s", CTRL_MODEON_RSP);
        }
        else
        {
            snprintf(section, rt_strlen(section), "%s", CTRL_MODEOFF_RSP);
        }
        rt_kprintf("%s", section);
        rd_usart_ed_send(section, rt_strlen(section));
        sw_SetBeepblink();
    }
    else if ((p = rt_strstr(_pcDate, cStatus)) && (p != RT_NULL))
    {
        int j = 0;
        for (i = 0; i < 32; i++)
        {
            for (j = 0; j < SUB_LIST_MAX; j++)
            {
                if (Main2In[1][j] == g_CLKDate[Oneby1[i]])
                {
                    iInputChn[i] = Main2In[0][j] - 0x100;
                }
            }
        }
        snprintf(section, rt_strlen(section), ">%d/STATUS_ON,CH1_%d,CH2_%d,CH3_%d,CH4_%d,CH5_%d,CH6_%d,CH7_%d,CH8_%d,CH9_%d,CH10_%d,CH11_%d,CH12_%d,CH13_%d,CH14_%d,CH15_%d,CH16_%d,CH17_%d,CH18_%d,CH19_%d,CH20_%d,CH21_%d,CH22_%d,CH23_%d,CH24_%d,CH25_%d,CH26_%d,CH27_%d,CH28_%d,CH29_%d,CH30_%d,CH31_%d,CH32_%d\n", 
                                                       iDevAddr, iInputChn[0], iInputChn[1], iInputChn[2], iInputChn[3], iInputChn[4], iInputChn[5], iInputChn[6], iInputChn[7],
                                                                 iInputChn[8], iInputChn[9], iInputChn[10], iInputChn[11], iInputChn[12], iInputChn[13], iInputChn[14], iInputChn[15], 
                                                                 iInputChn[16], iInputChn[17], iInputChn[18], iInputChn[19], iInputChn[20], iInputChn[21], iInputChn[22], iInputChn[23], 
                                                                 iInputChn[24], iInputChn[25], iInputChn[26], iInputChn[27], iInputChn[28], iInputChn[29], iInputChn[30], iInputChn[31]);
        rd_usart_ed_send(section, rt_strlen(section));
        sw_SetBeepblink();
    }
    else if ((p = rt_strstr(_pcDate, "+OK=")) && (p != RT_NULL) && g_MAC_flag == 1)
    {
        char cBuf[18] = {0x5a, 0xa5, 0x0f, 0x82, 0x17, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        sscanf(_pcDate, "\r\n+OK=%c%c%c%c%c%c%c%c%c%c%c%c\r\n",
            &cBuf[6], &cBuf[7], &cBuf[8], &cBuf[9],
            &cBuf[10], &cBuf[11], &cBuf[12], &cBuf[13],
            &cBuf[14], &cBuf[15], &cBuf[16], &cBuf[17]);
/*         rt_kprintf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\r\n",
            &cBuf[6], &cBuf[7], &cBuf[8], &cBuf[9],
            &cBuf[10], &cBuf[11], &cBuf[12], &cBuf[13],
            &cBuf[14], &cBuf[15], &cBuf[16], &cBuf[17]); */
        rd_usart_sz_send(cBuf, sizeof(cBuf));
    }
}

/**
 * @brief  sw_Init
 * @note   业务逻辑初始化(该函数会在外部main的死循环外调用)
 * @retval None
 */
void sw_Init(void)
{
    if (0 != Cfg_Init(CONFIG_NAME))//配置文件不存在
    {
        sw_default_DeviceParam();
        Cfg_SetDevicepara(sw_get_DeviceParam());
    }
    Cfg_GetDevicepara(sw_get_DeviceParam());
/*         rt_kprintf("%s[%d] %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",__func__,__LINE__ 
        &g_stDevicePara.stNetParam.m_iIPaddr01,
        &g_stDevicePara.stNetParam.m_iIPaddr02,
        &g_stDevicePara.stNetParam.m_iIPaddr03,
        &g_stDevicePara.stNetParam.m_iIPaddr04,
        &g_stDevicePara.stNetParam.m_iNetmask01,
        &g_stDevicePara.stNetParam.m_iNetmask02,
        &g_stDevicePara.stNetParam.m_iNetmask03,
        &g_stDevicePara.stNetParam.m_iNetmask04,
        &g_stDevicePara.stNetParam.m_iNetgate01,
        &g_stDevicePara.stNetParam.m_iNetgate02,
        &g_stDevicePara.stNetParam.m_iNetgate03,
        &g_stDevicePara.stNetParam.m_iNetgate04,
        &g_stDevicePara.stNetParam.m_iPCaddr01,
        &g_stDevicePara.stNetParam.m_iPCaddr02,
        &g_stDevicePara.stNetParam.m_iPCaddr03,
        &g_stDevicePara.stNetParam.m_iPCaddr04,
        &g_stDevicePara.stNetParam.m_iPCport,
        &g_stDevicePara.stNetParam.m_iDeviceport,
        &g_stDevicePara.stNetParam.m_iMode,
        &g_stDevicePara.m_iBitRate,
        &g_stDevicePara.m_iParityCheck,
        &g_stDevicePara.m_iSleepTim,lMACValue.MacAddr[0]
        &g_stDevicePara.m_iMode,
        &g_stDevicePara.m_iDeviceAddr); */
    sw_UpdateDevicePara();
    g_MAC_flag = 1;
    rd_usart_ed_send("+++", strlen("+++"));
    rt_thread_mdelay(200);
    rd_usart_ed_send("AT+MAC\n", strlen("AT+MAC\n"));
    rt_thread_mdelay(200);
    rd_usart_ed_send("AT+ENTM\n", strlen("AT+ENTM\n"));
    rt_thread_mdelay(SEC);
    g_MAC_flag = 0;
    sw_SetNetpara2module(sw_get_DeviceParam());
#ifdef FOUR_VERSION
    rt_pin_mode(GPIO9, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO10, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO11, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO12, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO13, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO14, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO15, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(GPIO16, PIN_MODE_INPUT_PULLDOWN);
#else
    rt_pin_mode(GPIO9, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO10, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO11, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO12, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO13, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO14, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO15, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(GPIO16, PIN_MODE_INPUT_PULLUP);
#endif
    rt_pin_mode(IN1, PIN_MODE_OUTPUT);
    rt_pin_mode(CLK1, PIN_MODE_OUTPUT);
    rt_pin_mode(RCLK1, PIN_MODE_OUTPUT);
    rt_pin_mode(IN2, PIN_MODE_OUTPUT);
    rt_pin_mode(CLK2, PIN_MODE_OUTPUT);
    rt_pin_mode(RCLK2, PIN_MODE_OUTPUT);
    rt_pin_mode(BEEP, PIN_MODE_OUTPUT);
    rt_pin_write(RCLK1, PIN_HIGH);
    rt_pin_write(RCLK2, PIN_HIGH);
    rt_pin_write(CLK1, PIN_LOW);
    rt_pin_write(CLK2, PIN_LOW);
    rt_pin_write(IN1, PIN_HIGH);
    rt_pin_write(IN2, PIN_HIGH);
 }

/**
 * @brief  sw_Proc
 * @note   业务逻辑轮询(该函数会在外部main的死循环内调用)
 * @retval None+ERR=-1

 */
void sw_Proc(void)
{
    int state = 0;
#ifdef FOUR_VERSION
    state = 1;
#endif
    char black[10] = {0x5a, 0xa5, 0x07, 0x82, 0x00, 0x84, 0x5a, 0x01, 0x00, 0x5f};
    g_iBlackTim += 1;
    //rt_kprintf("g_iBlackTim = %d\r\n", g_iBlackTim);
    if(g_iBlackTim >= g_stDevicePara.m_iSleepTim*60)
    {
        g_iBlackTim = 0;
        rd_usart_sz_send(black, sizeof(black));
    }
    if (state == rt_pin_read(GPIO9))
    {
        sw_SetLEDState(ALARM_LED01, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED01, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO10))
    {
        sw_SetLEDState(ALARM_LED02, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED02, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO11))
    {
        sw_SetLEDState(ALARM_LED03, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED03, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO12))
    {
        sw_SetLEDState(ALARM_LED04, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED04, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO13))
    {
        sw_SetLEDState(ALARM_LED05, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED05, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO14))
    {
        sw_SetLEDState(ALARM_LED06, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED06, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO15))
    {
        sw_SetLEDState(ALARM_LED07, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED07, ALARM_OFF);
    }
    if (state == rt_pin_read(GPIO16))
    {
        sw_SetLEDState(ALARM_LED08, ALARM_ON);
    }
    else
    {
        sw_SetLEDState(ALARM_LED08, ALARM_OFF);
    }
    rt_thread_mdelay(1000);  
}
