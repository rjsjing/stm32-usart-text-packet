#include "stm32f10x.h"                  // 设备头文件
#include "misc.h"
#include <stdio.h>                      // 标准IO，用于printf、sprintf
#include <stdarg.h>                     // 可变参数，用于Serial_Printf


/*
#define TEXT_BUF_SIZE  128  // 文本缓冲区大小，根据需求调整

char     Text_RxBuffer[TEXT_BUF_SIZE];   // 接收缓冲区
volatile uint8_t Text_RxFlag = 0;        // 接收完成标志（供主函数查询）
static uint16_t Text_RxIndex = 0;        // 缓冲区写入位置
*/


uint8_t Serial_RxPacket[100];   // 存放收到的4字节数据包（根据需要修改大小）
volatile uint8_t Serial_RxFlag ;   // 接收标志，1表示收到数据
/**
  * 函    数：Serial_Init
  * 功    能：初始化串口1（TX-PA9，RX-PA10），波特率9600，开启接收中断
  */
void Serial_Init(void)
{
    // 1. 开时钟：USART1 + GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    // 2. 配置GPIO：PA9-TX（复用推挽输出），PA10-RX（输入浮空）
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9; // PA9
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10; // PA10
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 输入浮空
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    // 3. 配置USART：波特率9600，8N1，开启RX和TX
    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate = 9600;                     // 波特率9600
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;    // 8个数据位
    USART_InitStruct.USART_StopBits = USART_StopBits_1;    // 1个停止位
    USART_InitStruct.USART_Parity = USART_Parity_No;     //无奇偶校验
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;   //收发模式
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件流控制
    USART_Init(USART1, &USART_InitStruct);
    // 4. 开启USART1接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
     NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);          // 抢占优先级和子优先级各2位
    // 5. 配置NVIC：USART1中断，优先级1
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;       // USART1中断
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;    // 抢占优先级1
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;    // 子优先级0
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    // 6. 使能USART1
    USART_Cmd(USART1, ENABLE);
}


/**
  * 函    数：Serial_SendByte
  * 功    能：串口发送1个字节
  */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    // 等待发送完成
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);       // 等待发送缓冲区空 发送寄存器空标志位   TXE=1表示发送寄存器空，可以发送下一个字节
}

/**
  * 函    数：Serial_SendArray
  * 功    能：串口发送一个字节数组
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for(i = 0; i < Length; i++)
    {
        Serial_SendByte(Array[i]);
    }
}

/**
  * 函    数：Serial_SendString
  * 功    能：串口发送字符串
  */
void Serial_SendString(char *String)
{
    uint8_t i;
    for(i = 0; String[i] != 0; i++)
    {
        Serial_SendByte(String[i]);
    }
}

/**
  * 函    数：Serial_Pow
  * 功    能：求X的Y次方，给数字发送用
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while(Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
  * 函    数：Serial_SendNumber
  * 功    能：串口发送数字（十进制）
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for(i = 0; i < Length; i++)
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * 函    数：fputc
  * 功    能：重定向printf到串口
  */
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);
    return ch;
}

/**
  * 函    数：Serial_Printf
  * 功    能：串口格式化打印（可变参数，和printf用法一样）
  */
void Serial_Printf(char *format, ...)  // ... 表示可变参数
{
    char String[100];            // 存放最终字符串
    va_list ap;               // 定义参数列表,创建一个 “参数指针”，用来找后面的 ... 参数
    
    va_start(ap, format);     // 开始解析可变参数，告诉系统：从 format 后面开始找可变参数
    vsprintf(String, format, ap); // 把参数格式化进字符串，把所有 ... 参数塞进字符串里
    va_end(ap);               // 结束解析
    
    Serial_SendString(String);   // 发送出去
}

/**
  * 函    数：USART1_IRQHandler
  * 功    能：串口1中断服务函数（收到数据自动进入）
  */
void USART1_IRQHandler(void)
{
	static uint8_t RxState = 0;
	static uint8_t pRxPacket = 0;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
		uint8_t RxData = USART_ReceiveData(USART1);
		switch (RxState)
        {
            case 0:   // 等待包头
                if (RxData == '@' && Serial_RxFlag == 0)
                {
                    RxState = 1;       // 切换到接收数据状态
                    pRxPacket = 0;     // 索引清零，准备存入新包
                }
                
                break;
            case 1:   // 接收数据
				if (RxData == '\r')
				{
					RxState = 2;
				}
				else
				{
					Serial_RxPacket[pRxPacket] = RxData;
					pRxPacket++;
				}
                
                break;
            case 2:   // 等待包尾 
                if (RxData == '\n')
                {
                    Serial_RxFlag = 1; // 通知主循环处理
					Serial_RxPacket[pRxPacket] = '\0';
                    RxState = 0;       // 状态复位，准备下一包
                }
                else
                {
                    // 包尾错误，直接丢弃当前包并复位状态机
                    RxState = 0;
                }
                break;
            default:
                RxState = 0;
                break;
		}
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}



