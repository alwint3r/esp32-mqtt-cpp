# C++ Wrapper Library for ESP32 MQTT Client (esp-mqtt)

This library wraps esp-mqtt library in a C++ class.

## Stability

Lack of documentation and examples on my part. Go ahead take a look at the code and try it on your own risk.
Please contribute if you find this wrapper library useful.

## C++17

This library requires C++17 or C++17 with GNU extensions (gnu++17) to be enabled in the compiler flags.


### Enabling C++17 in PlatformIO

Add the following lines to the `platformio.ini` file.

```ini
build_unflags = -std=gnu++11
build_flags = -std=gnu++17
```

### Enabling C++17 in Arduino IDE

No idea how. Please open a PR to add the instruction.
