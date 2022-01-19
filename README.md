# ESP8266an oinarrituriko IoT Termostatoa 

Hozgailu baten kontrola izango duen Web Termostatoa da hurrengo proiektua. ESP8266 mikrokontrolagailuan oinarrituriko termostatoak HOZGAILU baten kontrola hartuko du WEB ZERBITZAILE baten bitartez. Mikrokontrolagailuak DHT11 sentsoreak Tº eta Hº datuak jaso, prosezatu eta ERRELE baten bitartez hozgailua ON eta OFF egoeratan jarriko ditu. 

## ERABILERA
 
 Web nabigatzaile baten bitartez, [192.168.1.201](192.168.1.201) helbidean aurkituko dugu gure termostatoa.

## Hardwarea
<p align="center">
  <img src="/Irudiak/Termostatoa_bb.png" width="256" height="455">
 <img src="/Irudiak/WebTermostatoa.png" width="256" height="455">
</p>

![Eskema Elektrikoa](/Irudiak/Termostatoa_bb.png)

## Konfigurazioa

 > WIFI-a: ESP8266-DHT.ino artxiboan termostatoa zein wifi-tara konektatuko den konfiguratu. Wifi honek **192.168.1.X** tartean banatu beharko ditu helbideak.
~~~
const char* ssid     = "yourWiFiSSID";            
const char* password = "yourWiFiPassword"; 
~~~
 
  > Tº KONFIGURAZIOA: ESP8266-DHT.ino artxiboan hozgailuaren Tº max eta min konfiguratu (ºC).
~~~
int heatOn = 5;
int heatOff = 15;
~~~
 
   > PROGRAMAZIOA: Arduino IDE-aren bitartez ESP8266-DHT.ino artxiboa NODEMCU-ra igo.

1. [Arduino IDE Download](https://www.arduino.cc/en/software)
2. [Installing NODEMCU on Arduino IDE](https://create.arduino.cc/projecthub/najad/using-arduino-ide-to-program-nodemcu-33e899)
3. Hurrengo liburutegiak instalatu Arduino IDE-an.
- ESP8266WiFi.h
- WiFiClient.h
- ESP8266WebServer.h
- FS.h 
- ctype.h 
- DHT.h

4. [Termostatoaren Arduino Kodea](/ESP8266-DHT.ino)

## Estekak
 
Demostrazio bideoa https://youtu.be/uq5OR8RlGLc

Instructables https://www.instructables.com/ESP8266-12E-DHT-Thermostat/

### Thanks to @dmainmon

