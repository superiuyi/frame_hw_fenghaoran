#ifndef THREADS_HPP
#define THREADS_HPP
#include "tx_api.h"          // ← 加这行

#ifdef __cplusplus
extern "C" {
#endif

void ControlThread_Init(void);
void ImuThread_Init(void);
void AliveThread_Init(void);
    extern TX_SEMAPHORE sem_init;
    extern TX_SEMAPHORE sem_imu;
    extern TX_SEMAPHORE sem_control;
#ifdef __cplusplus
}
 
#endif

#endif // THREADS_HPP