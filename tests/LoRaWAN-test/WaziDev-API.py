
from flask import Flask, request, jsonify
from simple_rpc import Interface
import os
from threading import Thread

app = Flask(__name__)
app.config["DEBUG"] = True
wazidev_port = os.getenv("WAZIDEV_PORT", '/dev/ttyUSB0')

interface = Interface("/dev/ttyUSB0")


@app.route('/', methods=['POST'])
def postVal():
   return sendValueWaziDev(request.form)


def sendValueWaziDev(val: int) -> str:
    print("sending value: " + str(val))
    return interface.sendLoRaWAN(val)

app.run()

if __name__ == '__main__':
    print("main")
