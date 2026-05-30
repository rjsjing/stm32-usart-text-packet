# CLAUDE.md — STM32F103C8T6 USART文本数据包收发

## 技术栈

- **MCU**: STM32F103C8T6（Cortex-M3, 64KB Flash, 20KB SRAM, 72MHz）
- **库**: ST 标准外设库 v3.5.0
- **构建工具**: Keil MDK uVision5 + EIDE (VSCode 插件 `cl.eide`)
- **调试器**: ST-Link (SWD)

## 目录结构

```
├── Start/           CMSIS 启动 + system_stm32f10x
├── Library/         ST 标准外设库 v3.5.0
├── System/          Delay.c/h — SysTick 延时
├── Hardwera/        Serial.c/h — USART1 文本命令协议
│                    LED.c/h — LED 驱动
│                    OLED.c/h — OLED 显示
├── User/            main.c, stm32f10x_conf.h, stm32f10x_it.c/h
└── build/Objects/Listings/
```

## 硬件引脚映射

| 引脚 | 功能 | 连接目标 |
|------|------|----------|
| PA9 | USART1_TX AF PP | 串口 TX → PC/CH340 |
| PA10 | USART1_RX Floating | 串口 RX ← PC/CH340 |
| PA1 | GPIO Out PP | LED1 |
| PB5/PB6 | GPIO Out OD | OLED SCL/SDA |

## Serial 文本数据包协议 (Hardwera/Serial.c/h)

### 协议格式
- 文本命令以 `\r\n` 结尾识别为一包
- 全局接收缓冲区: `char Serial_RxPacket[]`
- 接收完成标志: `Serial_RxFlag`

### 支持命令
| 命令 | 响应 | 操作 |
|------|------|------|
| `LED_ON` | `LED_ON_OK\r\n` | 点亮 LED1 |
| `LED_OFF` | `LED_OFF_OK\r\n` | 关闭 LED1 |
| 其他 | `ERROR_COMMAND\r\n` | 无效命令 |

### API
- `Serial_Init()` — 初始化 USART1 TX/RX + 中断接收
- `Serial_SendString(*String)` — 发送字符串
- 中断 ISR 自动填充 Serial_RxPacket (以 `\r\n` 分帧)

## 功能说明

PC 通过串口助手发送文本命令 (`LED_ON\r\n`, `LED_OFF\r\n`) 控制 STM32 板载 LED1。命令识别使用 strcmp，响应通过 Serial_SendString 回复。OLED 显示收到的命令和响应结果。

## 编码规范
- 编码: UTF-8，注释用中文

## Git 规范
```
main              ← 稳定分支
提交: feat:/fix:/docs: 格式
```

## 构建命令
```
Keil:  打开 Project.uvprojx → F7 编译 → F8 烧录
EIDE:  Ctrl+Shift+P → "EIDE: Build"
```

## 注意事项
- `USE_STDPERIPH_DRIVER` + `STM32F10X_MD` 必须定义
- 文本命令必须以 `\r\n` 结尾
- 命令识别使用 strcmp(), 大小写敏感
- 需在串口助手设置 `发送新行: \r\n`
