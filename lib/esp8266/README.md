# ESP8266

## Usage

### General

In order to use the asynchronous library, you have to spawn several threads:

* RX Handler: `esp8266_rx_handler` should be highest priority of this library. It dispatches incoming asynchronous data.
* Socket Handler: `esp8266_socket_handler` should be lower priority than the RX Handler. It handles incoming server connections.

All other commands from the library can be spawned from separate tasks.
Please note that this library makes **excessive** use of the stack! So a stack size of 256 or more is recommended.

### Server

As the ESP8266 manages only one server (listening), only a single call to `esp8266_cmd_cipserver` is possible.
In case of an incoming connection, the library will spawn a task with the configured priority and stack size. The socket id of the
server connection is passed to the task's callback function.

## Commands check

### With echo

Sent:
```
AT\r\n
```

recv echo:
```
AT\r\r\n
```

recv:
```
\r\nOK\r\n
```

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
```
ATE0\r\n
```

Recv:
```
ATE0\r\r\n\r\nOK\r\n
```

Sent:
```
AT+CWAUTOCONN?\r\n
```

Recv:
```
AT+CWAUTOCONN?\r\r\n
+CWAUTOCONN:1\r\n
OK\r\n
```

Sent:
```
AT+CWAUTOCONN=0\r\n
```

Recv:
```
AT+CWAUTOCONN=0\r\r\n
\r\nOK\r\n
```


### Without echo

Sent:
```
AT\r\n
```

Recv:
```
\r\nOK\r\n
```

Sent:
```
AT+CIPMUX=1\r\n
```

Recv:
```
\r\nOK\r\n
```

Sent:
```
AT+CIPSERVER=1,1001\r\n
```

Recv:
```
\r\nOK\r\n
```

Sent:
```
AT+CIPSEND=0,5\r\n
```

Recv (there's a blank after `>`):
```
\r\nOK\r\n> 
```

Sent:
```
12345
```

Recv:
```
\r\nRecv 5 bytes\r\n\r\nSEND OK\r\n
```

Asynchronous:
```
WIFI DISCONNECT\r\n
WIFI CONNECTED\r\n
WIFI GOT IP\r\n
0,CONNECT\r\n
\r\n+IPD,0,1:a
0,CLOSED\r\n
```
