
# WS2812

## Configuration

* **172** LEDs in a column
* **5** rows, each row has it's own PWM output
* **DMA** driven
* **50 A** on 5V :exclamation:

## Connections

| STRIP | PIN | TIMER    | DMA               | Comment          |
| ----- | --- | -------- | ----------------- | ---------------- |
|     1 | C6  | TIM3 CH1 | DMA1 CH5 Stream 4 |                  |
|     2 | D12 | TIM4 CH1 | DMA1 CH2 Stream 0 | LD4 in parallel! |
|     3 | B0  | TIM3 CH3 | DMA1 CH5 Stream 7 |                  |
|     4 | B1  | TIM3 CH4 | DMA1 CH5 Stream 2 |                  |
|     5 | B7  | TIM4 CH2 | DMA1 CH2 Stream 3 |                  |

# ESP8266

## Connections:

| USART | PIN | DMA               | ESP8266 |         |
| ----- | --- | ----------------- | ------- | ------- |
| 2 RX  | A3  | DMA1 CH4 Stream 5 | TX      | Yellow  |
| 2 TX  | A2  | DMA1 CH4 Stream 6 | RX      | Orange  |
| 2 RTS | A1  |                   |         |         |
| 2 CTS | A0  |                   |         |         |

# Web server

There's a light whight http server included to configure and pilot the LED panel.

# Eclipse Paho MQTT C/C++ client for Embedded platforms

## Subscriptions

### Blue Led test

Test to turn blue led on and off:

`FreeRTOS/sample/wep/blueled ON`

`FreeRTOS/sample/wep/blueled OFF`

## Publish

Nothing yet

# Related articles and code

- Benjamin's robotics [USB-Serial on STM32F4](http://vedder.se/2012/07/usb-serial-on-stm32f4/)
- UB [78-WS2812-Library](http://mikrocontroller.bplaced.net/wordpress/?page_id=3665)
- UB [88-WS2812_8CH-Library](http://mikrocontroller.bplaced.net/wordpress/?page_id=4204)
- [Eclipse Paho](http://eclipse.org/paho) MQTT C/C++ client library for Embedded platorms.
- [FreeRTOS](http://www.freertos.org/)
