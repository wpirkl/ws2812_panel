#ifndef UART_DMA_H_
#define UART_DMA_H_

#include <stdbool.h>
#include <stdint.h>


void   usart_dma_open(void);
size_t usart_dma_rx_num(void);
size_t usart_dma_read_until(uint8_t * inBuffer, size_t inMaxNumBytes, uint8_t inCharacter);
size_t usart_dma_read_until_str(uint8_t * inBuffer, size_t inMaxNumBytes, size_t inStartOffset, uint8_t * inString, size_t inStringLength);
bool   usart_dma_peek(uint8_t * inString, size_t inStringLength);
void   usart_dma_rx_skip(size_t inNumberOfCharacters);
size_t usart_dma_read(uint8_t * inBuffer, size_t inMaxNumBytes);
size_t usart_dma_write(uint8_t * inBuffer, size_t inNumBytes);
void   usart_dma_rx_clear(void);
void   usart_dma_rx_wait(void);


#endif /* UART_DMA_H_ */

/* eof */
