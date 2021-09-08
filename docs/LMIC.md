
# Enabling LMIC support on Wazidev v1.3

If you use Wazidev v1.4 or above, there is a support automatically for LMIC lib. However, if you have Wazidev 1.3 you need to enable it via a small soldering as shown below:

![Soldering Wazidev_1.3 to enable LMIC](/docs/enable_LMIC_on_Wazidev_v1.3.jpeg)

- So as shown, you only need to Solder the **JR** pad which connects the *DIO0* of the Lora module to pin *D2* of the Atmega chip. 
- Also, you need to pick a wire and solder one end on where it is shown on the Lora module (which is *DIO1*) and another end to the *D3* pin of the Atmega chip. 
You can also use a jumper whire and plug one end to *D3* and solder the other end on *DIO1* as shown.
