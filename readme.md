
Basic information regarding the development side of the project

Hardware:
1. A esp132 board with bmp180 sensor, collects data, transmits data through local wifi network. This board is powered through a usb cable (from wall plug or power bank).
2. A home server that receives data, runs local data analytics tools and algorithm that decides whether window should be open, hosts a website which interacts with the esp132 board.

Software:
1. esp132 script: Written in Arduino programming language. Handles sensor connection, data transmition, display with LCD.
2. Server logic: Written in python. Handles the receiving and storage of sensor data, and transmiting "window open/close" signals to esp132 board.
3. Web front end: Static. Written in html. Handles the display of live sensor data and sending commands to esp132 board. 
2. Data analytics software: Written in C, with SDL2. Offers GUI tools that display stored data in various ways, and runs the window control algorithm.
