WaziDev command program
======================

This folder contains a small Arduino program to send commands to the WaziDev via the USB cable.
The Arduino program uses a library called "simpleRPC". With it, you can call functions on the WaziDev from your PC.
You can find the program [here](LoRaWAN-test.ino). This program have a function `sendLoRaWAN` that can be called from the USB interface of the WaziDev.
It can be used for example in a python program:
```
from simple_rpc import Interface

wazidev_port = '/dev/ttyUSB0'
interface = Interface(wazidev_port)

# Send a value with WaziDev
res = interface.sendLoRaWAN(62)
print(res)
```

The folder [API](API/] contains a python program capable of sending those USB commands to the WaziDev.
The python program exposes a REST API to the commands. 
You can call them with:
```
curl -X POST "http://127.0.0.1:5000" -H  "accept: application/json" -H  "Content-Type: application/json" -d "{\"val\":\"62\"}"
```
This will ask the WaziDev to send a temperature value of 62 Degrees on LoRaWAN (with an XLPP payload).
