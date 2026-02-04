# i.MX RT1040 eFlexPWM Deep-Dive: Breathing LED

This project implements a high-precision "breathing" LED effect on the **NXP i.MX RT1040 EVK**. It bypasses standard architectural "failsafes" that typically keep PWM outputs inactive during development.

## 📍 Hardware Mapping
* **Physical Pin:** GPIO_AD_B0_10
* **Board Label:** Arduino Header **D5**
* **Power Domain:** `NVCC_GPIO` (3.3V)
* **Hardware Instance:** `PWM1`
* **Submodule Index:** `3`
* **Signal Output:** `Channel A` (PWMA)



## 🏗 System Architecture & Logic

### 1. The Clock & Power Bridge
Unlike simpler MCUs, the RT1040 gates all peripheral clocks. 
* **IPG Clock:** The PWM logic runs on the Peripheral Clock. 
* **Isolation:** We use `GPIO_AD_B0_10` because it sits on the primary GPIO power rail. Previous attempts on `GPIO_SD` pins failed because the SD power regulator is disabled by default to protect memory cards.

### 2. The eFlexPWM Submodule Pipeline
The eFlexPWM is a sophisticated timing engine. To create the "breath," we manipulate the **VAL** registers:
* **VAL0/VAL1:** Define the frequency (1kHz).
* **VAL2/VAL3:** Define the "Turn-ON" and "Turn-OFF" points. By moving VAL3 relative to VAL2, we change the **Duty Cycle**.



### 3. Critical Register Overrides (The "Why")
The i.MX RT series features industrial-grade safety logic that often blocks PWM signals during initial testing:

| Register | Technical Purpose | Why it was needed |
| :--- | :--- | :--- |
| **FSTS** | Fault Status Clear | Floating pins on the EVK can "trip" a fault, locking the PWM. Writing `0xF` clears these latches. |
| **DISMAP** | Fault Disable Map | Disconnects the PWM from the internal "Emergency Stop" logic, allowing the signal to reach the pin. |
| **OUTEN** | Output Enable | The physical "gate" between the PWM engine and the I/O pad. We force this HIGH for Submodule 3. |
| **CTRL2[DBGEN]** | Debug Enable | Prevents the PWM timer from freezing when you hit a breakpoint in your IDE. |



## 💻 Software Implementation

### Electrical Pad Configuration
The pin is configured with **ALT2** muxing and a specific pad setting of `0x10B0`:
* **Speed:** 100MHz (Ensures clean square wave edges).
* **Drive Strength:** R0/6 (Provides sufficient current for the LED).
* **Pull:** 100K Ohm Pull-down (Prevents flickering during boot).
# i.MX RT1040 eFlexPWM Deep-Dive: Breathing LED

This project implements a high-precision "breathing" LED effect on the **NXP i.MX RT1040 EVK**. It bypasses standard architectural "failsafes" that typically keep PWM outputs inactive during development.

## 📍 Hardware Mapping
* **Physical Pin:** GPIO_AD_B0_10
* **Board Label:** Arduino Header **D5**
* **Power Domain:** `NVCC_GPIO` (3.3V)
* **Hardware Instance:** `PWM1`
* **Submodule Index:** `3`
* **Signal Output:** `Channel A` (PWMA)



## 🏗 System Architecture & Logic

### 1. The Clock & Power Bridge
Unlike simpler MCUs, the RT1040 gates all peripheral clocks. 
* **IPG Clock:** The PWM logic runs on the Peripheral Clock. 
* **Isolation:** We use `GPIO_AD_B0_10` because it sits on the primary GPIO power rail. Previous attempts on `GPIO_SD` pins failed because the SD power regulator is disabled by default to protect memory cards.

### 2. The eFlexPWM Submodule Pipeline
The eFlexPWM is a sophisticated timing engine. To create the "breath," we manipulate the **VAL** registers:
* **VAL0/VAL1:** Define the frequency (1kHz).
* **VAL2/VAL3:** Define the "Turn-ON" and "Turn-OFF" points. By moving VAL3 relative to VAL2, we change the **Duty Cycle**.



### 3. Critical Register Overrides (The "Why")
The i.MX RT series features industrial-grade safety logic that often blocks PWM signals during initial testing:

| Register | Technical Purpose | Why it was needed |
| :--- | :--- | :--- |
| **FSTS** | Fault Status Clear | Floating pins on the EVK can "trip" a fault, locking the PWM. Writing `0xF` clears these latches. |
| **DISMAP** | Fault Disable Map | Disconnects the PWM from the internal "Emergency Stop" logic, allowing the signal to reach the pin. |
| **OUTEN** | Output Enable | The physical "gate" between the PWM engine and the I/O pad. We force this HIGH for Submodule 3. |
| **CTRL2[DBGEN]** | Debug Enable | Prevents the PWM timer from freezing when you hit a breakpoint in your IDE. |



## 💻 Software Implementation

### Electrical Pad Configuration
The pin is configured with **ALT2** muxing and a specific pad setting of `0x10B0`:
* **Speed:** 100MHz (Ensures clean square wave edges).
* **Drive Strength:** R0/6 (Provides sufficient current for the LED).
* **Pull:** 100K Ohm Pull-down (Prevents flickering during boot).

### The Breathing Algorithm
We use a **triangular wave function** to modulate brightness:
1.  Initialize duty cycle at `0%`.
2.  Increment by `1%` every `25ms` until `100%` is reached.
3.  Decrement back to `0%`.
4.  **Sync:** Use `PWM_SetPwmLdok` after every update. This ensures the hardware only updates the brightness at the start of a new PWM period, preventing visual glitches.



## ⚠️ Hardware Connection Note
* **Anode (+):** Connect to Pin **D5**.
* **Cathode (-):** Connect to **GND** (Available on the same Arduino header).
* **Resistor:** Use a **220Ω - 1kΩ** resistor in series to protect the processor's GPIO pad from overcurrent.

---
*Documented for MCUXpresso SDK & i.MX RT1040 Reference Manual Revision 3.*
### The Breathing Algorithm
We use a **triangular wave function** to modulate brightness:
1.  Initialize duty cycle at `0%`.
2.  Increment by `1%` every `25ms` until `100%` is reached.
3.  Decrement back to `0%`.
4.  **Sync:** Use `PWM_SetPwmLdok` after every update. This ensures the hardware only updates the brightness at the start of a new PWM period, preventing visual glitches.



## ⚠️ Hardware Connection Note
* **Anode (+):** Connect to Pin **D5**.
* **Cathode (-):** Connect to **GND** (Available on the same Arduino header).
* **Resistor:** Use a **220Ω - 1kΩ** resistor in series to protect the processor's GPIO pad from overcurrent.

---
*Documented for MCUXpresso SDK & i.MX RT1040 Reference Manual Revision 3.*