[![Analytics](https://ga-beacon.appspot.com/UA-77377175-2/readme)](https://github.com/LukevdPalen/SJCAM-WIFI-remote)

# SJCAM-WIFI-remote
Arduino sketch for esp8266 esp01 working on a 3.7v 800mAh LIPO battery

Work in progress..

##What's working:
- Capture photo
- Record video
- Stop video
- Toggle photo/video mode

[DEMO](https://www.youtube.com/watch?v=BbjntvEiLL0)  
## Roadmap:
- Auto sleep 
- Restart
- camera led state on remote
- SSID discovery
- Shopping list
- PCB sketch
- 3D Model case

## Confirmed working on:  
:white_check_mark: SJCAM Sj4000 WIFI  
:white_large_square: SJCAM Sj5000 WIFI  
:white_large_square: SJCAM Sj5000+  
:white_large_square: SJCAM Sj5000X Elite  

[action cam commands guide](http://sj4000programming.sourceforge.net/)

## micropython flash cmd on ESP8285:
python.exe esptool.py --port COM4 erase_flash
python.exe esptool.py --port COM4 --baud 115200 write_flash -fm dout -fs 8m 0x00000 esp8266-20190125-v1.10.bin
