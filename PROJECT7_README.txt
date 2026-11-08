ECE 500 - PROJECT 7: TMP102 / SPI FRAM TEMPERATURE LOGGER

Target: NUCLEO-G071RB (STM32G071RBTx), 32 MHz system clock

FUNCTIONALITY
- TMP102 temperature sensor U7 communicates over I2C1:
  PB8 = I2C1_SCL, PB9 = I2C1_SDA.
- The TMP102 is kept in shutdown between readings. Each sample starts a
  one-shot conversion. START enables one sample per second.
- SPI1 communicates with the 8,192-byte MB85RS64V FRAM:
  PB3 = SPI1_SCK, PA6 = SPI1_MISO, PA7 = SPI1_MOSI.
  PB0 = active-low FRAM chip select; PC7 = write-protect control (high/inactive).
- USART2 provides the Tera Term command line at 115200 baud, 8-N-1.

UART COMMANDS
START  Start one-sample-per-second logging. Existing data is preserved and
       new samples append after the saved last record, including after reboot.
STOP   Stop automatic sampling without erasing saved data.
TEMP   Display saved temperatures, number of data points, and first/last
       data addresses, as specified by assignment_SPI.
DUMP   Alias for TEMP.
NOW    Take and display one immediate reading without storing it in FRAM.
STATUS Display logger state and saved FRAM pointers.
CLEAR  Stop logging and erase all 8,192 FRAM bytes, including metadata.
HELP   Display the command list.

FRAM MEMORY MAP (assignment_SPI format)
0x0000-0x0001: first temperature-record address, unsigned 16-bit, big-endian.
0x0002-0x0003: last temperature-record address, unsigned 16-bit, big-endian.
0x0004-0x1FFF: two-byte temperature records.

Each record stores the exact 16-bit TMP102 temperature register value in
big-endian order. The TMP102 register contains a left-justified 12-bit,
two's-complement temperature value. Two zero metadata pointers mean that no
records are stored. Otherwise, the pointers identify the first and last
record start addresses (inclusive). Records are spaced two bytes apart.
There are 4,094 available samples (about 68 minutes at one sample per second).
When FRAM fills, logging stops safely.

IMPLEMENTATION NOTES
- The I2C handle and MX_I2C1_Init() are defined only in Core/Src/main.c.
- I2C MSP pin/clock initialization is defined only in
  Core/Src/stm32g0xx_hal_msp.c. There is no duplicate Core/Src/i2c.c module.
- The FRAM driver validates read/write address ranges and sends WREN as a
  separate chip-select transaction before each write.
- SPI1 is configured as an 8-bit master, mode 0 (CPOL=0, CPHA=0), software NSS;
  the FRAM chip select is controlled manually on PB0.
- TMP102 uses the 7-bit I2C address 0x48 (ADD0 tied to GND).
- I2C timing 0x00B07CB4 is retained from the supplied CubeMX project for a
  32 MHz I2C kernel clock. Verify timing if the clock tree or bus speed changes.
- FRAM data and pointers survive a power cycle. Enter START after reboot to
  resume appending to the existing log.

BUILD AND OPERATION
Import the project folder into STM32CubeIDE, then use Project > Clean followed
by Build. Connect Tera Term to the NUCLEO ST-LINK virtual COM port at 115200
8-N-1. Available commands include NOW, START, STATUS, STOP, TEMP, DUMP,
CLEAR, and HELP.
