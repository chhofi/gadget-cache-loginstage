# gadget-cache-loginstage
A gadget cache that is part of a multi stage night-cache.

**The cache is split into two parts: A _physical_ and a _technical_ challenge part.**

## The physical part:
The following puzzles have to be solved in order to close the electronic circuit and to start the ESP8266:
- Filling water inside a tube
- Moving a knob up
- Pumping air into an outlet

Every time the task has successfully be fulfilled, this gets indicated by a green light.
When all of that is done, all three rods inside the case are lifted and you can activate the cache by pushing the wooden stick. -> This closes a contact and the circuit is activated. The unlocking mechanism the same as in a lock, only bigger.

## The technical part:
The technical part consists of:
- ESP8266
- Photodiode
- 3x LiFePo4 (Soshine-1800) battery a 1500mAh(measured)@3,3V
- 3x 18650 battery case
- SIM800 module
- OLED display
- Electronic enclosure

The ESP powers up and the OLED displays a progress bar. After that a WifiSymbol appears. Next you have to connect your phone to the wifihotspot the ESP just started sharing. You will get an automatic Captive Portal notification. This will directly point you the the loginpage (On some phones this does´nt work. In that case enter the URL 192.168.1.1 into your browser). There you have to enter your username and password. If successfully entered the Oled display will show another progress par. In the background it will send the batterylevel, the username, and the calculated time remaining till you have to swap batterys to IFTT.com. This service than sends me a notification with all that information via Telegram. 

## Files:
[Arduino project file](LoginStage/LoginStage.ino)
[Wifi logo for the OLED display](LoginStage/images.h)




