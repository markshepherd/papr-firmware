This project is the firmware that runs the Air-To-All PAPR version 2 PCB.

This is an Arduino-compatible app, that mostly uses Arduino APIs to access the hardware.

This project can be built with either the Arduino IDE, or Visual Studio with the [Visual Micro](https://www.visualmicro.com) extension.

This project uses the Arduino library "FanController 1.0.6". To ensure repeatable builds, we keep a copy of this library in the visual studio project, in the "Libraries" folder. (It's easy to do this in Visual Micro, using `Extensions > vMicro > Add Library` with the *Clone For Solution* option).

The "Docs" folder contains a rough specification for the functionality of this firmware. 

