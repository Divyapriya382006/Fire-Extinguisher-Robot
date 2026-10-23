# Autonomous Fire Detection, Localization and Suppression Robot

A low-cost autonomous firefighting robot built on the Arduino Uno. It detects and localizes a fire, navigates toward it while avoiding obstacles, suppresses it with a water pump, and then **verifies** whether the fire is actually out. If the fire persists after a limited number of attempts, it raises an alert for human intervention.

---

## Table of Contents

- [Problem Statement](#problem-statement)
- [Objectives](#objectives)
- [Key Features](#key-features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [System Architecture](#system-architecture)
- [Methodology](#methodology)
- [Status Indications](#status-indications)
- [Intended Outcomes](#intended-outcomes)
- [Comparison with Existing Systems](#comparison-with-existing-systems)
- [Limitations](#limitations)
- [Applications](#applications)
- [Future Scope](#future-scope)
- [Demo and Presentation](#demo-and-presentation)
- [References](#references)

---

## Problem Statement

Fire incidents in industrial, commercial and other hazardous environments put firefighters at significant risk, while a rapid response is critical to stop a fire from spreading. Many existing firefighting robots:

1. Cannot accurately determine the direction of the fire.
2. Cannot navigate autonomously around obstacles.
3. Cannot verify whether the fire has actually been extinguished.

A review of ten existing systems (from low-cost Arduino platforms to SLAM-based and neural-network-driven robots) shows that suppression is generally treated as a single terminal action: once the extinguishing agent is discharged, the control process ends with no reassessment and no retry or failure handling.

This project addresses that gap with a low-cost robot that detects, localizes, approaches and suppresses a fire, verifies the result, and escalates to a human operator when autonomous suppression is insufficient.

## Objectives

**Primary objective:** Design and develop an autonomous embedded firefighting robot capable of detecting, localizing, approaching and suppressing a fire while avoiding obstacles and verifying the success of suppression.

**Specific objectives:**

- Detect and localize fire using three flame sensors (left, centre, right).
- Navigate autonomously toward the fire based on its direction.
- Detect and avoid obstacles using an ultrasonic sensor.
- Integrate sensor data through the Arduino for real-time navigation decisions.
- Suppress the fire using a water pump and nozzle once the fire zone is reached.
- Verify suppression by rechecking the flame sensors after spraying.
- Implement retry logic with a limited number of suppression attempts.
- Trigger an alert state (LED + buzzer) when the fire cannot be extinguished autonomously.
- Provide clear LED and buzzer indications for important system states.

## Key Features

- **Directional fire localization** using three flame sensors, with no thermal imaging or learned classifiers.
- **Concurrent detect-and-avoid navigation** in a single decision loop.
- **Closed-loop suppression:** spray, re-check, retry.
- **Escalation:** alert state with LED and buzzer after 2-3 failed attempts.
- **Low cost:** built on an Arduino Uno with no thermal camera, LiDAR or SLAM.

## Hardware Requirements

| Component | Purpose |
|---|---|
| Arduino Uno R3 | Main microcontroller: decision logic, sensor reading, actuator control |
| Analog/digital flame sensor modules (x3) | Fire detection and localization (left, centre, right) |
| HC-SR04 ultrasonic sensor | Obstacle detection and distance measurement |
| Continuous-rotation servo motors | Robot locomotion / drive mechanism |
| 3-6 V mini DC submersible pump | Water-based fire suppression |
| Logic-level N-channel MOSFET | Switching of the water pump |
| 1N4007 flyback diode | Protects the pump-driving circuit from voltage spikes |
| Small spray/mist nozzle | Directing water toward the fire |
| Water reservoir with tubing | Storage and delivery of the suppressant |
| RGB LED | Visual status indication |
| Active buzzer | Audible status, verification and alert indication |
| 5 V, 2 A (minimum) buck converter | Regulated 5 V supply |
| 4.8-6 V battery pack | Power for the servos and the pump |
| 60-70 mm wheels and ball/caster wheel | Locomotion, chassis support and balance |
| 2WD chassis | Mechanical platform |
| Jumper wires, breadboard/PCB | Interconnections |

## Software Requirements

| Tool | Purpose |
|---|---|
| Arduino IDE | Writing and uploading code to the Arduino Uno |
| Embedded C/C++ (Arduino framework) | Core firmware: sensor reading, decision logic, and motor, pump, LED and buzzer control |
| Servo library (`Servo.h`) | Control of the drive servos and, if used, scanning or nozzle aiming |

## System Architecture

The system is organised around the Arduino Uno, which receives inputs from the three flame sensors and the ultrasonic sensor, and runs the decision algorithm that governs navigation and suppression.

> Add the block diagram (Figure 1) and hardware circuit diagram (Figure 2) here, e.g. `![Block Diagram](docs/block_diagram.png)` and `![Circuit Diagram](docs/circuit_diagram.png)`.

### Operating Flow

1. **Sensing and input:** Left, centre and right flame sensors give the fire direction; the ultrasonic sensor gives the distance to obstacles.
2. **Decision and navigation:** The microcontroller processes the inputs and drives the robot with obstacle avoidance.
3. **Suppression:** At the fire zone, the pump and nozzle are activated through the MOSFET switching stage.
4. **Verification:** After each spray the flame sensors are read again. If no flame is detected, the robot enters a safe state with the pump off and the status LED indicating the fire is out.
5. **Retry and escalation:** If the flame persists and the attempt limit has not been reached, the robot sprays again. At the limit, it enters the alert state and signals that human intervention is required.

```
Sense -> Decide/Navigate -> Suppress -> Verify --+--> Fire out: SAFE state
                                                 |
                                                 +--> Fire persists, attempts left: re-spray
                                                 |
                                                 +--> Limit reached: ALERT state
```

## Methodology

The firmware executes a closed **sense, suppress and reassess** cycle.

### 1. Fire Detection and Localization
Three flame sensors are mounted on the left, centre and right of the chassis and read continuously. The sensor reporting the strongest flame signal indicates the bearing of the fire.

### 2. Obstacle-Aware Navigation
The HC-SR04 measures the distance to objects ahead. The controller combines this with the fire direction in one ongoing decision: the robot turns toward the fire when the path is clear and steers around an obstacle detected within the threshold distance.

### 3. Suppression
When the flame sensors indicate the fire zone has been reached, the controller stops the drive and switches the submersible pump on via a logic-level MOSFET. A flyback diode across the pump protects the switching stage. Water is sprayed for a fixed interval, then the pump is switched off.

### 4. Verification, Retry and Escalation
After every spray the robot pauses and re-reads the flame sensors:

- **No flame:** enters the safe state; the status LED indicates the fire is out.
- **Flame present, attempts below limit:** repeats the spray and verification cycle.
- **Flame persists after 2-3 attempts:** stops and enters the alert state with a dedicated LED pattern and buzzer signal calling for human assistance.

## Status Indications

The RGB LED and active buzzer give distinct visual and audible signals for each re-check cycle, for confirmed extinguishment, and for the alert state, so an observer can follow the robot's decision state without a connected display.

## Intended Outcomes

| Function | Outcome |
|---|---|
| Fire localization | Fire bearing is identified from the left, centre and right sensor readings; the robot turns toward the strongest response. |
| Obstacle avoidance | Obstacles within the threshold distance are detected and the robot adjusts its path while continuing toward the fire. |
| Suppression | The pump is switched on at the fire zone and water is delivered through the nozzle. |
| Verification | Flame sensors are re-read after each spray; the pump is switched off and the status LED lit when no flame is detected. |
| Retry and alert | The spray is repeated within the attempt limit; after repeated failure the robot enters the alert state with LED and buzzer signalling. |

## Comparison with Existing Systems

None of the ten systems reviewed in the literature verifies extinguishment or retries after a failed attempt. This robot achieves verified suppression using only flame and ultrasonic sensors on a low-cost microcontroller, without thermal cameras, LiDAR or SLAM.

## Limitations

- Best suited to small and accessible fires; **not appropriate for electrical or chemical fires**.
- Flame sensor range and accuracy can be affected by environmental conditions.
- The ultrasonic sensor cannot distinguish a human victim from an obstacle.
- The onboard water reservoir is limited and needs periodic refilling.
- No mapping, so the robot is less robust in complex or dynamic indoor layouts.
- Large persistent fires still require human intervention after the retry limit is reached.

## Applications

- Initial fire response in industrial plants, warehouses and other hazardous facilities.
- Fire safety in commercial buildings, enabling faster detection and automated first response.
- Fire response in areas that are difficult or unsafe for people to enter.
- A low-cost teaching and research platform for embedded systems, robotics and autonomous control.
- A base design for further development with additional sensing, communication and suppression technologies.

**Sustainable Development Goals:** SDG 9 (Industry, Innovation and Infrastructure) and SDG 11 (Sustainable Cities and Communities) as primary alignments, with SDG 3 (Good Health and Well-being) as a supporting goal.

## Future Scope

- Thermal imaging or multi-sensor fusion for detection through smoke.
- SLAM-based mapping for complex indoor layouts.
- Additional suppression agents such as foam and dry powder.
- Camera or weight-based confirmation of suppression.
- IoT/GSM connectivity for remote alerts.
- Multi-robot coordination for larger environments.

## Demo and Presentation

Demo video and PPT: [Google Drive folder](https://drive.google.com/drive/folders/1WHj0e7zWW0xLIk137TNwPztTDNevHHd7?usp=sharing)

## References

1. Kim, J. H. (2014). *Autonomous navigation, perception and probabilistic fire location for an intelligent firefighting robot* (Doctoral dissertation). Virginia Polytechnic Institute and State University.
2. Nikhare, Shrikhande, Selote and Khalale (2022). *Development of autonomous fire fighting robot.*
3. *Fire detection and direction control of fire fighting robot.* IJCRT.
4. *Development of an IoT-based firefighting drone for enhanced safety and efficiency in fire suppression* (2024). Measurement and Control.
5. *Development of a novel IoT-based smart firefighting robot for real-time fire detection and suppression* (2025). Current Journal of Applied Science and Technology.
6. *IoT based fire fighting robot.* IRJMETS.
7. *Arduino based autonomous fire fighting robot.*
8. *An indoor autonomous inspection and firefighting robot based on SLAM and flame image recognition* (2023). Fire (MDPI).
9. *Research on accurate fire source localization and seconds-level autonomous fire extinguishing technology* (2025). Scientific Reports.
10. *Design of fire fighting robotic system for fire disaster control.*
