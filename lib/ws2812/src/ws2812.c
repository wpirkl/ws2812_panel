#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ws2812.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_tim.h"
#include "misc.h"

// ----------------------------- definitions -----------------------------
// moved to header file
// #define WS2812_NR_ROWS         (5)
// #define WS2812_NR_COLUMNS      (172)

#define SIZE_OF_LED     (24)      // 3(RGB) * 8 Bit

#define WS2812_TIM_FREQ       ((168000000 / 4) * 2)
#define WS2812_OUT_FREQ       (800000)

// timer values to generate a "one" or a "zero" according to ws2812 datasheet
#define WS2812_PWM_PERIOD       ((WS2812_TIM_FREQ / WS2812_OUT_FREQ))
#define WS2812_PWM_ZERO         (29)  // 0.5 µs of 2.5µs is high => 1/5 of the period
#define WS2812_PWM_ONE          (58) // 2µs of 2.5µs is high -> 4/5 of the period


// number of timer cycles (~2.5µs) for the reset pulse
#define WS2812_RESET_LEN        (20) //20*2.5µs = 50µs

/* buffer size for double buffer */
#define DMA_DOUBLE_BUFFER_NUM_LEDS      (2)
#define DMA_DOUBLE_BUFFER_SIZE          (DMA_DOUBLE_BUFFER_NUM_LEDS * SIZE_OF_LED)
#define DMA_DOUBLE_BUFFER_ROW_SIZE      (2 * DMA_DOUBLE_BUFFER_SIZE)

#define TIM3_CH1_ROW_IDX                (0)
#define TIM3_CH3_ROW_IDX                (2)
#define TIM3_CH4_ROW_IDX                (3)
#define TIM4_CH1_ROW_IDX                (1)
#define TIM4_CH2_ROW_IDX                (4)


#define WS2812_PARALLEL_ROW
#define WS2812_FREERTOS



#if defined(WS2812_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif

/*! Internal structure of a led row */
typedef struct {
    uint16_t             mDmaBuffer[DMA_DOUBLE_BUFFER_ROW_SIZE];
    volatile size_t      mDmaBufferIndex;
    volatile size_t      mDmaColumnIndex;
    volatile bool        mDmaLast;
#if defined(WS2812_FREERTOS)
    SemaphoreHandle_t   mDmaDoneSemaphore;
#else /* WS2812_FREERTOS */
    volatile bool        mDmaDone;
#endif /* WS2812_FREERTOS */
} ts_update_row;

/*! Structure defining skipped leds */
typedef struct {
    size_t      mStartIndex;
    size_t      mSkipLen;
} ts_skip_leds;

/*! Structure defining one row */
typedef struct {
    size_t                     mLeds;
    size_t                     mSkipCount;
    const ts_skip_leds * const mSkip;
} ts_led_panel;


static const ts_skip_leds sSkipRow3 = {
    .mStartIndex = 143,
    .mSkipLen = 6,
};

static color * sUpdatePanel = NULL;

static const ts_led_panel sLedPanel[WS2812_NR_ROWS] = {
    {
        .mLeds = 0 * WS2812_NR_COLUMNS,
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = 1 * WS2812_NR_COLUMNS,
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = 2 * WS2812_NR_COLUMNS,
        .mSkipCount = 1,
        .mSkip = &sSkipRow3,
    },
    {
        .mLeds = 3 * WS2812_NR_COLUMNS,
        .mSkipCount = 0,
        .mSkip = NULL
    },
    {
        .mLeds = 4 * WS2812_NR_COLUMNS,
        .mSkipCount = 0,
        .mSkip = NULL
    },
};

static ts_update_row sLedDMA[WS2812_NR_ROWS];

/*!
    Check if a led is skipped

    \param[in]  inLedRow    The row index of the LED
    \param[in]  inLedColumn The column index of the LED

    \retval the number of leds to be skipped
*/
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

void ws2812_setLED(color * inPanel, size_t inRow, size_t inColumn, uint8_t r, uint8_t g, uint8_t b) {

    assert_param(inRow < WS2812_NR_ROWS);
    assert_param(inColumn < WS2812_NR_COLUMNS);

    /* don't waste time if led is skipped */
    if(isLedSkipped(inRow, inColumn) > 0) {
        return;
    }

    inPanel[sLedPanel[inRow].mLeds + inColumn].R = r;
    inPanel[sLedPanel[inRow].mLeds + inColumn].G = g;
    inPanel[sLedPanel[inRow].mLeds + inColumn].B = b;
}

void ws2812_setLED_Column(color * inPanel, size_t inColumn, uint8_t r, uint8_t g, uint8_t b) {

    size_t lRowNum = ws2812_getLED_PanelNumberOfRows();
    size_t lRowCount;

    for(lRowCount = 0; lRowCount < lRowNum; lRowCount++) {
        ws2812_setLED(inPanel, lRowCount, inColumn, r, g, b);
    }
}

void ws2812_setLED_Row(color * inPanel, size_t inRow, uint8_t r, uint8_t g, uint8_t b) {

    size_t lColumnNum = ws2812_getLED_PanelNumberOfColumns();
    size_t lColumnCount;

    for(lColumnCount = 0; lColumnCount < lColumnNum; lColumnCount++) {
        ws2812_setLED(inPanel, inRow, lColumnCount, r, g, b);
    }
}

void ws2812_setLED_All(color * inPanel, uint8_t r, uint8_t g, uint8_t b){

    size_t lRowCount;
    size_t lColumnCount;

    for(lRowCount = 0; lRowCount < WS2812_NR_ROWS; lRowCount++) {
        for(lColumnCount = 0; lColumnCount < WS2812_NR_COLUMNS; lColumnCount++) {

            /* don't waste time if led is skipped */
            lColumnCount += isLedSkipped(lRowCount, lColumnCount);

            if(lColumnCount < WS2812_NR_COLUMNS) {
                inPanel[sLedPanel[lRowCount].mLeds + lColumnCount].R = r;
                inPanel[sLedPanel[lRowCount].mLeds + lColumnCount].G = g;
                inPanel[sLedPanel[lRowCount].mLeds + lColumnCount].B = b;
            }
        }
    }
}

size_t ws2812_getLED_PanelNumberOfRows(void) {

    return WS2812_NR_ROWS;
}

size_t ws2812_getLED_PanelNumberOfColumns(void) {

    return WS2812_NR_COLUMNS;
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
    GPIO_Init(GPIOD, &GPIO_InitStructure);
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
    TIM_DMACmd(TIM3, TIM_DMA_CC3, ENABLE);
    TIM_DMACmd(TIM3, TIM_DMA_CC4, ENABLE);

    // Timer4 DMA
    TIM_DMACmd(TIM4, TIM_DMA_CC1, ENABLE);
    TIM_DMACmd(TIM4, TIM_DMA_CC2, ENABLE);

    // Tim3 CH1
    DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, ENABLE);

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

#if defined(WS2812_FREERTOS)
    {
        size_t lRow;

        for(lRow = 0; lRow < WS2812_NR_ROWS; lRow++) {
            sLedDMA[lRow].mDmaDoneSemaphore = xSemaphoreCreateCounting(1, 0);
            assert_param(sLedDMA[lRow].mDmaDoneSemaphore != NULL);
        }
    }
#endif
}

/*! start dma on timer 3 ch1 */
static void start_dma_t3_ch1(void) {
    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &sLedDMA[TIM3_CH1_ROW_IDX].mDmaBuffer[0],   /* first double buffer */
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
    DMA_DoubleBufferModeConfig(DMA1_Stream4, (uint32_t)&sLedDMA[TIM3_CH1_ROW_IDX].mDmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream4, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream4, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
}

/*! start dma on timer 3 ch3 */
static void start_dma_t3_ch3(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &sLedDMA[TIM3_CH3_ROW_IDX].mDmaBuffer[0],           /* first double buffer */
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
    DMA_DoubleBufferModeConfig(DMA1_Stream7, (uint32_t)&sLedDMA[TIM3_CH3_ROW_IDX].mDmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream7, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream7, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM3, TIM_DMA_CC3, ENABLE);
}

/*! start dma on timer 3 ch4 */
static void start_dma_t3_ch4(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_5,                      /* DMA channel 5 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &sLedDMA[TIM3_CH4_ROW_IDX].mDmaBuffer[0],           /* first double buffer */
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
    DMA_DoubleBufferModeConfig(DMA1_Stream2, (uint32_t)&sLedDMA[TIM3_CH4_ROW_IDX].mDmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream2, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream2, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM3, TIM_DMA_CC4, ENABLE);
}

/*! start dma on timer 4 ch1 */
static void start_dma_t4_ch1(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_2,                      /* DMA channel 2 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &sLedDMA[TIM4_CH1_ROW_IDX].mDmaBuffer[0],           /* first double buffer */
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
    DMA_DoubleBufferModeConfig(DMA1_Stream0, (uint32_t)&sLedDMA[TIM4_CH1_ROW_IDX].mDmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream0, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream0, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM4, TIM_DMA_CC1, ENABLE);
}

/*! start dma on timer 4 ch2 */
static void start_dma_t4_ch2(void) {

    /* static const makes the linker put it to text section */
    static const DMA_InitTypeDef dma_init =
    {
        .DMA_BufferSize           = DMA_DOUBLE_BUFFER_SIZE,             /* set size of one buffer of double buffer */
        .DMA_Channel              = DMA_Channel_2,                      /* DMA channel 2 */
        .DMA_DIR                  = DMA_DIR_MemoryToPeripheral,         /* from memory to timer */
        .DMA_FIFOMode             = DMA_FIFOMode_Disable,               /* no fifo mode */
        .DMA_FIFOThreshold        = DMA_FIFOThreshold_HalfFull,         /* unused */
        .DMA_Memory0BaseAddr      = (uint32_t) &sLedDMA[TIM4_CH2_ROW_IDX].mDmaBuffer[0],           /* first double buffer */
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
    DMA_DoubleBufferModeConfig(DMA1_Stream3, (uint32_t)&sLedDMA[TIM4_CH2_ROW_IDX].mDmaBuffer[DMA_DOUBLE_BUFFER_SIZE], DMA_Memory_0);

    /* enable double buffering */
    DMA_DoubleBufferModeCmd(DMA1_Stream3, ENABLE);

    /* enable dma */
    DMA_Cmd(DMA1_Stream3, ENABLE);

    /* enable timer */
    TIM_DMACmd(TIM4, TIM_DMA_CC2, ENABLE);
}


typedef void (*f_start_dma)(void);

/*! Function pointers to simplify launching of dmas */
static f_start_dma s_start_dma_funcs[WS2812_NR_ROWS] = {
    start_dma_t3_ch1,
    start_dma_t4_ch1,
    start_dma_t3_ch3,
    start_dma_t3_ch4,
    start_dma_t4_ch2,
};

/*!
    Call a specific start dma function
*/
static void start_dma(size_t inRow) {

    assert_param(inRow < WS2812_NR_ROWS);

    s_start_dma_funcs[inRow]();
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
static inline void fillBuffer(size_t inRow) {

    size_t lCount;
    size_t lBitMask;
    size_t lBitIndex;
    size_t lIndex;

    assert_param(inRow < WS2812_NR_ROWS);

    /* avoid access to volatile variables */
    size_t lDmaBufferIndexCache = sLedDMA[inRow].mDmaBufferIndex;

    uint16_t * lGreenPtr = &sLedDMA[inRow].mDmaBuffer[lDmaBufferIndexCache * DMA_DOUBLE_BUFFER_SIZE];
    uint16_t * lRedPtr   = &sLedDMA[inRow].mDmaBuffer[lDmaBufferIndexCache * DMA_DOUBLE_BUFFER_SIZE +  8];
    uint16_t * lBluePtr  = &sLedDMA[inRow].mDmaBuffer[lDmaBufferIndexCache * DMA_DOUBLE_BUFFER_SIZE + 16];

    /* fill whole buffer */
    for(lCount = 0; lCount < DMA_DOUBLE_BUFFER_NUM_LEDS; lCount++) {

        lIndex = sLedDMA[inRow].mDmaColumnIndex;

        /* skip leds which are marked 'not present' */
        lIndex += isLedSkipped(inRow, lIndex);

        /* check if index is still in range */
        if(lIndex < WS2812_NR_COLUMNS) {

            color lColor;
            lColor.R = sUpdatePanel[sLedPanel[inRow].mLeds + lIndex].R;
            lColor.G = sUpdatePanel[sLedPanel[inRow].mLeds + lIndex].G;
            lColor.B = sUpdatePanel[sLedPanel[inRow].mLeds + lIndex].B;

            /* decode colors to pwm duty cycles */
            for(lBitMask = 0x80, lBitIndex = 0; lBitMask != 0; lBitMask >>= 1, lBitIndex++) {

                if((lColor.R & lBitMask) != 0) {
                    lRedPtr[lBitIndex] = WS2812_PWM_ONE;
                } else {
                    lRedPtr[lBitIndex] = WS2812_PWM_ZERO;
                }

                if((lColor.G & lBitMask) != 0) {
                    lGreenPtr[lBitIndex] = WS2812_PWM_ONE;
                } else {
                    lGreenPtr[lBitIndex] = WS2812_PWM_ZERO;
                }

                if((lColor.B & lBitMask) != 0) {
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
        sLedDMA[inRow].mDmaColumnIndex = lIndex + 1;
    }

    sLedDMA[inRow].mDmaBufferIndex = incrementBufferIndex(lDmaBufferIndexCache);
}

void ws2812_updateLED(color * inPanel){

    size_t lRow;

    sUpdatePanel = inPanel;

    /* iterate over all rows */
    for(lRow = 0; lRow < WS2812_NR_ROWS; lRow++) {

        /* initialize global variables */
        sLedDMA[lRow].mDmaBufferIndex = 0;
        sLedDMA[lRow].mDmaColumnIndex = 0;
        sLedDMA[lRow].mDmaLast = false;
#if !defined(WS2812_FREERTOS)
        sLedDMA[lRow].mDmaDone = false;
#endif

        /* fill memory 0 */
        fillBuffer(lRow);

        /* fill memory 1 */
        fillBuffer(lRow);

        /* start dma transfer */
        start_dma(lRow);

#if !defined(WS2812_PARALLEL_ROW)
#if defined(WS2812_FREERTOS)
        /* wait on semaphore */
        xSemaphoreTake(sLedDMA[lRow].mDmaDoneSemaphore, portMAX_DELAY);
#else /* WS2812_FREERTOS */
        /* wait for DMA done */
        for(;!sLedDMA[lRow].mDmaDone;);
#endif /* WS2812_FREERTOS */
#endif /* WS2812_PARALLEL_ROW */
    }

#if defined(WS2812_PARALLEL_ROW)
    for(lRow = 0; lRow < WS2812_NR_ROWS; lRow++) {
#if defined(WS2812_FREERTOS)
        /* wait on semaphore */
        xSemaphoreTake(sLedDMA[lRow].mDmaDoneSemaphore, portMAX_DELAY);
#else /* WS2812_FREERTOS */
        /* wait for DMA done */
        for(;!sLedDMA[lRow].mDmaDone;);
#endif /* WS2812_FREERTOS */
    }
#endif /* WS2812_PARALLEL_ROW */
}

/*! Check if DMA tranmitted the last leds of a specific row */
static inline bool checkLastIndex(size_t inRow) {

    /* +2 adds 2*3*24 bits which corresponds to the end frame ~50us */
    return sLedDMA[inRow].mDmaColumnIndex > (WS2812_NR_COLUMNS + 2);
}

/*! Handler for Tim3 CH1 DMA */
void DMA1_Stream4_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4)) {
        DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);

        if(sLedDMA[TIM3_CH1_ROW_IDX].mDmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream4, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM3, TIM_DMA_CC1, DISABLE);

#if defined(WS2812_FREERTOS)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(sLedDMA[TIM3_CH1_ROW_IDX].mDmaDoneSemaphore, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
#else /* WS2812_FREERTOS */
            sLedDMA[TIM3_CH1_ROW_IDX].mDmaDone = true;
#endif /* WS2812_FREERTOS */
        } else {

            /* fill next buffer */
            fillBuffer(TIM3_CH1_ROW_IDX);

            sLedDMA[TIM3_CH1_ROW_IDX].mDmaLast = checkLastIndex(TIM3_CH1_ROW_IDX);
        }
    }
}

/*! Handler for Tim3 CH3 DMA */
void DMA1_Stream7_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream7, DMA_IT_TCIF7)) {
        DMA_ClearITPendingBit(DMA1_Stream7, DMA_IT_TCIF7);

        if(sLedDMA[TIM3_CH3_ROW_IDX].mDmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream7, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM3, TIM_DMA_CC3, DISABLE);

#if defined(WS2812_FREERTOS)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(sLedDMA[TIM3_CH3_ROW_IDX].mDmaDoneSemaphore, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
#else /* WS2812_FREERTOS */
            sLedDMA[TIM3_CH3_ROW_IDX].mDmaDone = true;
#endif /* WS2812_FREERTOS */
        } else {

            /* fill next buffer */
            fillBuffer(TIM3_CH3_ROW_IDX);

            sLedDMA[TIM3_CH3_ROW_IDX].mDmaLast = checkLastIndex(TIM3_CH3_ROW_IDX);
        }
    }
}

/*! Handler for Tim3 CH4 DMA */
void DMA1_Stream2_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream2, DMA_IT_TCIF2)) {
        DMA_ClearITPendingBit(DMA1_Stream2, DMA_IT_TCIF2);

        if(sLedDMA[TIM3_CH4_ROW_IDX].mDmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream2, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM3, TIM_DMA_CC4, DISABLE);

#if defined(WS2812_FREERTOS)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(sLedDMA[TIM3_CH4_ROW_IDX].mDmaDoneSemaphore, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
#else /* WS2812_FREERTOS */
            sLedDMA[TIM3_CH4_ROW_IDX].mDmaDone = true;
#endif /* WS2812_FREERTOS */
        } else {

            /* fill next buffer */
            fillBuffer(TIM3_CH4_ROW_IDX);

            sLedDMA[TIM3_CH4_ROW_IDX].mDmaLast = checkLastIndex(TIM3_CH4_ROW_IDX);
        }
    }
}

/*! Handler for Tim4 CH1 DMA */
void DMA1_Stream0_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream0, DMA_IT_TCIF0)) {
        DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_TCIF0);

        if(sLedDMA[TIM4_CH1_ROW_IDX].mDmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream0, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM4, TIM_DMA_CC1, DISABLE);

#if defined(WS2812_FREERTOS)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(sLedDMA[TIM4_CH1_ROW_IDX].mDmaDoneSemaphore, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
#else /* WS2812_FREERTOS */
            sLedDMA[TIM4_CH1_ROW_IDX].mDmaDone = true;
#endif /* WS2812_FREERTOS */
        } else {

            /* fill next buffer */
            fillBuffer(TIM4_CH1_ROW_IDX);

            sLedDMA[TIM4_CH1_ROW_IDX].mDmaLast = checkLastIndex(TIM4_CH1_ROW_IDX);
        }
    }
}

/*! Handler for Tim4 CH2 DMA */
void DMA1_Stream3_IRQHandler(void) {

    if(DMA_GetITStatus(DMA1_Stream3, DMA_IT_TCIF3)) {
        DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);

        if(sLedDMA[TIM4_CH2_ROW_IDX].mDmaLast) {
            /* disable dma */
            DMA_Cmd(DMA1_Stream3, DISABLE);

            /* disable timer */
            TIM_DMACmd(TIM4, TIM_DMA_CC2, DISABLE);

#if defined(WS2812_FREERTOS)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(sLedDMA[TIM4_CH2_ROW_IDX].mDmaDoneSemaphore, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
#else /* WS2812_FREERTOS */
            sLedDMA[TIM4_CH2_ROW_IDX].mDmaDone = true;
#endif /* WS2812_FREERTOS */
        } else {

            /* fill next buffer */
            fillBuffer(TIM4_CH2_ROW_IDX);

            sLedDMA[TIM4_CH2_ROW_IDX].mDmaLast = checkLastIndex(TIM4_CH2_ROW_IDX);
        }
    }
}

// eof

