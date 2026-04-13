#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#include "main.h"

// 调试总开关：1=开启，0=关闭（比赛时改为0）
#define DEBUG_ENABLE    1   

#if DEBUG_ENABLE
    // 开启时，DEBUG_PRINT 等同于 printf
    #define DEBUG_PRINT(...)  printf(__VA_ARGS__)
    #define DEBUG_LOG(tag, format, ...) printf("[%s] " format "\r\n", tag, ##__VA_ARGS__)
#else
    // 关闭时，编译为空，不占代码，不占时间
    #define DEBUG_PRINT(...)
    #define DEBUG_LOG(tag, format, ...)
#endif

void Debug_Init(void);

#endif


