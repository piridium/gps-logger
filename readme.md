# GPS logger

A gps logger for arduino based on https://github.com/markfickett/gpslogger
Added a 128x64 oled display.
Todo: add support for 3-axis accelerometer and gyro.

## Components
- Arduino Uno
- GPS NEO-7M-C: https://www.bastelgarage.ch/gps-modul-uart-neo-7m-c-kompatibel?search=420576
- OLED Display Weiss I2c 128x64 0.96’’ SSD1306 - https://www.bastelgarage.ch/oled-display-weiss-i2c-128x64-0-96?search=420269
- Micro SD Card Modul 3.3V / 5V - https://www.bastelgarage.ch/micro-sd-card-modul-3-3v-5v?search=420555
- später: MPU-6050 3-Achsen Beschleunigungssensor mit Gyroskop - https://www.bastelgarage.ch/mpu-6050-3-achsen-beschleunigungssensor-mit-gyroskop?search=420350

## Wiring

	Dev - Arduino

### GPS
	VCC - 5V
	GND - GND
	TX  - D2
	RX  - D3

### Display
	VCC - 5V
	GND - GND
	SCL - A5
	SDA - A4

### SD-Card
	5V  - 5V
	GND - GND
	CLK - D13
	DO  - D12
	DI  - D11
	CS  - D4
