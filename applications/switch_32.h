#ifndef SWITCH_32_H

#define SWITCH_32_H

#include "rdcommon.h"

#define SUB_LIST_MAX 9
#define MAIN_BUTTON_MAX 32
#define SEC 100

#define CONFIG_NAME "/spiflash/config.ini"
#define CMD_SELCT_COLOR 0x1353
#define FRAME_HEAD 0x5AA5

#define GPIO9     GET_PIN(A, 12)
#define GPIO10    GET_PIN(A, 11)
#define GPIO11    GET_PIN(A, 8)
#define GPIO12    GET_PIN(C, 9)
#define GPIO13    GET_PIN(C, 8)
#define GPIO14    GET_PIN(C, 7)
#define GPIO15    GET_PIN(C, 6)
#define GPIO16    GET_PIN(G, 8)
#define IN1       GET_PIN(C, 13)
#define RCLK1     GET_PIN(E, 6)
#define CLK1      GET_PIN(E, 5)
#define IN2       GET_PIN(E, 4)
#define RCLK2     GET_PIN(B, 9)
#define CLK2      GET_PIN(B, 8)
#define BEEP      GET_PIN(B, 12)
#define CLK_COUNT_MAX 16

#define CTRL_MODEON_REQ "<%d/REM_ON\r"
#define CTRL_MODEOFF_REQ "<%d/REM_OFF\r"
#define SIGNLE_CHN_HEAD_REQ "<%d/D"
#define SIGNLE_CHN_FORMAT "/D%02d_%02d\r"
#define MLIU_CHN_HEAD_REQ "<%d/M"
#define MLIU_CHN_FORMAT "/M%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d%02d\r"
#define GET_REMOTE_MODE_REQ "<%d/REM_?\r"
#define GET_STATUS_REQ "<%d/STATUS_?\r"
#define SET_MANUFACTURE_INFO "<%d/WHAT="
#define GET_MANUFACTURE_INFO "<%d/WHAT_?"

#define CTRL_MODEON_RSP ">%d/REM_ON\n" 
#define CTRL_MODEOFF_RSP ">%d/REM_OFF\n" 
#define SIGNLE_CHN_HEAD_RSP ">%d/D"
#define MLIU_CHN_HEAD_RSP ">%d/M"
#define GET_STATUS_HEAD_RSP ">%d/STATUS_ON"
#define OUTPUT_STATUS_ON_RSP ">%d/STATUS_ON,CH1_%d,CH2_%d,CH3_%d,CH4_%d,CH5_%d,CH6_%d,CH7_%d,CH8_%d,CH9_%d,CH10_%d,CH11_%d,CH12_%d,CH13_%d,CH14_%d,CH15_%d,CH16_%d,CH17_%d,CH18_%d,CH19_%d,CH20_%d,CH21_%d,CH22_%d,CH23_%d,CH24_%d,CH25_%d,CH26_%d,CH27_%d,CH28_%d,CH29_%d,CH30_%d,CH31_%d,CH32_%d\n"
#define OUTPUT_STATUS_OFF_RSP ">%d/STATUS_OFF,CH1_%d,CH2_%d,CH3_%d,CH4_%d,CH5_%d,CH6_%d,CH7_%d,CH8_%d,CH9_%d,CH10_%d,CH11_%d,CH12_%d,CH13_%d,CH14_%d,CH15_%d,CH16_%d,CH17_%d,CH18_%d,CH19_%d,CH20_%d,CH21_%d,CH22_%d,CH23_%d,CH24_%d,CH25_%d,CH26_%d,CH27_%d,CH28_%d,CH29_%d,CH30_%d,CH31_%d,CH32_%d\n"
#define SET_MANUFACTURE_RSP ">%d/WHAT=%s\n"
#define GET_MANUFACTURE_RSP ">%d/WHAT=%s\n"

typedef union { 
    uint16 usaddr;
    struct {
        uint16 tail:4;//stm32为小端存储，位域需要反着写，此处为尾部
        uint16 Addr:12;//stm32为小端存储，位域需要反着写，此处为头部
    }__attribute__((packed));
}TAddr;

typedef union { 
    uint16 usicon;
    struct {
        uint16 Icon:12;
        uint16 head:4;//stm32为小端存储，位域需要反着写，此处为头部
    }__attribute__((packed));
}TIcon;

typedef union { 
    uint8 MacAddr[8];//存放MAC地址
    uint64 lMACValue;
}TMAC;

typedef union { 
    uint8 MacAddr[20];//存放MAC地址
    struct {
        TIcon stIcon;//图标对应值，详见EIcon
        uint8 ucNull[18];//预留
    }__attribute__((packed));
    struct {
        uint16 stvalue;
        uint16 stvaluex;
        uint8 Null[16];
    }__attribute__((packed));
}TExDate;

typedef union {
    uint8 ucDate[12];
    struct {
        uint16 usHead;//帧头,默认为5AA5
        uint8 ucSize; //数据长度
        uint8 ucCmd;  //命令，详见ECmd，一般为读和写
        TAddr stAddr; //变量地址，读命令时，地址详见EButtonAddr
        uint8 ucRet;  //返回数据字长度，暂时未关注
        uint16 ucValue;//按键值，参考EButtonAddr中对成员的描述
        uint16 ucValueEx;//键值扩展字段，大部分情况不关注设置端口时使用
        uint8 ucNull;//预留
    }__attribute__((packed));
} Tlcd2mcu;

typedef union {
    uint8 ucDate[26];
    struct {
        uint16 usHead;//帧头,默认为5AA5
        uint8 ucSize; //数据长度
        uint8 ucCmd;  //命令，详见ECmd，一般为读和写
        TAddr stAddr; //变量地址，读命令时，地址详见ELabelAddr
        TExDate stDateEx;
    }__attribute__((packed));
} Tmcu2lcd;

typedef struct {
    int m_iEnable;
    Tmcu2lcd m_stMcu2Lcd;
} Tmcu2lcdEx;


typedef enum {
    MCU2LCD = 0,
    LCD2MCU,
} EFlagBL;

typedef enum {
    CMD_NULL = 0,
    CMD_MAIN_BUTTON,//主菜单按下32个输出按钮
    CMD_SUB_BUTTON,//副菜单按下输入后
    CMD_SUB_SAVE,//副菜单点击返回
    CMD_SUB_ALL,//副菜单点击全部相同
    CMD_ONEBY1,//选择一对一的某一个按钮后
    CMD_ONEBY1_OK,//一对一界面点确定
    CMD_CONFIG_DEFAULT,//参数界面点击恢复默认值
    CMD_CONFIG_OK,//参数界面点击OK
    CMD_CONFIG_ITEM,//点击某一个参数配置控件
    CMD_IN_CONIFG,//从主菜单进入配置界面
    CMD_SET_PORT//设置网络端口
} EEventType;

typedef enum {
    CMD_READ = 0x83,//读命令(屏幕发给MCU)
    CMD_WRITE = 0x82,//写命令(MCU发给屏幕)    
} ECmd;

typedef enum {
    ALARM_ON = 0x001,//告警状态
    ALARM_OFF = 0x000,//正常状态    
} ELedState;

typedef enum {
    SUB_CLOSE = 0x0100, //按关闭  
    SUB_INPUT_1, //按01，下同
    SUB_INPUT_2,
    SUB_INPUT_3,
    SUB_INPUT_4,
    SUB_INPUT_5,
    SUB_INPUT_6,
    SUB_INPUT_7,
    SUB_INPUT_8,
} ESubButton;

typedef enum {
    ONEBY1_1 = 0x0301, //输出1,往下类似直到输出32 
    ONEBY1_2, 
    ONEBY1_3,
    ONEBY1_4, 
    ONEBY1_5, 
    ONEBY1_6,
    ONEBY1_7, 
    ONEBY1_8, 
    ONEBY1_9,
    ONEBY1_10 = 0x0310,
    ONEBY1_11,
    ONEBY1_12,     
    ONEBY1_13,     
    ONEBY1_14,     
    ONEBY1_15,     
    ONEBY1_16,     
    ONEBY1_17,     
    ONEBY1_18,     
    ONEBY1_19,     
    ONEBY1_20 = 0x0320,
    ONEBY1_21, 
    ONEBY1_22, 
    ONEBY1_23, 
    ONEBY1_24, 
    ONEBY1_25, 
    ONEBY1_26, 
    ONEBY1_27, 
    ONEBY1_28, 
    ONEBY1_29, 
    ONEBY1_30 = 0x0330, 
    ONEBY1_31, 
    ONEBY1_32,  
} EOneByOne;

typedef enum {
    MAIN_BUTTON_1_EX= 0x201, //输出1,往下类似直到输出32 
    MAIN_BUTTON_2_EX, 
    MAIN_BUTTON_3_EX, 
    MAIN_BUTTON_4_EX, 
    MAIN_BUTTON_5_EX,
    MAIN_BUTTON_6_EX, 
    MAIN_BUTTON_7_EX, 
    MAIN_BUTTON_8_EX,
    MAIN_BUTTON_9_EX,
    MAIN_BUTTON_10_EX = 0x210,
    MAIN_BUTTON_11_EX,
    MAIN_BUTTON_12_EX,     
    MAIN_BUTTON_13_EX,     
    MAIN_BUTTON_14_EX,     
    MAIN_BUTTON_15_EX,     
    MAIN_BUTTON_16_EX,     
    MAIN_BUTTON_17_EX,     
    MAIN_BUTTON_18_EX,     
    MAIN_BUTTON_19_EX,     
    MAIN_BUTTON_20_EX = 0x220,
    MAIN_BUTTON_21_EX, 
    MAIN_BUTTON_22_EX, 
    MAIN_BUTTON_23_EX, 
    MAIN_BUTTON_24_EX,
    MAIN_BUTTON_25_EX, 
    MAIN_BUTTON_26_EX, 
    MAIN_BUTTON_27_EX, 
    MAIN_BUTTON_28_EX, 
    MAIN_BUTTON_29_EX, 
    MAIN_BUTTON_30_EX = 0x230, 
    MAIN_BUTTON_31_EX, 
    MAIN_BUTTON_32_EX, 
} EOneBy1;

typedef enum {
    MAIN_BUTTON_16 = 0x201, //输出1,往下类似直到输出32 
    MAIN_BUTTON_15, 
    MAIN_BUTTON_14, 
    MAIN_BUTTON_13, 
    MAIN_BUTTON_12,
    MAIN_BUTTON_11, 
    MAIN_BUTTON_10, 
    MAIN_BUTTON_9, 
    MAIN_BUTTON_8,
    MAIN_BUTTON_7 = 0x210,
    MAIN_BUTTON_6,
    MAIN_BUTTON_5,     
    MAIN_BUTTON_4,     
    MAIN_BUTTON_3,     
    MAIN_BUTTON_2,     
    MAIN_BUTTON_1,     
    MAIN_BUTTON_32,     
    MAIN_BUTTON_31,     
    MAIN_BUTTON_30,     
    MAIN_BUTTON_29 = 0x220,
    MAIN_BUTTON_28, 
    MAIN_BUTTON_27, 
    MAIN_BUTTON_26, 
    MAIN_BUTTON_25,
    MAIN_BUTTON_24, 
    MAIN_BUTTON_23, 
    MAIN_BUTTON_22, 
    MAIN_BUTTON_21, 
    MAIN_BUTTON_20, 
    MAIN_BUTTON_19 = 0x230, 
    MAIN_BUTTON_18, 
    MAIN_BUTTON_17, 
    MAIN_BUTTON_STATE,//进入告警状态界面
    MAIN_BUTTON_SETTING,//进入参数设置界面
    SUB_BUTTON_GROUP = 0x251,//子界面按钮组的地址，键值详见ESubButton
    SUB_BUTTON_ALL = 0x260, //全部相同
    SUB_BUTTON_1BY1, //一对一界面
    SUB_BUTTON_OK, //子界面返回
    ONEBY1_BUTTON_GROUP = 0x301, //一对一界面按键组，键值详见EOneByOne
    ONEBY1_OK = 0x333,//一对一界面确认
    ONEBY1_BACK,//一对一界面返回
    ONEBY1_BUTTON01 = 0x400,
    ONEBY1_BUTTON02 = 0x402, 
    ONEBY1_BUTTON03 = 0x404, 
    ONEBY1_BUTTON04 = 0x406, 
    ONEBY1_BUTTON05 = 0x408, 
    ONEBY1_BUTTON06 = 0x410, 
    ONEBY1_BUTTON07 = 0x412, 
    ONEBY1_BUTTON08 = 0x414, 
    ONEBY1_BUTTON09 = 0x416, 
    ONEBY1_BUTTON10 = 0x418, 
    ONEBY1_BUTTON11 = 0x420, 
    ONEBY1_BUTTON12 = 0x422, 
    ONEBY1_BUTTON13 = 0x424, 
    ONEBY1_BUTTON14 = 0x426, 
    ONEBY1_BUTTON15 = 0x428, 
    ONEBY1_BUTTON16 = 0x430, 
    ONEBY1_BUTTON17 = 0x432,
    ONEBY1_BUTTON18 = 0x434, 
    ONEBY1_BUTTON19 = 0x436, 
    ONEBY1_BUTTON20 = 0x438, 
    ONEBY1_BUTTON21 = 0x440, 
    ONEBY1_BUTTON22 = 0x442, 
    ONEBY1_BUTTON23 = 0x444, 
    ONEBY1_BUTTON24 = 0x446, 
    ONEBY1_BUTTON25 = 0x448, 
    ONEBY1_BUTTON26 = 0x450, 
    ONEBY1_BUTTON27 = 0x452, 
    ONEBY1_BUTTON28 = 0x454, 
    ONEBY1_BUTTON29 = 0x456, 
    ONEBY1_BUTTON30 = 0x458, 
    ONEBY1_BUTTON31 = 0x460, 
    ONEBY1_BUTTON32 = 0x462, 
    CONFIG_DEFAULT = 0x620,//配置界面恢复默认按钮
    CONFIG_BACK = 0x600,//配置界面返回按钮
} EButtonAddr;//按钮控件地址值

typedef enum {
    SUB_LEDCLOSE = 0x168,
    CONFIG_LABEL_MAC = 0x170,//配置界面显示MAC地址
    ALARM_LED01 = 0x1D1,//灯状态详见ELedState
    ALARM_LED02,
    ALARM_LED03,
    ALARM_LED04,
    ALARM_LED05,
    ALARM_LED06,
    ALARM_LED07,
    ALARM_LED08
} ELabelAddr;//显示控件地址值

typedef enum {
    EDIT_LOCAL_IP01 = 0x500,
    EDIT_LOCAL_IP02 = 0x502,
    EDIT_LOCAL_IP03 = 0x504,
    EDIT_LOCAL_IP04 = 0x506,
    EDIT_NETMASK01 = 0x508,
    EDIT_NETMASK02 = 0x510,
    EDIT_NETMASK03 = 0x512,
    EDIT_NETMASK04 = 0x514,
    EDIT_REMOTE_IP01 = 0x516,
    EDIT_REMOTE_IP02 = 0x518,
    EDIT_REMOTE_IP03 = 0x520,
    EDIT_REMOTE_IP04 = 0x522,  
    EDIT_NETGATE01 = 0x524,
    EDIT_NETGATE02 = 0x526,
    EDIT_NETGATE03 = 0x528,
    EDIT_NETGATE04 = 0x530, 
    EDIT_LOCAL_PORT = 0x568,
    EDIT_REMOTE_PORT = 0x572,
    EDIT_SLEEP_TIME = 0x574,
    EDIT_DEVICE_ADDR = 0x532,
    EDIT_MODE = 0xA50,//模式，可选值详见EMode
    EDIT_CTRL = 0x700,//遥控，可选值详见ECtrl
    EDIT_BIT_RATE = 0x610,//波特率，可选值详见EBitRate
    EDIT_PARITY_CHECK = 0x650,//奇偶校验，可选值详见EParityCheck
} EEditAddr; //编辑框控件地址

typedef enum {
    MODE_TCPSER = 0x0000,//TCP服务器
    MODE_UDP,//TCP客户端
    MODE_TCPCLI,//UDP
} EMode;

typedef enum {
    CTRL_LOCAL = 0x0001,//本地
    CTRL_REMOTE,//遥控
} ECtrl;

typedef enum {
    BIT_RATE_1200 = 0x0001,
    BIT_RATE_2400,
    BIT_RATE_4800,
    BIT_RATE_9600,
    BIT_RATE_115200,
} EBitRate;

typedef enum {
    BIT8_ON_CHECK = 0x0001,//八位无校验
    BIT8_ODD_CHECK,//八位奇校验
    BIT8_EVEN_CHECK,//八位偶校验
    BIT7_ODD_CHECK,//七位奇校验
    BIT7_EVEN_CHECK,//七位偶校验
} EParityCheck;

typedef enum {
    RED   = 0xF800,//通用红色
    BLACK = 0x0000,//通用黑色
    BLUE  = 0x3A38,//通用蓝色
} EObjColor;

typedef struct {
    int m_iIPaddr01;    
    int m_iIPaddr02;    
    int m_iIPaddr03;    
    int m_iIPaddr04;
    int m_iNetmask01;    
    int m_iNetmask02;    
    int m_iNetmask03;    
    int m_iNetmask04;
    int m_iPCaddr01;
    int m_iPCaddr02;
    int m_iPCaddr03;
    int m_iPCaddr04;
    int m_iNetgate01;    
    int m_iNetgate02;    
    int m_iNetgate03;    
    int m_iNetgate04;
    int m_iDeviceport;
    int m_iPCport;
    int m_iMode;
    int m_iNull;
}__attribute__((packed)) TNetParam;


typedef struct {
    TNetParam stNetParam;
    int m_iBitRate;
    int m_iParityCheck;
    int m_iDeviceAddr;
    int m_iSleepTim;
    int m_iMode;
    char Manu[32];
    int m_iNull[3];
}__attribute__((packed)) TDeviceParam;

typedef enum {
    SELECT_INPUT1 = 0b1000000100000000,
    SELECT_INPUT2 = 0b0100000100000000,
    SELECT_INPUT3 = 0b0010000100000000,
    SELECT_INPUT4 = 0b0001000100000000,
    SELECT_INPUT5 = 0b0000000010000001,
    SELECT_INPUT6 = 0b0000000001000001,
    SELECT_INPUT7 = 0b0000000000100001,
    SELECT_INPUT8 = 0b0000000000010001,
    SELECT_CLOSE0 = 0b0000000000000000,
} EOutPut;

//对外接口
void sw_NetWorkProc(uint8 *_pcDate);
void sw_Lcd2McuProc(uint8 *_pcDate);
void sw_Init(void);
void sw_Proc(void);

#endif /* end of include guard: SWITCH_32_H */
