#include "config.h"
#include <dfs_posix.h>

/**
 * @brief  Cfg_Init
 * @note   初始化配置文件，若文件不存在，则会创建对应配置文件
 * @param  *_Filename: 文件绝对路径
 * @retval 为0表示文件存在，否则表示不存在
 */
int Cfg_Init(char *_Filename)
{
    int fd;
    int iRet = 0;
    struct stat state = {0};
    iRet = stat(_Filename, &state);
    if (iRet != 0)
    {
        fd = open(_Filename, O_RDWR | O_APPEND | O_CREAT, 0);
        if (fd < 0)
        {
            rt_kprintf("creat file: %s failed!\r\n", _Filename);
            goto END;
        }
    }
END:
    close(fd);
    return iRet;
}

#ifdef BSP_USING_OTA_DOWNLOADER
#include <fal.h>
static int rd_Fal_Init(void)
{
    fal_init();
    return 0;
}
INIT_APP_EXPORT(rd_Fal_Init);
#endif
#ifdef BSP_USING_SPI_FLASH_LITTLEFS
#include <fal.h>
#include <dfs_fs.h>
#include <dfs_posix.h>          
#define FS_PARTITION_NAME              "spiflash"
static int rd_init_spifs(void)
{
    struct rt_device *mtd_dev = RT_NULL;
    /* 初始化 fal */
    fal_init();
    /* 生成 mtd 设备 */
    mtd_dev = fal_mtd_nor_device_create(FS_PARTITION_NAME);
    if (!mtd_dev)
    {
        /* LOG_E("Can't create a mtd device on '%s' partition.", FS_PARTITION_NAME); */
    }
    else
    {
        /* 挂载 littlefs */
        dfs_mount(FS_PARTITION_NAME, "/", "lfs", 0, 0);
    }
    return RT_EOK;
}
INIT_APP_EXPORT(rd_init_spifs);

#define BUF_SIZE 20
int file_test(int argc, char**argv)
{
    int iRet = 0;
    int fd, fd2;
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    rt_kprintf("%s\n", argv[2]);
    if (argc != 3) 
    {
        rt_kprintf("Useage Error!\n");    
        iRet = -1;
        goto END;
    }
    fd = open(argv[1], O_CREAT | O_WRONLY);
    if (-1 == fd) 
    {
        rt_kprintf("open file %s fail!\n", argv[1]);
        iRet = -1;
        goto CLOSE;
    }
    iRet = write(fd, argv[2], strlen(argv[2]));
    if (iRet != strlen(argv[2])) 
    {
        rt_kprintf("write error!\n");
        iRet = -1;
        goto CLOSE;
    }
    fd2 = open(argv[1], O_RDONLY);
    iRet = read(fd2, buf, sizeof(buf));
    if (-1 == iRet)
    {
        rt_kprintf("read error!\n");
        iRet = -1;
        goto CLOSE;
    }
    rt_kprintf("buf : %s\n", buf);
CLOSE:
    close(fd);
    close(fd2);
END:
    return iRet;
}
MSH_CMD_EXPORT(file_test, a file test);
#endif
