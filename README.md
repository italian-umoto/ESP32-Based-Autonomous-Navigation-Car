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
