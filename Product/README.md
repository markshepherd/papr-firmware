This directory contains the firmware that runs the Air-To-All PAPR version 3.

The firmware runs on the ATMega328p microcontroller chip ("MCU") that is on the PAPR's PCB. Using the various pin input and output of the MCU, the firmware controls all the active components of the PAPR, including the buttons, LEDs, fan, buzzer, battery and charger. The firmware also has acceess to a serial port for text input/output for testing and debugging.

This is an Arduino-compatible app, written in C++. The app doesn't run on any actual arduino board, but we use Arduino as the base for this app so that we can take advantage of the Arduino APIs, Arduino libraries, and the Arduino community (forums, blogs, etc). If it became necessary to eliminate any Arduino dependencies, I think you could do it with a few days of work.

Unit testing: at present there is no automated testing of this code. However, the code is broken down into C++ classes in a way that would make it somewhat easy to add unit testing. This repo has a branch "unittest" that contains some very-out-of-date experimental code for this.

# Dev environment setup

To compile or develop this code:
1. get a Windows 10 machine. If you have a Intel macintosh, you can use Bootcamp and the free version of Windows 10. (The free Windows 10 is simply Windows 10 that has not been activated with a serial number; it works fine, but certain features are not available and there is an inconspicuous watermark on your screen.)
2. install Visual Studio 2019. The free "Community" edition works fine. Or you can use a paid version if you prefer.
3. in Visual Studio, install the [Visual Micro](https://www.visualmicro.com) extension. There is no free version but it only costs a few dollars. This extension knows how to create, build, and configure arduino-compatible projects.
4. get a programmer - this is a device that connects to your computer and to the PAPR PCB, and lets you download firmware and data into the MCU's memory. I am using the AVR ISP MK II programmer, which is no longer available made by Atmel, but is available from other manufacturers, for example [this one](https://www.amazon.com/waveshare-Compatible-AVRISP-USB-XPII/dp/B00ID98C5K/ref=sr_1_2_sspa?dchild=1&keywords=atmel+avr+isp+mkii&qid=1624736601&sr=8-2-spons&psc=1&smid=A2SA28G0M1VPHD&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUFLUzdNVzRJS1NYVTAmZW5jcnlwdGVkSWQ9QTA1NDQ1NzEzMTU1TkRIUkhMWDhWJmVuY3J5cHRlZEFkSWQ9QTAyMTEwOTIzS0M3Wk5ZMkE1RThYJndpZGdldE5hbWU9c3BfYXRmJmFjdGlvbj1jbGlja1JlZGlyZWN0JmRvTm90TG9nQ2xpY2s9dHJ1ZQ==).
5. download and install [Zadig](https://zadig.akeo.ie/), a Windows application that manages USB drivers. You need this to set up the driver for the AVRISPMKII programmer.
6. connect the AVRISPMKII to your computer, then use Zadig to set the AVRISPMKII's driver to "libusb32".
7. download and install [avrdude 6.3](https://www.nongnu.org/avrdude/), a windows command-line application that you can use to program the MCU's memories. I recommend unzipping into a new folder in "c:\avrdude". There are additional docs [here](https://www.ladyada.net/learn/avr/avrdude.html).

It may also be possible to build this project using the Arduino IDE, but I haven't tried that for months. Good luck.

This project uses the Arduino library "FanController 1.0.6". To ensure repeatable builds, we keep a copy of this library in the visual studio project, in the "Libraries" folder. (It's easy to do this in Visual Micro, using `Extensions > vMicro > Add Library` with the *Clone For Solution* option).

The "Docs" folder contains a rough specification for the functionality of this ownlofirmware. 

