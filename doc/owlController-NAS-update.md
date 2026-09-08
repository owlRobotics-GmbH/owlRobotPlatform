# owlController RP2040: NAS source update (2026-09-08)

This update imports the four modified source files from the NAS archive
`owlController_RP2040_00.zip`. Backup files are excluded.

## Output startup behavior

OUT1, OUT2 and OUT3 remain inactive until 3000 ms after the Pi power-on
command, then become active. The delay is configured with
`OUT_GND_AFTER_PI_POWER_ON_MS` in `config.h`. The PCA9555 drives the
ULN2803, whose active outputs sink to GND. These three outputs no longer
mirror their corresponding inputs. OUT4 retains its input-mirroring behavior.

## Additional CAN management status values

The controller now answers these management requests:

| Value ID | Payload |
| --- | --- |
| 46 | Byte 0: input flags; byte 1: output flags; byte 2: rain detected (0/1) |
| 47 | Rain sensor millivolts, 32-bit float in RP2040 byte order |
| 48 | Battery voltage in volts, 32-bit float in RP2040 byte order |
| 49 | Charger voltage in volts, 32-bit float in RP2040 byte order |

Input flag bits 0..3 represent bumper, stop, lift and slow-down respectively.
Output flag bits 0..3 represent OUT1..OUT4. Status is refreshed by the
existing approximately one-second sensor update loop.

## Filesystem initialization

If LittleFS fails to mount, the controller formats it and retries mounting.
Formatting removes existing LittleFS contents.

## Version and build status

The imported source preserves management firmware version `2` and the
legacy `VERS` string. These values do not uniquely identify this source update;
use the Git commit for provenance. The prebuilt UF2 in `tools/flash` is not
rebuilt by this source import and must not be assumed to contain these changes.
The import was checked against the archive and with `git diff --check`;
compilation and hardware validation were not performed.
