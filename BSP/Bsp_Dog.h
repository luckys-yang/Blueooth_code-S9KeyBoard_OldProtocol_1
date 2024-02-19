#ifndef __BSP_DOG_H
#define __BSP_DOG_H

/* Public define==========================================================*/
#define IWDG_LOAD_VALUE 163840

typedef struct
{
    void (*Bsp_Dog_Init)(void); // 看门狗初始化
    void (*Bsp_Dog_FeedDog)(void); // 喂狗
} Bsp_Dog_st;

extern Bsp_Dog_st Bsp_Dog;

#endif
