#ifndef RDCOMMON_H

#define RDCOMMON_H

typedef unsigned char                 uint8;     /**<  8bit unsigned integer type */
typedef unsigned short                uint16;    /**< 16bit unsigned integer type */
typedef unsigned int                  uint32;    /**< 32bit unsigned integer type */
typedef unsigned long long            uint64;    /**< 64bit unsigned integer type */
//下方为大小端转换宏
#define RD_BIG_LITTLE_SWAP16(x)        ( (((*(unsigned short int *)&x) & 0xff00) >> 8) | \
                                      (((*(unsigned short int *)&x) & 0x00ff) << 8) )
 
#define RD_BIG_LITTLE_SWAP32(x)        ( (((*(unsigned long int *)&x) & 0xff000000) >> 24) | \
                                      (((*(unsigned long int *)&x) & 0x00ff0000) >> 8) | \
                                      (((*(unsigned long int *)&x) & 0x0000ff00) << 8) | \
                                      (((*(unsigned long int *)&x) & 0x000000ff) << 24) )

#define RD_BIG_LITTLE_SWAP64(x)        ( (((*(unsigned long long int *)&x) & 0xff00000000000000) >> 56) | \
                                      (((*(unsigned long long int *)&x) & 0x00ff000000000000) >> 40) | \
                                      (((*(unsigned long long int *)&x) & 0x0000ff0000000000) >> 24) | \
                                      (((*(unsigned long long int *)&x) & 0x000000ff00000000) >> 8) | \
                                      (((*(unsigned long long int *)&x) & 0x00000000ff000000) << 8) | \
                                      (((*(unsigned long long int *)&x) & 0x0000000000ff0000) << 24) | \
                                      (((*(unsigned long long int *)&x) & 0x000000000000ff00) << 40) | \
                                      (((*(unsigned long long int *)&x) & 0x00000000000000ff) << 56) )

void udelay(unsigned long usecs);

#endif /* end of include guard: RDCOMMON_H */

