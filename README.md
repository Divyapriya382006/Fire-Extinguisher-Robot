# Autonomous Fire Detection, Localization and Suppression Robot

A low-cost Arduino-based firefighting robot that detects a fire, navigates to it while avoiding obstacles, sprays water, and then **verifies** the fire is out. If the fire persists after a few attempts, it alerts a human operator.

Developed by a team of 5.

## Why this project

Most existing firefighting robots treat suppression as a single action: they spray once and stop, with no check that the fire is actually out and no retry. This robot closes that loop.

## Features

- **Fire localization** with three flame sensors (left, centre, right)
- **Obstacle avoidance** with an HC-SR04 ultrasonic sensor
- **Water-based suppression** via a submersible pump and nozzle
- **Verification and retry:** flame sensors are re-read after each spray, with up to 2-3 attempts
- **Alert state:** RGB LED and buzzer signal when human help is needed
- **Low cost:** no thermal camera, LiDAR or SLAM

## How it works

```
Sense -> Navigate -> Spray -> Verify --+--> Fire out: safe state
                                       +--> Still burning: spray again
                                       +--> Attempt limit reached: alert
```

1. The sensor with the strongest flame reading gives the fire's direction.
2. The robot turns toward the fire, steering around obstacles detected by the ultrasonic sensor.
3. At the fire zone it stops and runs the pump (switched by a logic-level MOSFET with a flyback diode).
4. It re-checks the flame sensors. No flame means done; flame present means retry or alert.

## Hardware

| Component | Purpose |
|---|---|
| Arduino Uno R3 | Main controller |
| 3x flame sensor modules | Fire detection and direction |
| HC-SR04 ultrasonic sensor | Obstacle detection |
| Continuous-rotation servos, 2WD chassis, wheels | Drive |
| 3-6 V submersible pump, nozzle, reservoir | Suppression |
| Logic-level N-MOSFET + 1N4007 diode | Pump switching and protection |
| RGB LED + active buzzer | Status and alert |
| 5 V 2 A buck converter, 4.8-6 V battery pack | Power |

**Software:** Arduino IDE, Arduino C/C++, `Servo.h`

## Limitations

- Suited to small, accessible fires only; not for electrical or chemical fires
- Flame sensor range and accuracy depend on the environment
- Limited water reservoir
- No mapping, so less reliable in complex layouts

## Future scope

Thermal imaging, SLAM mapping, foam/dry powder agents, camera- or weight-based confirmation, IoT/GSM alerts, multi-robot coordination.

## Demo and slides

[Google Drive folder](https://drive.google.com/drive/folders/1WHj0e7zWW0xLIk137TNwPztTDNevHHd7?usp=sharing)
