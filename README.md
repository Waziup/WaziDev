WaziDev
=======

This repository contains everything you need to use [WaziDev](http://www.waziup.io/documentation/wazidev/).

It can be loaded as a sketchbook in Arduino IDE.


Arduino Sketches
=======

➡️ Check out the [wazidev library examples](https://github.com/Waziup/wazidev-lib/tree/v2/examples) at the [waziup/wazidev-lib](https://github.com/Waziup/wazidev-lib) repository.



Develop
=======


You can clone this repo with:
```
git clone git@github.com:Waziup/WaziDev.git
```

ATTENTION, this repo use a "subtree" for WaziDev library. The subtree is in libraries/WaziDev.
You can get the last change to the library with:
```
git subtree pull --prefix libraries/WaziDev/ --squash wazidev-lib master
```

If you made changes to the WaziDev library, push them to the original repo of the library:
```
git subtree push --prefix libraries/WaziDev/ --squash wazidev-lib master
```

# Enabling LMIC support on Wazidev v1.3

If you use Wazidev v1.4 or above, there is a support automatically for LMIC lib. However, if you have Wazidev 1.3 you need to enable it via a small soldering as shown below:

![Soldering Wazidev_1.3 to enable LMIC](/docs/enable_LMIC_on_Wazidev_v1.3.jpeg)

- So as shown, you only need to Solder the **JR** pad which connects the *DIO0* of the Lora module to pin *D2* of the Atmega chip. 
- Also, you need to pick a wire and solder one end on where it is shown on the Lora module (which is *DIO1*) and another end to the *D3* pin of the Atmega chip. 
You can also use a jumper whire and plug one end to *D3* and solder the other end on *DIO1* as shown.

# Hardware

WaziDev is based on the design by [Fabien Ferrero](https://github.com/FabienFerrero/UCA_Board).
