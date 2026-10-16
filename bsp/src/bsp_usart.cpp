#include "bsp_usart.hpp"
#include "usart.h"
static uint8_t rx1_buf[64];         // USART1 的接收缓冲,64 字节
static uint8_t rx6_buf[64];         // USART6 的接收缓冲

void USART_Init(void)
{
  USART1_Init();
  USART6_Init();
}

void USART6_Init()
{
  // TODO: 配置 USART6 的 DMA 接收、发送及空闲中断，并启动接收。
   HAL_UART_Receive_DMA(&huart6, rx6_buf, sizeof(rx6_buf));
    //                    ↑       ↑        ↑
    //                USART6句柄 收进哪  收多少(=64)
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    //                    ↑            ↑
    //                 USART6句柄   开空闲中断
};

void USART1_Init()
{
  // TODO: 配置 USART1 的 DMA 接收、发送及空闲中断，并启动接收。
  HAL_UART_Receive_DMA(&huart1, rx1_buf, sizeof(rx1_buf));
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

};

void USART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, enum USART_Mode mode)
{
  // TODO: 检查参数，并按 mode 选择阻塞、DMA 或中断发送。
  (void)huart;
  (void)pData;
  (void)Size;
  (void)mode;
      if (huart == nullptr || pData == nullptr || Size == 0) return;
    switch (mode)
    {
        case USART_MODE_BLOCK:                              // 阻塞
            HAL_UART_Transmit(huart, pData, Size, 100);     // 最后是超时 100ms
            break;
        case USART_MODE_DMA:                                // DMA
            HAL_UART_Transmit_DMA(huart, pData, Size);      // 没有超时参数
            break;
        case USART_MODE_IT:                                 // 中断
            HAL_UART_Transmit_IT(huart, pData, Size);       // 没有超时参数
            break;
    }
}

void USART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
  // TODO: 选择接收方式并启动 HAL UART 接收。
  (void)huart;
  (void)pData;
  (void)Size;
   if (huart == nullptr || pData == nullptr || Size == 0) return;
    HAL_UART_Receive_DMA(huart, pData, Size);
}
