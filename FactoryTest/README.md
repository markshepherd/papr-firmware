This firmware project is an app that exercises the hardware of the PAPR V2 after it has been manufactured. To use the app, you program it into the PAPR's microcontroller, then you perform the steps described in the test script, to verify that the PAPR is working correctly.

The test script source file is `PAPR v2 Test Script.docx` and a printable test script is `PAPR v2 Test Script.pdf`. If you modify how the test works, don't forget to update the docx and re-generate the PDF.

To develop or build this project, you can use either Visual Studio with the [Visual Micro](https://www.visualmicro.com) extension, or the Arduino IDE.

The `libraries` folder contains Arduino libraries that the code uses. We use our own copy of these libraries (rather than rely on Arduino's library manager to fetch us a copy) so that builds of our software are 100% repeatable.

