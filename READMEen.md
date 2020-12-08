### Install software 

#### Arduino IDE
[Install Arduino IDE](https://www.arduino.cc/en/software)

Start Arduino IDE and install all necessary libraries

Tools -> Manage libraries -> search libraries
 - LiquidCrystal_I2C.h
 - Adafruit_MCP23017.h
 - HX711.h

You can skip this step if you are going to use vscode as the main IDE

Select the right board
Tools -> boards manager -> ESP8266 -> NodeMCU 1.0(ESP-12E)

#### VsCode cofniguration
If you would like to use vscode as your main IDE to work with arduino you would need to install [arduino plugin](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-arduino) and configure it to work with your ESP

[Here is an example hot to configure vscode](https://www.youtube.com/watch?v=FnEvJXpxxNM)



Now let's try to upload a simple example, like blink
File -> examples -> basics -> blink
Click upload

When the upload process is done you should see that a led on the board is blinking.

Let's upload the mixer code. Here is the latest [code](https://paste.ubuntu.com/p/D6pyjJ9H5K/)

- Open in your arduino IDE the mixer file
- Update the pump port names
- replace this to your port names
- Here is the naming convention:

```bash
A0=0 .. A7=7
B0=8 .. B7=15
```

That means if you have B0/A0 == 8/0, one more example B3/A3 == 11/3

```bash
// A0
int pmp1 = 0;
// B0
int pmp1r = 8;
...
etc.
```

click upload

As soon as it was upload you should see `Ready` on the screen

We are ready to continue.
Here is a connection diagram
https://easyeda.com/editor#id=ddc9071d732f415cb2ba157b1ce8f923|5c0b5124a6da439392efe447b6ff0a1b

### Калибровка весов/стола

1. нужно взять точно известный вес в районе 500 грамм
2. отметить две точки на которых будут стоять бутылки А и B
3. Перезагрузить систему с пустым столом.
4. Поставить на точку А известный вес и подбирать значение `scale_calibration`

Если вес ниже эталлонного то значение `scale_calibration` уменьшаем

Например:
- scale_calibration = 2045
- эталлоный вес 47.90
- ставим его на весы(стол) и если значение ниже эталонного
- вес на столе 47.50
- scale_calibration уменьшаем
- scale_calibration = 2040
- прошиваем
- повторяем процедуру
- ставим эталлоный вес на весы(стол) и если значение ниже эталонного
- вес на столе 47.64
- scale_calibration уменьшаем
- scale_calibration = 2040

4. полученное значение в вносим в строку 183
