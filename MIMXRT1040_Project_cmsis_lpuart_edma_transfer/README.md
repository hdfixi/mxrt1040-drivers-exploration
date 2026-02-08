# RT1040 to ESP32: High-Performance DMA UART Link

## 📝 Project Summary
This project implements a professional-grade UART communication system between an **i.MX RT1040 (Cortex-M7)** and an **ESP32**. It moves away from CPU-heavy "Interrupt" methods to **Direct Memory Access (DMA)**, allowing the RT1040 to handle high-speed data while keeping the CPU idle.

---

## 📌 1. Hardware Connections (Pinout)

| Signal Name | RT1040 Pin | Direction | ESP32 Pin | Logic Level |
| :--- | :--- | :---: | :--- | :--- |
| **UART TX** | GPIO_AD_B1_02 | ----> | GPIO 16 (RX2) | 3.3V |
| **UART RX** | GPIO_AD_B1_03 | <---- | GPIO 17 (TX2) | 3.3V |
| **Ground** | GND | <---> | GND | 0V |

**Note:** Ensure a common ground is connected. Without it, the reference voltage will float, resulting in data corruption.

---

## 🚀 2. Technical Architecture

### A. DMA (Direct Memory Access)
- **Efficiency:** The CPU load is reduced to near 0% during the actual data transfer.
- **Mechanism:** We used the **eDMA engine** and **DMAMUX0**. The DMA controller is programmed to watch the LPUART2 FIFO and move bytes directly into RAM as they arrive.
- **CPU Sleep:** By using `__WFI()` (Wait For Interrupt) in the main loop, the CPU physically pauses execution while the DMA hardware works in the background.

### B. Cache Coherency (The M7 Challenge)
The Cortex-M7 features an L1 Data Cache. To prevent the CPU from reading "stale" data while the DMA writes to physical RAM, we used:
- **`AT_NONCACHEABLE_SECTION_ALIGN`**: This places the RX/TX buffers in a memory region that the CPU is not allowed to cache.
- **Alignment:** Buffers are aligned to 32-byte boundaries to satisfy eDMA burst requirements.



### C. Clock Configuration
- **UART Root:** Verified as **80 MHz** (`BOARD_BOOTCLOCKRUN_UART_CLK_ROOT`). 
- **Baud Rate:** Set to **115,200 bps**. Providing the correct 80MHz "Ground Truth" to the CMSIS driver is what eliminated garbage characters.

---

## 🛠 3. Software Logic Flow

1. **Initialization:** The system configures the DMAMUX to route LPUART2 signals to eDMA channels.
2. **DMA Receive:** The command `USART_DRV.Receive(rxBuffer, 10)` starts the DMA engine. This function returns immediately.
3. **Hardware Transfer:** As the ESP32 sends bytes, the DMA engine fills the buffer. The CPU remains in a low-power "Wait" state.
4. **Completion:** Once 10 bytes are received, the DMA triggers a single interrupt to wake the CPU.
5. **Processing:** The CPU clears the buffer, prints the data to the console, and triggers a DMA-based "Pong" back to the ESP32.



---

## ⚠️ 4. Troubleshooting
- **Garbage Characters:** Confirm `LPUART2_GetFreq()` returns `80000000U`.
- **Old Data Appearing:** Ensure buffers are defined with `AT_NONCACHEABLE_SECTION_ALIGN`. If they are in standard RAM, the CPU Cache will mask new data.
- **No Response:** Check `RTE_Device.h` to ensure `RTE_USART2_RX_DMA` and `RTE_USART2_TX_DMA` are enabled.

---
*README generated for the i.MX RT1040 DMA-UART Project.*