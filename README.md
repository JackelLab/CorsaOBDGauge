Bluetooth OBD Based Gauge For The Vauxhall/Opel Corsa

Anyone who owns an early Corsa knows you can't show the battery percentage on the dash
without having the car plugged in to a charger. The only other way is using an app on
your phone connected to and Bluetooth OBD adaptor (or WiFi adaptor if you have an iphone).

The problem with using an app (like 'Car Scanner' is that you need to connect to the
adaptor and then the car every time you want to see the data.

I decided (too long ago) that I wanted to make a display which showed the battery
percentage so it was always visible. My biggest stumbling block to this was understanding
what requests to send to the car and how to decode this data. After sometime, I found
some details on another github page 'EVNotify' which mentioned what data needed to be
sent and what the responses would be:
https://github.com/EVNotify/EVNotify/issues/205

With this information, I was finally able to get access to the data from the car.

I built a PCB with an ESP32-WROVER-IE as the main processor as it has Bluetooth built in,
a 0.96" LCD from ebay with 160 x 80 pixel 65k colours.

The features I've added so far are:

SOC        - Battery state of charge. I've used the non calibrated version, I don't know
what the differences are.

SOH        - Battery state of health. This only shows after the unit has setup and
checked communications with the OBD adaptor and car.

SOC Delta  - State of charge change. This shows to SOC change since the unit was powered
up, ideal for tracking how much battery has been used for a journey or how much battery
has been added while charging.

Temperature - Battery temperature.

Speed       - Shows the speed in 100th of MPH which can be re-calibrated if measured
against a GPS speedo.

Voltage     - OBD 12V battery voltage. Measured at the OBD adaptor.

Est. Range  - Estimated range based on speed and battery percentage used over the last
minute and 5 minutes.


There is a reed switch to change the display modes, the gauge will remember the last mode
and restore it at power up.


The ELM327 OBD.

When selecting a Bluetooth OBD adaptor, you will need to find one with the PIC18F25K80
chip. May of the adaptors for sale will say they are PIC versions, but often, they are
emulated versions and for whatever reason, the ESP32 can't communicate correctly with
them and just receives echoed data back.

So, ELM Electronics created the ELM327 chip for communicating with the OBD port on
vehicles, but with their version 1.5 software, chips where sent out without the code
protection on and so this code has been copied by may of the Chinese manufactures making
the currently available adaptors. If the adaptor you are looking to buy has any mention of
V2.1, then it is more likely a chip pretending to be a PIC.

The only way to really confirm that the adaptor has a PIC chip in it is to pop it open and
check.

My code requires you to put in the MAC address of the OBD adaptor, here are 2 ways you can
find the MAC address. Use an ESP32 to scan for Bluetooth devices and report the addresses
( https://github.com/esp32beans/ESP32-BT-exp ) or pair with a windows machine and get the
MAC address from the device details.

The alternative way, would be to use WiFi versions of the adaptor. At the moment, I've not
done any development on this method but may look into it in the future.

Another thing to note, not all adaptors have the correct resistors to do voltage
measurement and some that do, may not have been calibrated. Check the voltage with a volt
meter and compare that with the adaptor. Calibration would need to be done through a
computer with a terminal program.

Currently, the enclosure you can download is just a simple cover to hide and protect the
board, a friend is currently trying to map the instrument cluster to make a neater, almost
factory style, cover for the board which will sit to the right of the current instrument
cluster.

WIP..
