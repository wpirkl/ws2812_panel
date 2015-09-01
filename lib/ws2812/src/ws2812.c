#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ws2812.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_tim.h"
#include "misc.h"

/* buffer size for double buffer */
#define DMA_DOUBLE_BUFFER_NUM_LEDS      (2)
#define DMA_DOUBLE_BUFFER_SIZE          (DMA_DOUBLE_BUFFER_NUM_LEDS * SIZE_OF_LED)

static uint16_t dmaBuffer[2 * DMA_DOUBLE_BUFFER_SIZE];      /* non concurrent access from ISR and from thread */
static volatile uint8_t  dmaBufferIdx = 0;                  /* changed from ISR and thread which could lead to an optimisation issue, defines which DMA buffer to fill */
static volatile size_t   dmaLedIdx = 0;                     /* changed from ISR and thread which could lead to an optimisation issue, indicates which led to process next */
static volatile bool     dmaLast = false;                   /* changed from ISR and thread which could lead to an optimisation issue, flag last DMA */
static volatile bool     dmaDone = false;                   /* changed from ISR, thread is polling, flag dma done */

static size_t   dmaLedRow = 0;

typedef struct {
    size_t      mStartIndex;
    size_t      mSkipLen;
} ts_skip_leds;

typedef struct {
    color        * mLeds;
    size_t         mSkipCount;
    ts_skip_leds * mSkip;
} ts_led_panel;

static color sLeds[NR_COLUMNS * NR_ROWS];

static ts_led_panel sLedPanel[NR_ROWS] = {
    {
        .mLeds = &sLeds[0 * NR_COLUMNS],
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = &sLeds[1 * NR_COLUMNS],
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = &sLeds[2 * NR_COLUMNS],
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = &sLeds[3 * NR_COLUMNS],
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = &sLeds[4 * NR_COLUMNS],
        .mSkipCount = 0,
        .mSkip = NULL
    },
};

static inline size_t isLedSkipped(size_t inLedRow, size_t inLedColumn) {

    size_t lCount;

    for(lCount = 0; lCount < sLedPanel[inLedRow].mSkipCount; lCount++) {
        if(inLedColumn >= sLedPanel[inLedRow].mSkip[lCount].mStartIndex &&
           inLedColumn <  (sLedPanel[inLedRow].mSkip[lCount].mStartIndex + sLedPanel[inLedRow].mSkip[lCount].mSkipLen)) {

                /* return the number of leds to be skipped */
                return sLedPanel[inLedRow].mSkip[lCount].mStartIndex + sLedPanel[inLedRow].mSkip[lCount].mSkipLen - inLedColumn;
            }
    }

    /* no leds are skipped */
    return 0;
}

void setLED(size_t inRow, size_t inColumn, uint8_t r, uint8_t g, uint8_t b) {

    assert_param(inRow < NR_ROWS);
    assert_param(inColumn < NR_COLUMNS);

    /* don't waste time if led is skipped */
    if(isLedSkipped(inRow, inColumn) > 0) {
        return;
    }

    sLedPanel[inRow].mLeds[inColumn].R = r;
    sLedPanel[inRow].mLeds[inColumn].G = g;
    sLedPanel[inRow].mLeds[inColumn].B = b;
}

void setLED_Color(size_t inRow, size_t inColumn, color *c) {

    assert_param(inRow < NR_ROWS);
    assert_param(inColumn < NR_COLUMNS);

    sLedPanel[inRow].mLeds[inColumn].R = c->R;
    sLedPanel[inRow].mLeds[inColumn].G = c->G;
    sLedPanel[inRow].mLeds[inColumn].B = c->B;
}

void setAllLED(uint8_t r, uint8_t g, uint8_t b){

    size_t lRowCount;
    size_t lColumnCount;

    for(lRowCount = 0; lRowCount < NR_ROWS; lRowCount++) {
        for(lColumnCount = 0; lColumnCount < NR_COLUMNS; lColumnCount++) {

            /* don't waste time if led is skipped */
            lColumnCount += isLedSkipped(lRowCount, lColumnCount);

            if(lColumnCount < NR_COLUMNS) {
                sLedPanel[lRowCount].mLeds[lColumnCount].R = r;
                sLedPanel[lRowCount].mLeds[lColumnCount].G = g;
                sLedPanel[lRowCount].mLeds[lColumnCount].B = b;
            }
        }
    }
}

void setAllLED_Color(color *c){

    setAllLED(c->R, c->G, c->B);
}

size_t getLEDPanelNumberOfRows(void) {

    return NR_ROWS;
}

size_t getLEDPanelNumberOfColumns(void) {

    return NR_COLUMNS;
}

void setAllLED_bulk(uint8_t * inBuffer, size_t inOffset, size_t inBufferSize) {

    assert_param(inOffset < sizeof(sLeds));
    assert_param(inOffset + inBufferSize < sizeof(sLeds));

    memcpy(&sLeds[inOffset], inBuffer, inBufferSize);
}

void ws2812_init(void) {

    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef timbaseinit;
    TIM_OCInitTypeDef timocinit;
    NVIC_InitTypeDef nvic_init;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    // GPIO C6 - TIM3 CH1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);

    // GPIO B5 - TIM3 CH2
    // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    // GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    // GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    // GPIO_Init(GPIOB, &GPIO_InitStructure);
    // GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_TIM3);

    // GPIO B0 - TIM3 CH3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);

    // GPIO B1 - TIM3 CH4
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM3);

    // GPIO D12 - TIM4 CH1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);

    // GPIO B7 - TIM4 CH2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_TIM4);

    // TIMER 3
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseStructInit(&timbaseinit);
    timbaseinit.TIM_ClockDivision = TIM_CKD_DIV1;
    timbaseinit.TIM_CounterMode = TIM_CounterMode_Up;
    timbaseinit.TIM_Period = WS2812_PWM_PERIOD;
    timbaseinit.TIM_Prescaler = 0;
    TIM_TimeBaseInit(TIM3, &timbaseinit);

    // TIMER 4
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_TimeBaseStructInit(&timbaseinit);
    timbaseinit.TIM_ClockDivision = TIM_CKD_DIV1;
    timbaseinit.TIM_CounterMode = TIM_CounterMode_Up;
    timbaseinit.TIM_Period = WS2812_PWM_PERIOD;
    timbaseinit.TIM_Prescaler = 0;
    TIM_TimeBaseInit(TIM4, &timbaseinit);


    // Timer 3 - Channel 1
    TIM_OCStructInit(&timocinit);
    timocinit.TIM_OCMode = TIM_OCMode_PWM1;
    timocinit.TIM_OCPolarity = TIM_OCPolarity_High;
    timocinit.TIM_OutputState = TIM_OutputState_Enable;
    timocinit.TIM_Pulse = 0;
    TIM_OC1Init(TIM3, &timocinit);

    // Timer 3 - Channel 2
    // TIM_OC2Init(TIM3, &timocinit);

    // Timer 3 - Channel 3
    TIM_OC3Init(TIM3, &timocinit);

    // Timer 3 - Channel 4
    TIM_OC4Init(TIM3, &timocinit);

    // Timer 4 - Channel 1
    TIM_OC1Init(TIM4, &timocinit);

    // Timer 4 - Channel 2
    TIM_OC2Init(TIM4, &timocinit);

    // Timer 3 - Channel 1
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // Timer 3 - Channel 2
    // TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // Timer 3 - Channel 3
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // Timer 3 - Channel 4
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // Timer 4 - Channel 1
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

    // Timer 4 - Channel 2
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);

    // Timer 3 ARR
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    // Timer 4 ARR
    TIM_ARRPreloadConfig(TIM4, ENABLE);

    // Timer 3 Enable
    TIM_CCxCmd(TIM3, TIM_Channel_1, TIM_CCx_Enable);
//    TIM_CCxCmd(TIM3, TIM_Channel_2, TIM_CCx_Enable);
    TIM_CCxCmd(TIM3, TIM_Channel_3, TIM_CCx_Enable);
    TIM_CCxCmd(TIM3, TIM_Channel_4, TIM_CCx_Enable);
    TIM_Cmd(TIM3, ENABLE);

    // Timer 4 Enable
    TIM_CCxCmd(TIM4, TIM_Channel_1, TIM_CCx_Enable);
    TIM_CCxCmd(TIM4, TIM_Channel_2, TIM_CCx_Enable);
    TIM_Cmd(TIM4, ENABLE);

    // DMA
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    // Timer3 DMA
    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
//    TIM_DMACmd(TIM3, TIM_DMA_CC2, ENABLE);
    TIM_DMACmd(TIM3, TIM_DMA_CC3, ENABLE);
    TIM_DMACmd(TIM3, TIM_DMA_CC4, ENABLE);

    // Timer4 DMA
    TIM_DMACmd(TIM4, TIM_DMA_CC1, ENABLE);
    TIM_DMACmd(TIM4, TIM_DMA_CC2, ENABLE);

    // Tim3 CH1
    DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, ENABLE);

    // Tim3 CH2
//    DMA_ITConfig(DMA1_Stream5, DMA_IT_TC, ENABLE);

    // Tim3 CH3
    DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, ENABLE);

    // Tim3 CH4
    DMA_ITConfig(DMA1_Stream2, DMA_IT_TC, ENABLE);

    // Tim4 CH1
    DMA_ITConfig(DMA1_Stream0, DMA_IT_TC, ENABLE);

    // Tim4 CH2
    DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);


    // NVIC for DMA - Tim3 Ch1
    nvic_init.NVIC_IRQChannel = DMA1_Stream4_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 7;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    // NVIC for DMA - Tim3 Ch2
    // nvic_init.NVIC_IRQChannel = DMA1_Stream5_IRQn;
    // nvic_init.NVIC_IRQChannelPreemptionPriority = 7;
    // nvic_init.NVIC_IRQChannelSubPriority = 0;
    // nvic_init.NVIC_IRQChannelCmd = ENABLE;
    // NVIC_Init(&nvic_init);

    // NVIC for DMA - Tim3 Ch3
    nvic_init.NVIC_IRQChannel = DMA1_Stream7_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 7;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    // NVIC for DMA - Tim3 Ch4
    nvic_init.NVIC_IRQChannel = DMA1_Stream2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 7;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    // NVIC for DMA - Tim4 Ch1
    nvic_init.NVIC_IRQChannel = DMA1_Stream0_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 7;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    // NVIC for DMA - Tim4 Ch2
    nvic_init.NVIC_IRQChannel = DMA1_Stream3_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 7;
    nvic_init.NVIC_IRQChannelSubPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    setAllLED(0,0,0);
    updateLED();
}

// start dma on timer 3 ch1
static void start_dma_t3_ch1(void) {
    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &dmaBuffer[0],           /* first double buffer */
        .DMA_MemoryBurst          = DMA_MemoryBurst_Single,             /* no burst */
        .DMA_MemoryDataSize       = DMA_MemoryDataSize_HalfWord,        /* 16 bit */
        .DMA_MemoryInc            = DMA_MemoryInc_Enable,               /* increment memory address */
        .DMA_Mode                 = DMA_Mode_Circular,                  /* circular for double buffering */
        .DMA_PeripheralBaseAddr   = (uint32_t) &TIM3->CCR1,             /* timer capture compare register */
        .DMA_PeripheralBurst      = DMA_PeripheralBurst_Single,         /* no burst */
        .DMA_PeripheralDataSize   = DMA_PeripheralDataSize_HalfWord,    /* 16 bit */
        .DMA_PeripheralInc        = DMA_PeripheralInc_Disable,          /* no increment */
        .DMA_Priority             = DMA_Priority_High                   /* high priority */
    };

    /* initialize dma */
    DMA_Init(DMA1_Stream4, &dma_init);

    /* start with double buffer 0 */
    DMA_DoubleBufferModeConfig(DMA1_Stream4, (uint32_t)&dmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream4, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream4, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
}

// start dma on timer 3 ch2
// void start_dma_t3_ch2(void)
// {
    // /* static const makes the linker put it to text section */
    // static const DMA_InitTypeDef dma_init =
    // {
        // .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        // .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        // .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        // .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        // .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        // .DMA_Memory0BaseAddr      = (uint32_t) &dmaBuffer[0],           /* first double buffer */
        // .DMA_MemoryBurst          = DMA_MemoryBurst_Single,             /* no burst */
        // .DMA_MemoryDataSize       = DMA_MemoryDataSize_HalfWord,        /* 16 bit */
        // .DMA_MemoryInc            = DMA_MemoryInc_Enable,               /* increment memory address */
        // .DMA_Mode                 = DMA_Mode_Circular,                  /* circular for double buffering */
        // .DMA_PeripheralBaseAddr   = (uint32_t) &TIM3->CCR2,             /* timer capture compare register */
        // .DMA_PeripheralBurst      = DMA_PeripheralBurst_Single,         /* no burst */
        // .DMA_PeripheralDataSize   = DMA_PeripheralDataSize_HalfWord,    /* 16 bit */
        // .DMA_PeripheralInc        = DMA_PeripheralInc_Disable,          /* no increment */
        // .DMA_Priority             = DMA_Priority_High                   /* high priority */
    // };

    // /* initialize dma */
    // DMA_Init(DMA1_Stream5, &dma_init);

    // /* start with double buffer 0 */
    // DMA_DoubleBufferModeConfig(DMA1_Stream5, (uint32_t)&dmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    // /* enable double buffering */
    // DMA_DoubleBufferModeCmd(DMA1_Stream5, ENABLE);

    // /* enable dma */
    // DMA_Cmd(DMA1_Stream5, ENABLE);

    // /* enable timer */
    // TIM_DMACmd(TIM3, TIM_DMA_CC2, ENABLE);
// }

// start dma on timer 3 ch3
static void start_dma_t3_ch3(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &dmaBuffer[0],           /* first double buffer */
        .DMA_MemoryBurst          = DMA_MemoryBurst_Single,             /* no burst */
        .DMA_MemoryDataSize       = DMA_MemoryDataSize_HalfWord,        /* 16 bit */
        .DMA_MemoryInc            = DMA_MemoryInc_Enable,               /* increment memory address */
        .DMA_Mode                 = DMA_Mode_Circular,                  /* circular for double buffering */
        .DMA_PeripheralBaseAddr   = (uint32_t) &TIM3->CCR3,             /* timer capture compare register */
        .DMA_PeripheralBurst      = DMA_PeripheralBurst_Single,         /* no burst */
        .DMA_PeripheralDataSize   = DMA_PeripheralDataSize_HalfWord,    /* 16 bit */
        .DMA_PeripheralInc        = DMA_PeripheralInc_Disable,          /* no increment */
        .DMA_Priority             = DMA_Priority_High                   /* high priority */
    };

    /* initialize dma */
    DMA_Init(DMA1_Stream7, &dma_init);

    /* start with double buffer 0 */
    DMA_DoubleBufferModeConfig(DMA1_Stream7, (uint32_t)&dmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream7, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream7, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM3, TIM_DMA_CC3, ENABLE);
}

// start dma on timer 3 ch4
static void start_dma_t3_ch4(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &dmaBuffer[0],           /* first double buffer */
        .DMA_MemoryBurst          = DMA_MemoryBurst_Single,             /* no burst */
        .DMA_MemoryDataSize       = DMA_MemoryDataSize_HalfWord,        /* 16 bit */
        .DMA_MemoryInc            = DMA_MemoryInc_Enable,               /* increment memory address */
        .DMA_Mode                 = DMA_Mode_Circular,                  /* circular for double buffering */
        .DMA_PeripheralBaseAddr   = (uint32_t) &TIM3->CCR4,             /* timer capture compare register */
        .DMA_PeripheralBurst      = DMA_PeripheralBurst_Single,         /* no burst */
        .DMA_PeripheralDataSize   = DMA_PeripheralDataSize_HalfWord,    /* 16 bit */
        .DMA_PeripheralInc        = DMA_PeripheralInc_Disable,          /* no increment */
        .DMA_Priority             = DMA_Priority_High                   /* high priority */
    };

    /* initialize dma */
    DMA_Init(DMA1_Stream2, &dma_init);

    /* start with double buffer 0 */
    DMA_DoubleBufferModeConfig(DMA1_Stream2, (uint32_t)&dmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream2, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream2, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM3, TIM_DMA_CC4, ENABLE);
}

// start dma on timer 4 ch1
static void start_dma_t4_ch1(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_2,                      /* DMA channel 2 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &dmaBuffer[0],           /* first double buffer */
        .DMA_MemoryBurst          = DMA_MemoryBurst_Single,             /* no burst */
        .DMA_MemoryDataSize       = DMA_MemoryDataSize_HalfWord,        /* 16 bit */
        .DMA_MemoryInc            = DMA_MemoryInc_Enable,               /* increment memory address */
        .DMA_Mode                 = DMA_Mode_Circular,                  /* circular for double buffering */
        .DMA_PeripheralBaseAddr   = (uint32_t) &TIM4->CCR1,             /* timer capture compare register */
        .DMA_PeripheralBurst      = DMA_PeripheralBurst_Single,         /* no burst */
        .DMA_PeripheralDataSize   = DMA_PeripheralDataSize_HalfWord,    /* 16 bit */
        .DMA_PeripheralInc        = DMA_PeripheralInc_Disable,          /* no increment */
        .DMA_Priority             = DMA_Priority_High                   /* high priority */
    };

    /* initialize dma */
    DMA_Init(DMA1_Stream0, &dma_init);

    /* start with double buffer 0 */
    DMA_DoubleBufferModeConfig(DMA1_Stream0, (uint32_t)&dmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream0, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream0, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM4, TIM_DMA_CC1, ENABLE);
}

// start dma on timer 4 ch2
static void start_dma_t4_ch2(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_2,                      /* DMA channel 2 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &dmaBuffer[0],           /* first double buffer */
        .DMA_MemoryBurst          = DMA_MemoryBurst_Single,             /* no burst */
        .DMA_MemoryDataSize       = DMA_MemoryDataSize_HalfWord,        /* 16 bit */
        .DMA_MemoryInc            = DMA_MemoryInc_Enable,               /* increment memory address */
        .DMA_Mode                 = DMA_Mode_Circular,                  /* circular for double buffering */
        .DMA_PeripheralBaseAddr   = (uint32_t) &TIM4->CCR2,             /* timer capture compare register */
        .DMA_PeripheralBurst      = DMA_PeripheralBurst_Single,         /* no burst */
        .DMA_PeripheralDataSize   = DMA_PeripheralDataSize_HalfWord,    /* 16 bit */
        .DMA_PeripheralInc        = DMA_PeripheralInc_Disable,          /* no increment */
        .DMA_Priority             = DMA_Priority_High                   /* high priority */
    };

    /* initialize dma */
    DMA_Init(DMA1_Stream3, &dma_init);

    /* start with double buffer 0 */
    DMA_DoubleBufferModeConfig(DMA1_Stream3, (uint32_t)&dmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream3, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream3, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM4, TIM_DMA_CC2, ENABLE);
}


typedef void (*f_start_dma)(void);

/* Function pointers to simplify launching of dmas */
static f_start_dma s_start_dma_funcs[NR_ROWS] = {
    start_dma_t3_ch1,
//    start_dma_t3_ch2,
    start_dma_t4_ch1,
    start_dma_t3_ch3,
    start_dma_t3_ch4,
    start_dma_t4_ch2,
};

/*!
    Call a specific start dma function
*/
static void start_dma() {

    assert_param(dmaLedRow < NR_ROWS);

    s_start_dma_funcs[dmaLedRow]();
}

/*!
    Circular increment for DMA double buffer
*/
static inline size_t incrementBufferIndex(size_t inBufferIndex) {
    return (inBufferIndex + 1) & 1;
}

/*!
    This function fills the next double buffer with the led contents

    It is absolutely neccessary to finish this before the dma can complete the second double buffer
*/
static inline void fillBuffer(void) {

    size_t lCount;
    size_t lBitMask;
    size_t lBitIndex;
    size_t lIndex;

    /* avoid access to volatile variables */
    size_t lDmaBufferIndexCache = dmaBufferIdx;
    size_t lDmaLedRow = dmaLedRow;

    uint16_t * lGreenPtr = &dmaBuffer[lDmaBufferIndexCache * DMA_DOUBLE_BUFFER_SIZE];
    uint16_t * lRedPtr   = &dmaBuffer[lDmaBufferIndexCache * DMA_DOUBLE_BUFFER_SIZE +  8];
    uint16_t * lBluePtr  = &dmaBuffer[lDmaBufferIndexCache * DMA_DOUBLE_BUFFER_SIZE + 16];

    /* fill whole buffer */
    for(lCount = 0; lCount < DMA_DOUBLE_BUFFER_NUM_LEDS; lCount++) {

        lIndex = dmaLedIdx;

        /* skip leds which are marked 'not present' */
        lIndex += isLedSkipped(lDmaLedRow, lIndex);

        /* check if index is still in range */
        if(lIndex < NR_COLUMNS) {

            /* decode colors to pwm duty cycles */
            for(lBitMask = 0x80, lBitIndex = 0; lBitMask != 0; lBitMask >>= 1, lBitIndex++) {

                if((sLedPanel[lDmaLedRow].mLeds[lIndex].R & lBitMask) != 0) {
                    lRedPtr[lBitIndex] = WS2812_PWM_ONE;
                } else {
                    lRedPtr[lBitIndex] = WS2812_PWM_ZERO;
                }

                if((sLedPanel[lDmaLedRow].mLeds[lIndex].G & lBitMask) != 0) {
                    lGreenPtr[lBitIndex] = WS2812_PWM_ONE;
                } else {
                    lGreenPtr[lBitIndex] = WS2812_PWM_ZERO;
                }

                if((sLedPanel[lDmaLedRow].mLeds[lIndex].B & lBitMask) != 0) {
                    lBluePtr[lBitIndex] = WS2812_PWM_ONE;
                } else {
                    lBluePtr[lBitIndex] = WS2812_PWM_ZERO;
                }
            }

        } else {

            /* fill with zeroes */
            for(lBitIndex = 0; lBitIndex < 8; lBitIndex++) {
                lRedPtr[lBitIndex] = 0;
                lGreenPtr[lBitIndex] = 0;
                lBluePtr[lBitIndex] = 0;
            }
        }

        /* jump over other colors */
        lGreenPtr += SIZE_OF_LED;
        lRedPtr   += SIZE_OF_LED;
        lBluePtr  += SIZE_OF_LED;

        /* next led */
        dmaLedIdx = lIndex + 1;
    }

    dmaBufferIdx = incrementBufferIndex(lDmaBufferIndexCache);
}


void updateLED(void){

    /* iterate over all rows */
    for(dmaLedRow = 0; dmaLedRow < NR_ROWS; dmaLedRow++) {

        /* initialize global variables */
        dmaBufferIdx = 0;
        dmaLedIdx = 0;
        dmaDone = false;
        dmaLast = false;

        /* fill memory 0 */
        fillBuffer();

        /* fill memory 1 */
        fillBuffer();

        /* start dma transfer */
        start_dma();

        /* wait for DMA done */
        for(;!dmaDone;);
    }
}

static inline bool checkLastIndex(void) {

    /* +2 adds 2*3*24 bits which corresponds to the end frame ~50us */
    return dmaLedIdx > (NR_COLUMNS + 2);
}

// Handler for Tim3 CH1 DMA
void DMA1_Stream4_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4)) {
        DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);

        if(dmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream4, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM3, TIM_DMA_CC1, DISABLE);

            dmaDone = true;
        } else {

            /* fill next buffer */
            fillBuffer();

            dmaLast = checkLastIndex();
        }
    }
}

// Handler for Tim3 CH2 DMA
// void DMA1_Stream5_IRQHandler(void) {

    // if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5)) {
        // DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);

        // if(dmaLast) {
            // /* disable dma */
            // DMA_Cmd(DMA1_Stream5, DISABLE);

            // /* disable timer */
            // TIM_DMACmd(TIM3, TIM_DMA_CC2, DISABLE);

            // dmaDone = true;
        // } else {

            // /* fill next buffer */
            // fillBuffer();

            // dmaLast = checkLastIndex();
        // }
    // }
// }

// Handler for Tim3 CH3 DMA
void DMA1_Stream7_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream7, DMA_IT_TCIF7)) {
        DMA_ClearITPendingBit(DMA1_Stream7, DMA_IT_TCIF7);

        if(dmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream7, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM3, TIM_DMA_CC3, DISABLE);

            dmaDone = true;
        } else {

            /* fill next buffer */
            fillBuffer();

            dmaLast = checkLastIndex();
        }
    }
}

// Handler for Tim3 CH4 DMA
void DMA1_Stream2_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream2, DMA_IT_TCIF2)) {
        DMA_ClearITPendingBit(DMA1_Stream2, DMA_IT_TCIF2);

        if(dmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream2, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM3, TIM_DMA_CC4, DISABLE);

            dmaDone = true;
        } else {

            /* fill next buffer */
            fillBuffer();

            dmaLast = checkLastIndex();
        }
    }
}

// Handler for Tim4 CH1 DMA
void DMA1_Stream0_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream0, DMA_IT_TCIF0)) {
        DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_TCIF0);

        if(dmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream0, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM4, TIM_DMA_CC1, DISABLE);

            dmaDone = true;
        } else {

            /* fill next buffer */
            fillBuffer();

            dmaLast = checkLastIndex();
        }
    }
}

// Handler for Tim4 CH2 DMA
void DMA1_Stream3_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream3, DMA_IT_TCIF3)) {
        DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);

        if(dmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream3, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM4, TIM_DMA_CC2, DISABLE);

            dmaDone = true;
        } else {

            /* fill next buffer */
            fillBuffer();

            dmaLast = checkLastIndex();
        }
    }
}

// eof

