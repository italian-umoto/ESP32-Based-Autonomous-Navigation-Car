# ESP32-Based-Autonomous-Navigation-Car

## About the Project

ESP32-Based-Autonomous-Navigation-Car is a group project created for **Junior Design Project at Tufts University**. Our goal was to **create a fully autonomous robot that is capable of traversing a track without human intervention and learn about the desinging process on a more in-depth level**.

## Features

* [Feature 1]
* [Feature 2]
* [Feature 3]

## Technologies Used

This project was built using:

* **ESP32** – 
* **Git & GitHub** – version control and team collaboration

## Team Members

This project was developed by:

* **[Name]** – [role]


## How It Works

The FSM uses one course sequence for both solo roles:

```text
Idle -> CrossOutbound -> TurnBack -> FindPrimaryLane -> AlignWithPrimaryLane
     -> FollowPrimaryLane -> TurnTowardYellow -> FindYellowLane
     -> AlignWithYellowLane -> FollowYellowLane -> TurnTowardHome
     -> CrossHome -> Complete
```

Bot #1 finds blue and turns left at lane alignment and lane-ending turns.
Bot #2 finds red and turns right at those steps. Both bots turn clockwise
180 degrees after the first crossing and align with both primary and yellow lanes.

WebSocket commands:

```text
MAGICSMOKE67 set: STATE=7,1  (start/restart solo bot #1)
MAGICSMOKE67 set: STATE=7,2  (start/restart solo bot #2)
MAGICSMOKE67 set: STATE=0,0  (stop and return to manual mode)
```

Send only the message before the parenthesized explanation. Manual command
IDs 1-6 keep their previous meanings and are ignored during a solo run. The
physical button is not used by this FSM. Manual collision stopping requires
a fresh command after the obstacle clears.

Set `COUNTS_PER_90` in `src/fsm.cpp` to the encoder counts per wheel for a
90-degree pivot and tune it on the track. A 180-degree pivot uses twice that
count. The current value is zero, which skips turns. Each wheel stops when
its encoder travel reaches the target.

Each tick prints RGB reflection values, the detected color, and collision
background/illuminated/reflected readings with WALL or CLEAR status. Setup
waits for Wi-Fi before these readings begin.

Lane following steers right on the target color and left off it using 100/200
cps wheel speeds. Each tick reads wall and color once. Walls end the crossing
and lane-following legs; while finding a lane, a wall simply holds the robot
stopped. Complete stays stopped until restart or cancellation. There is no
button handling, debounce, turn timeout, lane-loss timer, or Fault state.
Partner coordination is not implemented.


## Project Structure

```text
[project-folder]/
├── [folder]/
├── [folder]/
├── [file]
├── [file]
└── README.md
```

## Challenges & What We Learned

Throughout this project, our team gained experience 

## Future Improvements

If we continued developing this project, we would like to:

* [Future improvement]
* [Future improvement]
* [Future improvement]

## Acknowledgments

We would like to thank **John Doe** for providing the guidance and resources that helped us complete this project.
