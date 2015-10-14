
# WS2812

## Configuration

## Connections

| STRIP | PIN | TIMER    | DMA               |
| ----- | --- | -------- | ----------------- |
|     1 | C6  | TIM3 CH1 | DMA1 CH5 Stream 4 |
|     2 | D12 | TIM4 CH1 | DMA1 CH2 Stream 0 |
|     3 | B0  | TIM3 CH3 | DMA1 CH5 Stream 7 |
|     4 | B1  | TIM3 CH4 | DMA1 CH5 Stream 2 |
|     5 | B7  | TIM4 CH2 | DMA1 CH2 Stream 3 |

# ESP8266

## Connections:

| USART | PIN | DMA               | ESP8266 |         |
| ----- | --- | ----------------- | ------- | ------- |
| 2 RX  | A3  | DMA1 CH4 Stream 5 | TX      | Yellow  |
| 2 TX  | A2  | DMA1 CH4 Stream 6 | RX      | Orange  |
| 2 RTS | A1  |                   |         |         |
| 2 CTS | A0  |                   |         |         |

## Commands check

### With echo

Sent:
`AT\r\n`

recv echo:
`AT\r\r\n`

recv:
`\r\nOK\r\n`

```
\r\n+IPD,0,1:h
\r\n+IPD,0,3:elp
```

```
AT+CIPSEND=0,5\r\r\n
\r\n
OK\r\n
> \r\n
Recv 5 bytes\r\n
\r\n
SEND OK\r\n
```

Sent: 
`ATE0\r\n`

Recv:
`ATE0\r\r\n\r\nOK\r\n`

### Without echo

Sent:
`AT\r\n`

Recv:
`\r\nOK\r\n`

Sent:
`AT+CIPMUX=1\r\n`

Recv:
`\r\nOK\r\n`

Sent:
`AT+CIPSERVER=1,1001\r\n`

Recv:
`\r\nOK\r\n`

Sent:
`AT+CIPSEND=0,5\r\n`

Recv:
`\r\nOK\r\n> `

Sent:
`12345`

Recv:
`\r\nRecv 5 bytes\r\n\r\nSEND OK\r\n`

Asynchronous:
```
WIFI DISCONNECT\r\n
WIFI CONNECTED\r\n
WIFI GOT IP\r\n
0,CONNECT\r\n
\r\n+IPD,0,1:a
0,CLOSED\r\n
```

# Eclipse Paho MQTT C/C++ client for Embedded platforms

This repository uses [Eclipse Paho](http://eclipse.org/paho) MQTT C/C++ client library for Embedded platorms.

