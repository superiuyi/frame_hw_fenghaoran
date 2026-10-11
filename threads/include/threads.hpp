#ifndef THREADS_HPP
#define THREADS_HPP

#ifdef __cplusplus
extern "C" {
#endif

void ControlThread_Init(void);
void ImuThread_Init(void);
void AliveThread_Init(void);

#ifdef __cplusplus
}
#endif

#endif // THREADS_HPP