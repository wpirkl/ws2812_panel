#ifndef UART_DMA_H_
#define UART_DMA_H_

#include <stdbool.h>
#include <stdint.h>

#define USART2_RX_BUFFER_LEN        (1024)


void   usart_dma_open(void);
size_t usart_dma_rx_num(void);
size_t usart_dma_read(uint8_t * inBuffer, size_t inMaxNumBytes);
size_t usart_dma_write(uint8_t * inBuffer, size_t inNumBytes);
void   usart_dma_rx_clear(void);



#endif /* UART_DMA_H_ */

/* eof */
