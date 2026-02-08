# RT1040 & ESP32 UART Cross-Platform Communication

This project implements a robust, interrupt-driven full-duplex communication link between an **NXP i.MX RT1040 (Cortex-M7)** and an **ESP32**. It solves common industrial challenges such as clock synchronization, interrupt management, and data framing.

---

## 📌 1. Hardware Interface & Wiring

| Signal | RT1040 Pin | Dir | ESP32 Pin | Logic Level |
| :--- | :--- | :---: | :--- | :--- |
| **UART TX** | GPIO_AD_B1_02 (TX2) | ➔ | GPIO 16 (RX2) | 3.3V |
| **UART RX** | GPIO_AD_B1_03 (RX2) | ⇠ | GPIO 17 (TX2) | 3.3V |
| **Ground** | GND | ⟷ | GND | 0V (Common) |

> **Warning:** A common ground is mandatory for a shared electrical reference. Without it, you will see "floating" data (garbage characters).



---

## ⚙️ 2. Deep-Dive: Technical Architecture

### A. The Clock Frequency "Truth"
The RT1040 LPUART uses a high-speed clock root. 
- **The Issue:** Many developers assume the UART runs on the 24MHz crystal. 
- **The Fact:** In the `BOARD_BootClockRUN` configuration, the **UART_CLK_ROOT** is set to **80 MHz**. 
- **The Result:** We hardcoded or dynamically linked the frequency to **80,000,000 Hz** to ensure the Baud Rate generator produced exactly **115,200 bits per second**.

### B. CMSIS Driver & Interrupts (RT1040)
Instead of "polling" (constantly checking the pin), we use an Interrupt Service Routine (ISR).
1. **LPUART2_IRQHandler:** This is the entry point in the vector table. It passes control to the NXP driver.
2. **SignalEvent:** When the hardware detects that 10 bytes have arrived, it triggers a callback to your code.
3. **Wait For Interrupt (__WFI):** The CPU stays in a low-power state until the UART hardware "wakes it up" with a full data packet.

### C. Data Framing Protocol
To prevent "byte-shifting" (where you get `A_OUTDAT` instead of `DATA_OUT`), we use a specific frame:
- **Start Byte (`!`):** Tells the receiver "a new packet starts NOW."
- **Payload:** The data content.
- **End Byte (`\n`):** Signals the end of the packet.
- **Fixed Length:** Both sides agree to send/receive exactly **10 bytes**.



---

## 🛠️ 3. Software Logic Flow

### RT1040 (The Responder)
- **Initialization:** Sets up IOMUXC daisy-chaining for the RX pin and enables the LPUART2 clock gate.
- **Buffer Management:** Uses `memset` to clear memory before every transaction to prevent "ghost" data from previous messages.
- **Null-Termination:** Manually ensures the 11th byte is `\0` so that `PRINTF` displays strings correctly without memory corruption.

### ESP32 (The Master)
- **Ping Loop:** Initiates a transfer every 2000ms.
- **Timeout Logic:** If the RT1040 fails to "Pong" within 500ms, the ESP32 flushes its circular hardware buffer to re-sync for the next attempt.

---

## ⚠️ 4. Troubleshooting Checklist
1. **Garbage Symbols?** Check `LPUART2_GetFreq`. If it doesn't match the Root Clock (80MHz), the baud rate will be wrong.
2. **Readable but shifted text?** Reset the RT1040 first, then the ESP32. This ensures the "Listener" is ready before the "Speaker" starts.
3. **No data at all?** Swap TX and RX wires. Remember: **TX(A) goes to RX(B)**.

---
*README generated for the RT1040 x ESP32 UART Interoperability Project.*# RT1040 & ESP32 UART Cross-Platform Communication

This project implements a robust, interrupt-driven full-duplex communication link between an **NXP i.MX RT1040 (Cortex-M7)** and an **ESP32**. It solves common industrial challenges such as clock synchronization, interrupt management, and data framing.

---

## 📌 1. Hardware Interface & Wiring

| Signal | RT1040 Pin | Dir | ESP32 Pin | Logic Level |
| :--- | :--- | :---: | :--- | :--- |
| **UART TX** | GPIO_AD_B1_02 (TX2) | ➔ | GPIO 16 (RX2) | 3.3V |
| **UART RX** | GPIO_AD_B1_03 (RX2) | ⇠ | GPIO 17 (TX2) | 3.3V |
| **Ground** | GND | ⟷ | GND | 0V (Common) |

> **Warning:** A common ground is mandatory for a shared electrical reference. Without it, you will see "floating" data (garbage characters).



---

## ⚙️ 2. Deep-Dive: Technical Architecture

### A. The Clock Frequency "Truth"
The RT1040 LPUART uses a high-speed clock root. 
- **The Issue:** Many developers assume the UART runs on the 24MHz crystal. 
- **The Fact:** In the `BOARD_BootClockRUN` configuration, the **UART_CLK_ROOT** is set to **80 MHz**. 
- **The Result:** We hardcoded or dynamically linked the frequency to **80,000,000 Hz** to ensure the Baud Rate generator produced exactly **115,200 bits per second**.

### B. CMSIS Driver & Interrupts (RT1040)
Instead of "polling" (constantly checking the pin), we use an Interrupt Service Routine (ISR).
1. **LPUART2_IRQHandler:** This is the entry point in the vector table. It passes control to the NXP driver.
2. **SignalEvent:** When the hardware detects that 10 bytes have arrived, it triggers a callback to your code.
3. **Wait For Interrupt (__WFI):** The CPU stays in a low-power state until the UART hardware "wakes it up" with a full data packet.

### C. Data Framing Protocol
To prevent "byte-shifting" (where you get `A_OUTDAT` instead of `DATA_OUT`), we use a specific frame:
- **Start Byte (`!`):** Tells the receiver "a new packet starts NOW."
- **Payload:** The data content.
- **End Byte (`\n`):** Signals the end of the packet.
- **Fixed Length:** Both sides agree to send/receive exactly **10 bytes**.



---

## 🛠️ 3. Software Logic Flow

### RT1040 (The Responder)
- **Initialization:** Sets up IOMUXC daisy-chaining for the RX pin and enables the LPUART2 clock gate.
- **Buffer Management:** Uses `memset` to clear memory before every transaction to prevent "ghost" data from previous messages.
- **Null-Termination:** Manually ensures the 11th byte is `\0` so that `PRINTF` displays strings correctly without memory corruption.

### ESP32 (The Master)
- **Ping Loop:** Initiates a transfer every 2000ms.
- **Timeout Logic:** If the RT1040 fails to "Pong" within 500ms, the ESP32 flushes its circular hardware buffer to re-sync for the next attempt.

---

## ⚠️ 4. Troubleshooting Checklist
1. **Garbage Symbols?** Check `LPUART2_GetFreq`. If it doesn't match the Root Clock (80MHz), the baud rate will be wrong.
2. **Readable but shifted text?** Reset the RT1040 first, then the ESP32. This ensures the "Listener" is ready before the "Speaker" starts.
3. **No data at all?** Swap TX and RX wires. Remember: **TX(A) goes to RX(B)**.

---
*README generated for the RT1040 x ESP32 UART Interoperability Project.*