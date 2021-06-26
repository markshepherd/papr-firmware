This directory contains the firmware that runs the Air-To-All PAPR version 3.

The firmware runs on the ATMega328p microcontroller chip ("MCU") that is on the PAPR's PCB. Using the various pin input and output of the MCU, the firmware controls all the active components of the PAPR, including the buttons, LEDs, fan, buzzer, battery and charger. The firmware also has acceess to a serial port for text input/output for testing and debugging.

This is an Arduino-compatible app, written in C++. The app doesn't run on any actual arduino board, but we use Arduino as the base for this app so that we can take advantage of the Arduino APIs, Arduino libraries, and the Arduino community (forums, blogs, etc). If it became necessary to eliminate any Arduino dependencies, I think you could do it with a few days of work.

Unit testing: at present there is no automated testing of this code. However, the code is broken down into C++ classes in a way that would make it somewhat easy to add unit testing. This repo has a branch "unittest" that contains some very-out-of-date experimental code for this.

# Dev environment setup

To compile or develop this code:
1. get a Windows 10 machine. If you have a Intel macintosh, you can use Bootcamp and the free version of Windows 10. (The free Windows 10 is simply Windows 10 that has not been activated with a serial number; it works fine, but certain features are not available and there is an inconspicuous watermark on your screen.)
2. install Visual Studio 2019. The free "Community" edition works fine. Or you can use a paid version if you prefer.
3. in Visual Studio, install the [Visual Micro](https://www.visualmicro.com) extension. There is no free version but it only costs a few dollars. This extension knows how to create, build, and configure arduino-compatible projects.
4. get a programmer - this is a device that connects to your computer and to the PAPR PCB, and lets you download firmware and data into the MCU's memory. I am using the AVR ISP MK II programmer, which is no longer made by Atmel, but is available from other manufacturers, for example [this one](https://www.amazon.com/waveshare-Compatible-AVRISP-USB-XPII/dp/B00ID98C5K/ref=sr_1_2_sspa?dchild=1&keywords=atmel+avr+isp+mkii&qid=1624736601&sr=8-2-spons&psc=1&smid=A2SA28G0M1VPHD&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUFLUzdNVzRJS1NYVTAmZW5jcnlwdGVkSWQ9QTA1NDQ1NzEzMTU1TkRIUkhMWDhWJmVuY3J5cHRlZEFkSWQ9QTAyMTEwOTIzS0M3Wk5ZMkE1RThYJndpZGdldE5hbWU9c3BfYXRmJmFjdGlvbj1jbGlja1JlZGlyZWN0JmRvTm90TG9nQ2xpY2s9dHJ1ZQ==).
5. download and install [Zadig](https://zadig.akeo.ie/), a Windows application that manages USB drivers. You need this to set up the driver for the AVRISPMKII programmer.
6. connect the AVRISPMKII to your computer, then use Zadig to set the AVRISPMKII's driver to "libusb32".
7. download and install [avrdude 6.3](https://www.nongnu.org/avrdude/), a windows command-line application for programming the MCU's memories. I recommend unzipping into a new folder `c:\avrdude`. There are additional docs [here](https://www.ladyada.net/learn/avr/avrdude.html).


# Working with the code

In visual studio, open the solution file "product.sln".

Edit source code in the usual way.

To compile the code, use xxxxx, or F7.

To download and run the code on the PAPR's PCB:
1. make sure the PCB has power, either from the battery connector, or the charger connector.
1. connect the AVRISPMKII's USB cable to your computer
1. connect the AVRISPMKII's 6-pin SPI connector to the PCB's 6-pin SPI header
1. use xxxxx, or F5, to compile and download
If the PCB's power is coming from the battery connector, you must disconnect the 6-pin SPI after downloading, in order to run the board. If power is coming from the charger connector, you can leave the SPI connected.

# Setting up a new MCU

If you have a PCB whose microcontroller has never been set up, you must first program the MCU's "fuse bytes" to the correct values:
1. make sure the PCB has power, either from the battery connector, or the charger connector.
1. connect the AVRISPMKII's USB cable to your computer
1. connect the AVRISPMKII's 6-pin SPI connector to the PCB's 6-pin SPI header
1. copy the .hex files from this repo's `Product/binaries` folder into your avrdude install folder `c:\avrdude`.
2. run `Command Prompt`
3. `cd c:\avrdude`
4. `avrdude -c avrispmkII -p m328p -U lfuse:w:lfusefile-PAPR.hex:i`
4. `avrdude -c avrispmkII -p m328p -U hfuse:w:hfusefile-PAPR.hex:i`
4. `avrdude -c avrispmkII -p m328p -U efuse:w:efusefile-PAPR.hex:i`

This will set the fuse bytes to:
- low byte = 0x72
- high byte = 0xDA
- extended byte = 0xFF
This sets the MCU clock to 8 MHz using the internal oscillator, and sets the initial clock divider to 8, which results in a clock speed of 1 MHz. For more details on the fuse bytes, see the ATMega328p datasheet.

# Misc notes

Serial port...

It may also be possible to build this project using the Arduino IDE, but I haven't tried that for months. Good luck.

This project uses the Arduino library "Low-Power 1.6". To ensure repeatable builds, we keep a copy of this library in the visual studio project, in the "Libraries" folder. (FYI, in Visual Micro, you can add more libraries `Extensions > vMicro > Add Library` with the *Clone For Solution* option).

The "Docs" folder contains a rough specification for the functionality of this ownlofirmware. 


# Development practices

I am not going to write a list of best practices for coding because there are a million web sites that talk about this. When working with the PAPR firmware, please act like a professsional. Follow the naming, formatting, and commenting practices that you see in the code. The PAPR is a medical device and we need the code to be high-quality, maintainable, and reliable. Make sure your code is as clean and simple as possible. Make sure that other people will be able to read and understand your code. Make sure you test your code thoroughly - once the product is delivered to the customer it is extremely difficult to fix bugs or update the code. If you add new features or change the behavior in any way, make sure to update the Specification and the Verification test suite (see links below).

# Related documents

[Functional Core Specification](https://docs.google.com/document/d/1O3QTVKkepRBzme7QUEG_r9uPpGkaOA27uIlCKO2OQU8/edit) - detailed description of everything the firmware does. 
[Functional Core Verification Test Suite] - a detailed set of tests that fully exercise the firmware. You MUST run this entire suite before shipping a new version of the firmware.
[PAPR battery discharge 6-4-21](https://docs.google.com/spreadsheets/d/14-mchRN22HC6OSyAcN329NEcRRjF2_VMbKz3yHDDEoI/edit#gid=1527307635) - measurements of battery voltage and current when the battery is being discharged. This information helps understand the behavior of the battery, and is the basis for some of the magic constants that appear in the firmware.
[PAPR battery charge 6-5-21](https://docs.google.com/spreadsheets/d/1fPnn2ukakk8MpyGW_KrOW2ediHh8FU6yWr4Kfq2UNJs/edit#gid=13224763) - ditto for battery charging.
[PCB Schematic](TBD)
