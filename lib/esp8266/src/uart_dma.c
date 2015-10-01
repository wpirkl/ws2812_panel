/*!
    \file       uart_dma.c
    \author     Werner Pirkl
    \version    1.0
    \date       25-August-2015

    This file provides a driver which adds a simple ring-buffer around
    usart2 by using DMA in circular mode.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// for debug
#include <stdio.h>

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_rcc.h"
#include "misc.h"

#include "uart_dma.h"

#define USART2_RX_BUFFER_LEN        (1024)

#define USART_DMA_FREERTOS


#if defined(USART_DMA_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif /* USART_DMA_FREERTOS */

static uint16_t sUsart2RxBuffer[USART2_RX_BUFFER_LEN];
static size_t   sUsart2RxTail;
#if defined(USART_DMA_FREERTOS)
static SemaphoreHandle_t sUsart2RxSemaphore;
static SemaphoreHandle_t sUsart2TxSemaphore;
#endif /* USART_DMA_FREERTOS */

void usart_dma_open(void) {

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef nvic_init;

    sUsart2RxTail = 0;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // GPIO A3 - USART2 RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    // GPIO A2 - USART2 TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);

    /* Set USART parameters */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART_InitStructure);

    /* initialize Rx DMA */
    DMA_InitStructure.DMA_BufferSize         = USART2_RX_BUFFER_LEN;            /* 1024 bytes */
    DMA_InitStructure.DMA_Channel            = DMA_Channel_4;                   /* Channel 4 */
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralToMemory;      /* from USART to memory */
    DMA_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;            /* No fifo mode */
    DMA_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull;      /* Irrelevant */
    DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t) &sUsart2RxBuffer[0];  /* RX buffer */
    DMA_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;          /* no burst */
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;     /* Half Words */
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;            /* Increment memory address */
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;               /* Restart on transfer complete */
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;           /* Read USART2 data register */
    DMA_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;      /* Single word */
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; /* Half Words */
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;       /* Always read from same location */
    DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;             /* Normal priority */
    DMA_Init(DMA1_Stream5, &DMA_InitStructure);     /* Init DMA */

    /* enable Rx DMA */
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);

#if defined(USART_DMA_FREERTOS)
    sUsart2RxSemaphore = xSemaphoreCreateCounting(1, 0);
    assert_param(sUsart2RxSemaphore != NULL);

    sUsart2TxSemaphore = xSemaphoreCreateCounting(1, 0);
    assert_param(sUsart2TxSemaphore != NULL);
#endif

    /* enable UART Interrupt */
    nvic_init.NVIC_IRQChannel = USART2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 8;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    /* enable DMA Interrupt */
    nvic_init.NVIC_IRQChannel = DMA1_Stream6_IRQn;
    NVIC_Init(&nvic_init);

    /* enable RXNE interrupt */
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    /* enable dma interrupt on tx done */
    DMA_ITConfig(DMA1_Stream6, DMA_IT_TC, ENABLE);

    /* enable DMA */
    DMA_Cmd(DMA1_Stream5, ENABLE);

    /* enable USART */
    USART_Cmd(USART2, ENABLE);
}

/*!
    Get the index of the FIFO head

    \retval The index of the head counter
*/
static inline size_t usart_dma_rx_get_head(void) {

    return (sizeof(sUsart2RxBuffer) / sizeof(uint16_t)) - DMA_GetCurrDataCounter(DMA1_Stream5);
}

/*!
    Cirular increment of the tail index

    \param[in]  inTail  Tail index before increment

    \return Increment of 1 of the tail index
*/
static inline size_t usart_dma_rx_inc_tail(size_t inTail) {

    return (inTail + 1) & (USART2_RX_BUFFER_LEN - 1);
}

/*!
    Circular decrement of the head index

    \param[in]  inHead  Head index before decrement

    \return Decrement of 1 of the head index
*/
static inline size_t usart_dma_rx_dec_head(size_t inHead) {

    return (inHead + USART2_RX_BUFFER_LEN - 1) & (USART2_RX_BUFFER_LEN - 1);
}

size_t usart_dma_rx_num(void) {

    return ((usart_dma_rx_get_head() + USART2_RX_BUFFER_LEN) - sUsart2RxTail) & (USART2_RX_BUFFER_LEN -1);
}

size_t usart_dma_read(uint8_t * inBuffer, size_t inMaxNumBytes) {

    size_t lAvailable;
    size_t lCount;
    size_t lCopy;

    assert_param(inBuffer != NULL);

    lAvailable = usart_dma_rx_num();
    lCopy = ((lAvailable < inMaxNumBytes)? lAvailable : inMaxNumBytes);

    for(lCount = 0; lCount < lCopy; lCount++) {
        inBuffer[lCount] = sUsart2RxBuffer[sUsart2RxTail];
        sUsart2RxTail = usart_dma_rx_inc_tail(sUsart2RxTail);
    }

    return lCopy;
}

size_t usart_dma_read_until(uint8_t * inBuffer, size_t inMaxNumBytes, uint8_t inCharacter) {

    size_t lAvailable;
    size_t lCount;
    size_t lCopy;
    uint8_t lCharacter;

    assert_param(inBuffer != NULL);

    lAvailable = usart_dma_rx_num();
    lCopy = ((lAvailable < inMaxNumBytes)? lAvailable : inMaxNumBytes);

    for(lCount = 0; lCount < lCopy;) {

        lCharacter = sUsart2RxBuffer[sUsart2RxTail];
        sUsart2RxTail = usart_dma_rx_inc_tail(sUsart2RxTail);
        inBuffer[lCount++] = lCharacter;

        if(lCharacter == inCharacter) {
            break;
        }
    }

    return lCount;
}

size_t usart_dma_read_until_str(uint8_t * inBuffer, size_t inMaxNumBytes, size_t inStartOffset, uint8_t * inString, size_t inStringLength) {
 
    size_t lAvailable;
    size_t lCount;
    size_t lCopy;

    assert_param(inBuffer != NULL);
    assert_param(inString != NULL);
    assert_param(inStringLength > 0);

    lAvailable = usart_dma_rx_num();
    lCopy = ((lAvailable < inMaxNumBytes)? lAvailable : inMaxNumBytes);

    for(lCount = 0; lCount < lCopy;) {

        inBuffer[inStartOffset + (lCount++)] = sUsart2RxBuffer[sUsart2RxTail];
        sUsart2RxTail = usart_dma_rx_inc_tail(sUsart2RxTail);

        /* got enough bytes to compare */
        if(inStartOffset + lCount >= inStringLength) {
            if(memcmp(&inBuffer[(inStartOffset + lCount) - inStringLength], inString, inStringLength) == 0) {
                /* match found */
                break;
            }
        }
    }

    return lCount;
}

bool usart_dma_peek(uint8_t * inString, size_t inStringLength) {

    size_t lAvailable;
    size_t lTail;
    size_t lIndex;

    lAvailable = usart_dma_rx_num();
    lTail = sUsart2RxTail;

    /* skip leading whitespaces */
    /* for(lTail = sUsart2RxTail; isspace(sUsart2RxBuffer[lTail]); lTail = usart_dma_rx_inc_tail(lTail), lAvailable--); */

    if(lAvailable < inStringLength) {
        return false;
    }

    for(lIndex = 0; lIndex < inStringLength; lIndex++, lTail = usart_dma_rx_inc_tail(lTail)) {

        /* mismatch */
        if(inString[lIndex] != (uint8_t)sUsart2RxBuffer[lTail]) {
            return false;
        }
    }

    return true;
}

bool usart_dma_peek_end(uint8_t * inString, size_t inStringLength) {

    size_t lAvailable;
    size_t lHead;
    size_t lIndex;

    lAvailable = usart_dma_rx_num();
    lHead = usart_dma_rx_dec_head(usart_dma_rx_get_head());

    if(lAvailable < inStringLength) {
        return false;
    }

    for(lIndex = inStringLength; lIndex > 0; lIndex--, lHead = usart_dma_rx_dec_head(lHead)) {

        /* mismatch */
        if(inString[lIndex-1] != (uint8_t)sUsart2RxBuffer[lHead]) {
            return false;
        }
    }

    return true;
}

size_t usart_dma_match(uint8_t * inString, size_t inStringLength) {

    size_t lAvailable;
    size_t lTail;
    size_t lOldTail;
    size_t lIndex;
    size_t lCount;
    bool   lFound;

    lAvailable = usart_dma_rx_num();

    if(lAvailable < inStringLength) {
        return 0;
    }

    lTail = sUsart2RxTail;
    lOldTail = lTail;
    for(lCount = 0; lCount < lAvailable - inStringLength; lCount++, lTail = usart_dma_rx_inc_tail(lOldTail)) {

        lOldTail = lTail;
        lFound = false;

        for(lIndex = 0; lIndex < inStringLength; lIndex++, lTail = usart_dma_rx_inc_tail(lTail)) {
            if(inString[lIndex] != (uint8_t)sUsart2RxBuffer[lTail]) {
                lFound = false;
                break;
            } else {
                lFound = true;
            }
        }

        if(lFound) {
            return lCount + inStringLength;
        }
    }

    return 0;
}

void usart_dma_rx_skip(size_t inNumberOfCharacters) {

    sUsart2RxTail = (sUsart2RxTail + inNumberOfCharacters) & (USART2_RX_BUFFER_LEN - 1);
}

void usart_dma_rx_skip_until(uint8_t inCharacter) {

    size_t lCount;
    size_t lAvailable;
    size_t lTail;

    lAvailable = usart_dma_rx_num();
    lTail = sUsart2RxTail;

    for(lCount = 0; lCount < lAvailable; lCount++, lTail = usart_dma_rx_inc_tail(lTail)) {
        if((uint8_t)sUsart2RxBuffer[lTail] == inCharacter) {

            sUsart2RxTail = usart_dma_rx_inc_tail(lTail);
            break;
        }
    }
}

void   usart_dma_rx_clear(void) {

    sUsart2RxTail = usart_dma_rx_get_head();
}

/*! Buffer storing dma data in TX direction. This is neccessary, as USART Data register only digests 16 or 32 bit accesses */
static uint16_t sUsart2TxBuffer[USART2_RX_BUFFER_LEN];

/*! Structure which is filled on each write */
static DMA_InitTypeDef sDMA_InitStructureTx = {
    .DMA_BufferSize         = 0,                                /* The size will be defined for each packet */
    .DMA_Channel            = DMA_Channel_4,                    /* Channel 4 */
    .DMA_DIR                = DMA_DIR_MemoryToPeripheral,       /* from memory to USART */
    .DMA_FIFOMode           = DMA_FIFOMode_Disable,             /* No fifo mode */
    .DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull,       /* Irrelevant */
    .DMA_Memory0BaseAddr    = (uint32_t) &sUsart2TxBuffer[0],   /* TX buffer */
    .DMA_MemoryBurst        = DMA_MemoryBurst_Single,           /* no burst */
    .DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord,      /* Half Words */
    .DMA_MemoryInc          = DMA_MemoryInc_Enable,             /* Increment memory address */
    .DMA_Mode               = DMA_Mode_Normal,                  /* Stop on transfer complete */
    .DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR,            /* Read USART2 data register */
    .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,       /* Single word */
    .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,  /* Half Words */
    .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,        /* Always read from same location */
    .DMA_Priority           = DMA_Priority_Medium,              /* Normal priority */
};

size_t usart_dma_write(uint8_t * inBuffer, size_t inNumBytes) {

    size_t lStartIndex;
    size_t lSendSize = 0;
    size_t lCount;
    size_t lCount1;

    for(lStartIndex = 0; lStartIndex < inNumBytes; lStartIndex += lSendSize) {

        lSendSize = (inNumBytes - lStartIndex > USART2_RX_BUFFER_LEN)? USART2_RX_BUFFER_LEN : (inNumBytes - lStartIndex);

        /* copy data to buffer */
        for(lCount = 0, lCount1 = lStartIndex; lCount < lSendSize; lCount++, lCount1++) {
            sUsart2TxBuffer[lCount] = inBuffer[lCount1];
        }

        /* setup DMA */
        sDMA_InitStructureTx.DMA_BufferSize      = lSendSize;
        sDMA_InitStructureTx.DMA_Memory0BaseAddr = (uint32_t) &sUsart2TxBuffer[0];

        DMA_Init(DMA1_Stream6, &sDMA_InitStructureTx);      /* Init DMA */

        /* start DMA */
        DMA_Cmd(DMA1_Stream6, ENABLE);

#if defined(USART_DMA_FREERTOS)
        /* wait on semaphore */
        xSemaphoreTake(sUsart2TxSemaphore, portMAX_DELAY);
#else
        while(!DMA_GetFlagStatus(DMA1_Stream6, DMA_FLAG_TCIF6));
        DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6);

        DMA_Cmd(DMA1_Stream6, DISABLE);
#endif
    }

    return lStartIndex;

#if 0
    size_t lCount;

    for(lCount = 0; lCount < inNumBytes; lCount++) {

        while(!USART_GetFlagStatus(USART2, USART_FLAG_TXE));
        USART_SendData(USART2, inBuffer[lCount]);
    }

    return inNumBytes;
#endif
}

void usart_dma_rx_wait(void) {

    /* wait on semaphore */
    xSemaphoreTake(sUsart2RxSemaphore, portMAX_DELAY);
}



/*! Usart2 interrupt handler */
void USART2_IRQHandler(void) {

    // if(USART_GetITStatus(USART2, USART_IT_RXNE)) {
        // USART_ClearITPendingBit(USART2, USART_IT_RXNE);

        // /* activate IDLE interrupt */
        // USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

        // /* deactivate RXNE interrupt */
        // USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    // }

    if(USART_GetITStatus(USART2, USART_IT_IDLE)) {     /*! Check if received Data */
        USART_ClearITPendingBit(USART2, USART_IT_IDLE);

        /* activate RXNE interrupt */
        USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

        /* deactivate IDLE interrupt */
        USART_ITConfig(USART2, USART_IT_IDLE, DISABLE);

#if defined(USART_DMA_FREERTOS)
        /* rise semaphore to treat data */
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(sUsart2RxSemaphore, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        }
#endif /* USART_DMA_FREERTOS */

    } else {    /* usually we should check for RXNE here, but DMA is faster... */

        /* activate IDLE interrupt */
        USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

        /* deactivate RXNE interrupt */
        USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    }
}

#if defined(USART_DMA_FREERTOS)
void DMA1_Stream6_IRQHandler(void) {


    if(DMA_GetITStatus(DMA1_Stream6, DMA_IT_TCIF6)) {
        DMA_ClearITPendingBit(DMA1_Stream6, DMA_IT_TCIF6);

//        DMA_Cmd(DMA1_Stream6, DISABLE);
        /* rise semaphore to treat data */
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(sUsart2TxSemaphore, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        }
    }
}
#endif


/* eof */
