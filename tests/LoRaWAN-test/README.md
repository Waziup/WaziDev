WaziDev command program
======================

This folder contains a small Arduino program to send commands to the WaziDev via the USB cable.
With it, you can call functions on the WaziDev from your PC.
The Arduino program uses a library called "simpleRPC".

It is coupled with a python program to send those commands.
The python program exposes a REST API to the commands.

You can call them with:
```
curl -X POST "http://127.0.0.1:5000" -H  "accept: application/json" -H  "Content-Type: application/json" -d "{\"val\":\"62\"}"
```
This will ask the WaziDev to send a temperature value of 62 Degrees on LoRaWAN (with an XLPP payload).
