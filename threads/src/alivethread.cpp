// threads/src/alivethread.cpp
#include "threads.hpp"
#include "tx_api.h"
#include "main.h"          // 为了 LED 宏
#include "usart.h"        // ← 加:huart6
#include <cstdio>         // ← 加:snprintf

TX_SEMAPHORE sem_init;
TX_SEMAPHORE sem_imu;
TX_SEMAPHORE sem_control;

static TX_THREAD alive_thread;
static UCHAR alive_stack[1024];

static void alive_thread_entry(ULONG input);
void AliveThread_Init()
{
    // 建三个信号量,初始计数都是 0
    tx_semaphore_create(&sem_init,    (CHAR *)"sem_init",    0);
    tx_semaphore_create(&sem_imu,     (CHAR *)"sem_imu",     0);
    tx_semaphore_create(&sem_control, (CHAR *)"sem_control", 0);

    tx_thread_create(&alive_thread, (CHAR *)"alive", alive_thread_entry, 0,
                     alive_stack, sizeof(alive_stack),
                     5, 5, TX_NO_TIME_SLICE, TX_AUTO_START);
}

static void alive_thread_entry(ULONG input)
{
    (void)input;
    // ---- 依次点亮三个灯,各亮 1 秒 ----
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
    tx_thread_sleep(1000);
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
    tx_thread_sleep(1000);

  while (1)
    {
        // ---------- ① 初始化成功了吗 ----------
        bool init_ok = (tx_semaphore_get(&sem_init, TX_NO_WAIT) == TX_SUCCESS);
        if (init_ok)
            tx_semaphore_put(&sem_init);   // ★ 取走了要放回去,保持计数不变
        // ---------- ② imu 活着吗(最多等 200ms) ----------
        bool imu_ok = (tx_semaphore_get(&sem_imu, 200) == TX_SUCCESS);

        // ---------- ③ control 活着吗 ----------
        bool control_ok = (tx_semaphore_get(&sem_control, 200) == TX_SUCCESS);

        // ---------- ④ 灯效 ----------
        // 蓝灯:每 500ms 翻转一次 → 表示 alive 自己在跑
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);

        if (init_ok && imu_ok && control_ok)
        {
            // 一切正常 → 绿灯亮,红灯灭
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
        }
        else
        {
            // 有异常 → 红灯亮,绿灯灭
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
        }

        tx_thread_sleep(500);
    }
}