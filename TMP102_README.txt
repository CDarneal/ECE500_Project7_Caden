ECE 500 TMP102 Undergraduate Project

Changes made:
- Added I2C1 HAL driver on PB8 (SCL) and PB9 (SDA).
- Added tmp102_initialize() and one-shot temperature read function.
- Updated main loop to print temperature in Celsius every second over USART2.
- Retained 32 MHz register-level clock setup and USART2 HAL printf retarget.

Important verification notes:
- The schematic note says U5 is not populated and TMP102 is U7.
- This code assumes TMP102 ADD0 is tied low (7-bit I2C address 0x48). Verify address straps on the actual PCB.
- I2C timing register is a starting value and must be validated with the actual clock and board.
- In CubeMX, inspect the .ioc after opening. The manually added i2c.c file is the authoritative initialization until CubeMX regeneration is performed.


Final review updates (2026-10-23):
- Corrected PLLR encoding: PLLR field 000 selects DIV2; HSI16 x N8 / R2 = 64 MHz SYSCLK, AHB /2 = 32 MHz HCLK/CPU.
- Added Flash latency setting for 32 MHz HCLK before switching to PLL.
- TMP102 accesses use HAL I2C memory read/write helpers.
- One-shot conversion completion is polled through the OS bit with a timeout.
- Temperature conversion explicitly sign-decodes the 12-bit two's-complement value.
- I2C timing value 0x00707CBB is retained as the project's Standard-mode value for a 32 MHz PCLK1 assumption; verify with the actual CubeMX clock tree and hardware before deployment.
- Assumes TMP102 ADD0 is tied low (7-bit address 0x48). Confirm against the board schematic.

