# ESP32-P4 board power test fixture

This is a standalone Arduino sketch for the board in the supplied schematic.
It uses GPIO53/GPIO54 for I2C and controls U26 (TCA9555, address `0x20`).
GPIO8 (`VE_CTRL`) controls the upstream VEXT regulator and GPIO45 (`CHIP_EN`)
controls the external ESP32-C6. The sketch enables VEXT before accessing U26,
then disables the downstream rails before shutting VEXT off.
The SX1262 is connected to GPIO11..17; `min` sends the SX126x `SetSleep`
command (`0x84, 0x00`) before entering ESP32-P4 Deep-sleep, while `max` wakes
it to Standby. This is not continuous LoRa TX/RX; add a radio workload when a
radio-transmit peak is required.

Serial commands at 115200 baud:

- `min`: switch off board rails and enter indefinite Deep-sleep. Press EN/RESET to leave it.
- `max`: switch on board rails, set the CPU target frequency, and run all available FreeRTOS cores at full load.
- `idle`: keep the board rails off but remain awake.
- `status`: print CPU/core/TCA status.

The expander mapping comes from the schematic:

| TCA pin | Signal | Function |
|---|---|---|
| P4 | `AU_CTRL` | Audio control |
| P5 | `VFEM_EN` | V-FEM enable |
| P6 | `TFT_LEDK` | TFT backlight |
| P7 | `TFT_CTRL` | TFT power/control |
| P10 | `PA_EN` | Power amplifier enable |
| P11 | `GPS_CTRL` | GPS control |
| P12 | `TP_RST` | Touch reset |
| P16 | `SD_PWRN` | SD power-down |
| P17 | `SDA_EN` | SDA/power enable |

The schematic page does not unambiguously show the active level for every
control signal. Verify the `*_ACTIVE` definitions near the top of the sketch
against the regulator/power-tree pages. `TP_RST` is assumed active-low.
`SD_PWRN` is currently assumed active-high because the SD sheet shows it
driving a power/discharge switch; verify this before measuring. The other
listed controls are assumed active-high.

Set `POWER_TEST_BOOT_MODE` to `1` or `2` for unattended current-analyser runs.
Disconnect USB-UART and any external device that can back-power the board when
measuring minimum current. The `max` mode enables rails and saturates the CPU;
it does not automatically perform TFT refresh, GPS tracking, SD I/O, or LoRa
transmission. Add those workloads in `enterMaximum()` if their peak current is
also required.
