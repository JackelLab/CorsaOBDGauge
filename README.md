Bluetooth OBD Based Gauge For The Vauxhall/Opel Corsa

Anyone who owns an early Corsa knows you can't show the battery percentage on the dash
without having the car plugged in to a charger. The only other way is using an app on
your phone connected to and Bluetooth OBD adaptor (or WiFi adaptor if you have an iphone).

The problem with using an app (like 'Car Scanner' is that you need to connect to the
adaptor and then the car everytime you want to see the data.

I decided (too long ago) that I wanted to make a display which showed the battery
percentage so it was always visable. My biggest stumbling block to this was understanding
what requests to send to the car and how to decode this data. After sometime, I found
some details on another github page 'EVNotify' which mentioned what data needed to be
sent and what the responses would be:
https://github.com/EVNotify/EVNotify/issues/205

With this information, I was finally able to get access to the data from the car.

I built a PCB with an ESP32 as the main processor as it has Bluetooth builtin, an LCD
from ebay with 160 x 80 pixels

WIP....
