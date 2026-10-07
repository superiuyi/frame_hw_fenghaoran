/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
TX_THREAD test_thread;                    // 线程控制块
static UCHAR test_stack[1024];            // 给它 1KB 栈

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void test_thread_entry(ULONG thread_input);   // 声明入口函数
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN App_ThreadX_Init */
  (void)byte_pool;
  tx_thread_create(&test_thread,           // ① 控制块地址
                   "test",                 // ② 线程名(调试时能看到)
                   test_thread_entry,      // ③ 入口函数
                   0,                      // ④ 传给入口函数的参数
                   test_stack,             // ⑤ 栈的起始地址
                   sizeof(test_stack),     // ⑥ 栈大小
                   5,                      // ⑦ 优先级(数字小=高)
                   5,                      // ⑧ 抢占阈值(不用时=优先级)
                   TX_NO_TIME_SLICE,       // ⑨ 时间片(不用)
                   TX_AUTO_START);         // ⑩ 创建后立刻启动
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

/**
  * @brief  MX_ThreadX_Init
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */
static void test_thread_entry(ULONG thread_input)
{
    (void)thread_input;          // 参数没用,消掉编译警告
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOH, GPIO_PIN_10);   // 翻转蓝灯
        tx_thread_sleep(500);                      // 睡 500 个 tick = 500ms
    }
}
/* USER CODE END 1 */
