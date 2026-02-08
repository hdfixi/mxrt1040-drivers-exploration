#include "board.h"
#include "app.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_lpuart_cmsis.h"
#include "fsl_edma.h"
#include "fsl_dmamux.h"

/* --- Driver & DMA Hardware --- */
#define USART_DRV Driver_USART2
extern ARM_DRIVER_USART Driver_USART2;
extern void LPUART2_DriverIRQHandler(void);

#define EXAMPLE_LPUART_DMA_MUX_BASE DMAMUX
#define EXAMPLE_LPUART_DMA_BASE     DMA0

/* --- Memory (NON-CACHEABLE IS MANDATORY FOR DMA) --- */
#define MSG_LEN 10

/* * We use AT_NONCACHEABLE_SECTION_ALIGN so the DMA hardware and the CPU
 * look at the same physical RAM. 32-byte alignment is required for eDMA.
 */
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t rxBuffer[MSG_LEN + 1], 32);
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t txBuffer[MSG_LEN + 1], 32);

volatile bool rxDone = false;

/* Frequency remains 80MHz */
uint32_t LPUART2_GetFreq(void) {
    return BOARD_BOOTCLOCKRUN_UART_CLK_ROOT;
}

/* ISR is still needed to tell the CMSIS driver that a DMA transfer finished */
void LPUART2_IRQHandler(void) {
    LPUART2_DriverIRQHandler();
}

/**
 * @brief This is where we prepare the DMA "Pipes"
 */
void LPUART2_InitPins(void) {
    BOARD_InitPins();

    /* Start the DMA Mux (The switchboard) */
    DMAMUX_Init(EXAMPLE_LPUART_DMA_MUX_BASE);

    /* Start the eDMA Engine (The muscle) */
    edma_config_t edmaConfig;
    EDMA_GetDefaultConfig(&edmaConfig);
    EDMA_Init(EXAMPLE_LPUART_DMA_BASE, &edmaConfig);
}

void LPUART2_DeinitPins(void) {}

void USART_SignalEvent(uint32_t event) {
    if (event & ARM_USART_EVENT_RECEIVE_COMPLETE) {
        rxDone = true;
    }
}

int main(void) {
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    CLOCK_EnableClock(kCLOCK_Lpuart2);

    /* Initialize Driver - The driver uses DMA channels configured in RTE_Device.h */
    USART_DRV.Initialize(USART_SignalEvent);
    USART_DRV.PowerControl(ARM_POWER_FULL);
    USART_DRV.Control(ARM_USART_MODE_ASYNCHRONOUS, 115200);

    PRINTF("RT1040 DMA Mode Active. CPU will sleep during transfer...\r\n");

    while (1) {
        rxDone = false;
        memset(rxBuffer, 0, sizeof(rxBuffer));

        /* * STEP 1: Start DMA Receive.
         * This function tells the DMA: "Go grab 10 bytes from UART and put them in rxBuffer."
         * The function returns IMMEDIATELY.
         */
        USART_DRV.Receive(rxBuffer, MSG_LEN);

        /* * STEP 2: CPU SLEEP.
         * Instead of checking a flag constantly (polling), we use __WFI().
         * The CPU physically stops executing code here. It consumes almost no power.
         * The DMA engine is the ONLY thing running, moving bytes in the background.
         */
        while (!rxDone) {
            __WFI(); // CPU sleeps here until the DMA finishes and triggers an interrupt
        }

        /* STEP 3: Process the data now that DMA is done */
        PRINTF("DMA Finished! Received: %s\r\n", rxBuffer);

        memcpy(txBuffer, rxBuffer, MSG_LEN);
        USART_DRV.Send(txBuffer, MSG_LEN);
    }
}
