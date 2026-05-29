# Vendored Arduino Libraries

These libraries are copied from the local Arduino sketchbook so the owlController
firmware can be built without relying on user-global Arduino libraries.

Build with the local libraries:

```sh
./verify_local_libraries.sh
```

The script sets `sketchbook.path` to this directory, so Arduino resolves
`libraries/` here before the global sketchbook. Board core libraries such as
`Wire` and `SPI` still come from the installed RP2040 Arduino core.

Included libraries:

- TCA9548A 1.1.3
- MCP342x 1.0.4
- Adafruit_NeoPixel 1.11.0
- RP2040_PWM 1.7.0
- NeoPixelConnect 1.3.2
- Adafruit_ILI9341 1.6.2
- Adafruit_GFX_Library 1.12.1
- Adafruit_BusIO 1.17.1
- Adafruit_SSD1306 2.5.14
