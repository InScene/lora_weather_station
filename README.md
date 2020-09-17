# lora_weather_station
A **solar powered weather station** which used LoRaWAN to send the data to the Things Network server.
The hardware is based on an Arduino mini.

Used hardware:
* Arduino mini with ATmega328P (3,3V, 8MHz)
* BME280 sensor (temperature, humidity, pressure)
* Rain sensor 
* Rain gauge sensor
* Lipo battery
* Solar panel 

The data is sent in the CayenneLPP format every 3.5 minutes. The channels are assigned as follows:
1. Temperature
2. Relative humidity
3. Barometric pressure
4. Battery value (Volt)
5. Interpretet rain sensor: 0=dry, 1=light rain, 2=heavy rain, 3=downpour
6. digital rain value (2bytes)
7. rain amount counter last 5 minutes (mm)

To calibrate the interpreted rain sensor, the border values could be sent via the LoRaWAN Rx functionality.
Ensure that the data will be sent in little-endian as raw format, and always send 6 bytes:
 *   Example: FF 00 90 01 9C 02
 *   FF 00 : 256 = cloudburst
 *   90 01 : 400 = heavy rain
 *   9c 02 : 668 = light rain