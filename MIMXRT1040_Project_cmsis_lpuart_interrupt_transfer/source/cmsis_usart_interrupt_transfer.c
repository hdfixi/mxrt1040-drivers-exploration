#include "board.h"
#include "app.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_lpuart_cmsis.h"

/* --- Driver Configuration --- */
#define USART_DRV Driver_USART2       // Use CMSIS LPUART2 Driver
extern ARM_DRIVER_USART Driver_USART2;
extern void LPUART2_DriverIRQHandler(void); // Link to the SDK Internal Handler

/* --- Communication Settings --- */
#define MSG_LEN 10                    // Expecting "!PING_123\n"
// 1. Make buffer 1 byte larger for the 'Stop' marker
uint8_t rxBuffer[MSG_LEN + 1];           // Memory to store incoming data
volatile bool rxDone = false;         // Shared flag between Interrupt and Main

/**
 * @brief Frequency Helper for the CMSIS Driver.
 * The driver uses this value to calculate the Baud Rate registers.
 * We use 80MHz (UART_CLK_ROOT) as discovered in clock_config.h.
 */
uint32_t LPUART2_GetFreq(void) {
    return BOARD_BOOTCLOCKRUN_UART_CLK_ROOT;
}

/**
 * @brief Hardware Interrupt Service Routine.
 * Triggered by the CPU whenever a byte is sent or received.
 */
void LPUART2_IRQHandler(void) {
    LPUART2_DriverIRQHandler();
}

/**
 * @brief Pin initialization wrapper.
 * The CMSIS driver calls this automatically during Initialize().
 */
void LPUART2_InitPins(void) {
    BOARD_InitPins();
}

void LPUART2_DeinitPins(void) {} // Required by linker, but unused

/**
 * @brief Signal Event Callback.
 * This function is called by the Driver (via Interrupt) when a task finishes.
 */
void USART_SignalEvent(uint32_t event) {
    if (event & ARM_USART_EVENT_RECEIVE_COMPLETE) {
        rxDone = true; // Signal 'main' that the buffer is full
    }
}

int main(void) {
    /* 1. Basic Hardware Setup */
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole(); // For PC Monitor

    /* 2. UART Driver Setup */
    CLOCK_EnableClock(kCLOCK_Lpuart2);
    USART_DRV.Initialize(USART_SignalEvent);
    USART_DRV.PowerControl(ARM_POWER_FULL);
    USART_DRV.Control(ARM_USART_MODE_ASYNCHRONOUS, 115200);

    PRINTF("RT1040 Ping-Pong (80MHz Root) Ready...\r\n");

    while (1) {
    	rxDone = false;
    	    memset(rxBuffer, 0, sizeof(rxBuffer)); // Clear the buffer of old data

    	    USART_DRV.Receive(rxBuffer, MSG_LEN);
    	    while (!rxDone) { __WFI(); }

    	    // rxBuffer[10] is already 0 because of memset,
    	    // so PRINTF will stop exactly at the 10th character.
    	    PRINTF("Received: %s", rxBuffer);

    	    USART_DRV.Send(rxBuffer, MSG_LEN);
    	    PRINTF(" -> Successfully Echoed.\r\n");
    	}
}
