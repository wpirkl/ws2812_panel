#ifndef UART_DMA_H_
#define UART_DMA_H_

#include <stdbool.h>
#include <stdint.h>

/*!
    Initialize the USART using DMA
*/
void usart_dma_open(void);


/*!
    Get the number of received bytes

    \return the number of entries
*/
size_t usart_dma_rx_num(void);


/*!
    Get a number of bytes from the rx buffer

    This function is nonblocking. It will return 0 if nothing's received

    \param[in]  inBuffer
    \param[in]  inMaxNumBytes

    \retval     The number of bytes read
*/
size_t usart_dma_read(uint8_t * inBuffer, size_t inMaxNumBytes);


/*! Get a number of bytes from the rx buffer without removing them

    This function is nonblocking. It will return 0 if nothing's received

    \param[in]  inBuffer
    \param[in]  inMaxNumBytes

    \retval     The number of bytes read
*/
size_t usart_dma_read_peek(uint8_t * inBuffer, size_t inMaxNumBytes);


/*!
    Get a number of bytes from the rx buffer until a certain character or end of buffer

    This function is nonblocking. It will return 0 if nothing's received

    \param[in]  inBuffer
    \param[in]  inMaxNumBytes   
    \param[in]  inCharacter     The character to stop reading

    \retval     The number of bytes read
*/
size_t usart_dma_read_until(uint8_t * inBuffer, size_t inMaxNumBytes, uint8_t inCharacter);


/*!
    Get a number of bytes from the rx buffer until a certain character or end of buffer

    This function is nonblocking. It will return 0 if nothing's received

    \param[in]  inBuffer
    \param[in]  inMaxNumBytes   
    \param[in]  inStartOffset   Initial offset into the buffer
    \param[in]  inString        The character sequence to compare
    \param[in]  inStringLength  The length of the character sequence to match

    \retval     The number of bytes read
*/
size_t usart_dma_read_until_str(uint8_t * inBuffer, size_t inMaxNumBytes, size_t inStartOffset, const uint8_t * inString, size_t inStringLength);


/*!
    Check if the buffer starts with a certain pattern

    \param[in]  inString        The character sequence to compare
    \param[in]  inStringLength  The length of the character sequence to match

    \retval     true: if the buffer starts with the string, false if it doesn't
*/
bool usart_dma_peek(const uint8_t * inString, size_t inStringLength);
 

/*!
    Check if the buffer ends with a certain pattern

    \param[in]  inString        The character sequence to compare
    \param[in]  inStringLength  The length of the character sequence to match

    \retval     true: if the buffer ends with the string, false if it doesn't
*/
bool usart_dma_peek_end(const uint8_t * inString, size_t inStringLength);


/*!
    Check if the buffer contains a certain pattern

    \param[in]  inString        The character sequence to compare
    \param[in]  inStringLength  The length of the character sequence to match
    \param[out] outLen          The index of the first character after inString

    \retval     true    Found string
    \retval     false   Did not find string
*/
bool usart_dma_match(const uint8_t * inString, size_t inStringLength, size_t * outLen);


/*!
    Skip a number of received characters

    \param[in]  inNumberOfCharacters    The number of characters to skip
*/
void usart_dma_rx_skip(size_t inNumberOfCharacters);


/*!
    Skip everything until a character is found

    \param[in]  inCharacter     The character which was found
*/
void usart_dma_rx_skip_until(uint8_t inCharacter);


/*!
    Send Data on USART using DMA

    This function blocks until all data has been processed

    \param[in]  inBuffer    Buffer to send
    \param[in]  inNumBytes  Number of bytes to send

    \retval The number of bytes actually sent
*/
size_t usart_dma_write(const uint8_t * const inBuffer, size_t inNumBytes);

/*!
    Flush all received characters
*/
void usart_dma_rx_clear(void);

/*!
    Wait until a character arrived
*/
bool usart_dma_rx_wait(void);


#endif /* UART_DMA_H_ */

/* eof */
