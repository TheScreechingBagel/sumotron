# Mechatron Sumo Robot

1. Install [PlatformIO](https://platformio.org/)
    i'm just using the cli [PlatformIO Core](https://platformio.org/install/cli), u don't need the ide

2. connect the ESP.

## runneing

```sh
pio run --target upload

pio run -t uploadfs -t monitor
```

mmnh yea

## run checks
```sh
pio check
```
maybe `pio run -t clean` also


## 'heavily inspired' by:
- https://github.com/donskytech/platformio-projects/tree/main/esp32-projects/esp32-robot-car-websockets
- the provided example code from competition
