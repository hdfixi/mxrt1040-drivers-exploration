#include "fsl_device_registers.h"   // MCU register definitions
#include "fsl_debug_console.h"      // PRINTF / debug console
#include "fsl_adc.h"                // ADC driver
#include "fsl_edma.h"               // eDMA driver
#include "fsl_dmamux.h"             // DMAMUX driver
#include "clock_config.h"           // Clock configuration
#include "board.h"                  // Board-specific init
#include "pin_mux.h"                // Pin multiplexing

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ADC_DMA_CHANNEL      3       // eDMA channel used for ADC transfers
#define ADC_BUFFER_SIZE      20      // Number of ADC samples per DMA major loop

/*******************************************************************************
 * Globals
 ******************************************************************************/
/*
 * ADC DMA buffer
 * - Non-cacheable: avoids cache coherency issues between CPU and DMA
 * - 32-byte aligned: matches Cortex-M7 cache line size
 *   (prevents cache line corruption)
 */
AT_NONCACHEABLE_SECTION_ALIGN(static uint16_t adcBuffer[ADC_BUFFER_SIZE], 32);

/* eDMA channel handle */
static edma_handle_t g_AdcDmaHandle;

/*
 * Flag set by DMA interrupt when one full buffer
 * (ADC_BUFFER_SIZE samples) has been transferred
 */
volatile bool g_AdcBufferFull = false;

/*******************************************************************************
 * DMA Callback
 ******************************************************************************/
/*
 * This callback is executed when the DMA major loop completes
 * (i.e. ADC_BUFFER_SIZE samples have been transferred).
 */
void AdcDmaCallback(edma_handle_t *handle,
                    void *userData,
                    bool transferDone,
                    uint32_t tcds)
{
    if (transferDone)
    {
        /*
         * Signal main loop that a full ADC buffer is ready.
         * Do NOT do heavy processing or PRINTF here.
         */
        g_AdcBufferFull = true;

        /*
         * Restart the DMA major loop.
         * Without this, the DMA channel would stop accepting
         * new ADC conversion requests after one buffer.
         *
         * ADC continues converting continuously.
         * DMA is now re-armed to capture the next buffer.
         */
        EDMA_StartTransfer(handle);
    }
}

/*******************************************************************************
 * ADC + DMA Init
 ******************************************************************************/
static void ADC_DMA_Init(void)
{
    adc_config_t adcConfig;                  // ADC global configuration
    adc_channel_config_t adcChannelConfig;   // ADC channel configuration
    edma_config_t dmaConfig;                 // eDMA global configuration
    edma_transfer_config_t transferConfig;   // eDMA transfer configuration

    /* Enable peripheral clocks */
    CLOCK_EnableClock(kCLOCK_Adc1);          // Enable ADC1 clock
    CLOCK_EnableClock(kCLOCK_Dma);           // Enable DMA clock

    /**************** ADC CONFIG ****************/
    ADC_GetDefaultConfig(&adcConfig);        // Load default ADC configuration

    /*
     * Continuous conversion:
     * - ADC keeps converting back-to-back
     * - Each conversion generates a DMA request
     */
    adcConfig.enableContinuousConversion = true;

    /*
     * Allow ADC result register overwrite if DMA is late.
     * Safe for streaming use cases.
     */
    adcConfig.enableOverWrite = true;

    /*
     * High-speed mode disabled to reduce bus pressure
     * and avoid starving CPU / DMA.
     */
    //adcConfig.enableHighSpeed = true;

    ADC_Init(ADC1, &adcConfig);              // Initialize ADC hardware
    ADC_DoAutoCalibration(ADC1);             // Perform ADC calibration
    ADC_EnableDMA(ADC1, true);               // Enable DMA requests from ADC

    /* Configure ADC channel */
    adcChannelConfig.channelNumber = 3;      /* ADC1 channel 3 */
    adcChannelConfig.enableInterruptOnConversionCompleted = false;
    /* No ADC interrupt: DMA handles data movement */

    /**************** DMAMUX CONFIG ****************/
    DMAMUX_Init(DMAMUX);                     // Initialize DMAMUX
    /*
     * Route ADC1 conversion complete signal
     * to eDMA channel ADC_DMA_CHANNEL
     */
    DMAMUX_SetSource(DMAMUX, ADC_DMA_CHANNEL, kDmaRequestMuxADC1);
    DMAMUX_EnableChannel(DMAMUX, ADC_DMA_CHANNEL);

    /**************** EDMA CONFIG ****************/
    EDMA_GetDefaultConfig(&dmaConfig);       // Load default DMA config
    EDMA_Init(DMA0, &dmaConfig);             // Initialize DMA controller

    /* Create DMA handle for the selected channel */
    EDMA_CreateHandle(&g_AdcDmaHandle, DMA0, ADC_DMA_CHANNEL);

    /* Register DMA callback for major loop completion */
    EDMA_SetCallback(&g_AdcDmaHandle, AdcDmaCallback, NULL);

    /**************** DMA TRANSFER SETUP ****************/
    /*
     * Configure DMA transfer:
     * - Source: ADC result register
     * - Destination: adcBuffer[]
     * - Minor loop: 1 ADC sample (16 bits)
     * - Major loop: ADC_BUFFER_SIZE samples
     */
    EDMA_PrepareTransfer(
        &transferConfig,
        (void *)&ADC1->R[0],                 /* Source: ADC result register */
        sizeof(uint16_t),
        adcBuffer,                           /* Destination buffer */
        sizeof(uint16_t),
        sizeof(uint16_t),                    /* Minor loop = 1 sample */
        ADC_BUFFER_SIZE * sizeof(uint16_t),  /* Major loop size */
        kEDMA_PeripheralToMemory
    );

    /* Submit transfer configuration to DMA channel */
    EDMA_SubmitTransfer(&g_AdcDmaHandle, &transferConfig);

    /*
     * Enable interrupt at the end of each major loop
     * (i.e. after ADC_BUFFER_SIZE samples)
     */
    EDMA_EnableChannelInterrupts(
            DMA0,
            ADC_DMA_CHANNEL,
            kEDMA_MajorInterruptEnable
        );

    /* Start the DMA channel */
    EDMA_StartTransfer(&g_AdcDmaHandle);

    /**************** START FIRST ADC CONVERSION ****************/
    /*
     * Writing channel configuration triggers
     * the first ADC conversion.
     * After that, ADC runs continuously.
     */
    ADC_SetChannelConfig(ADC1, 0, &adcChannelConfig);
}

/*******************************************************************************
 * Main
 ******************************************************************************/
int main(void)
{
    BOARD_InitBootPins();                    // Initialize pins
    BOARD_InitBootClocks();                  // Initialize clocks
    BOARD_InitDebugConsole();                // Initialize UART / PRINTF

    PRINTF("Software-triggered ADC + DMA example (manual re-trigger)\r\n");

    ADC_DMA_Init();                          // Initialize ADC + DMA pipeline

    /*
     * Local buffer used to copy DMA data before printing.
     * Prevents reading data while DMA is overwriting it.
     */
    uint16_t localCopy[ADC_BUFFER_SIZE];

    while (1)
    {
        if (g_AdcBufferFull)
        {
            /* Clear flag first to avoid missing next buffer */
            g_AdcBufferFull = false;

            /*
             * Copy ADC samples locally.
             * DMA may overwrite adcBuffer while we print.
             */
            memcpy(localCopy, adcBuffer, sizeof(localCopy));

            /*
             * Print only summary values.
             * Printing too much data would stall the system.
             */
            PRINTF("First sample = %d, Last sample = %d\r\n",
                   localCopy[0],
                   localCopy[ADC_BUFFER_SIZE - 1]);

            /*
             * Artificial delay to slow down printing.
             * ADC + DMA keep running during this delay.
             */
            for (volatile uint32_t i = 0; i < 10000000; i++) __NOP();
        }
    }
}
