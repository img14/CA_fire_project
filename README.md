# CA Fire Alerts

#### Uses Arduino Uno Wifi Rev 2 to obtain air quality data, locations of wildfires, and local fire weather warnings from the following APIs:
  * services3.arcgis.com (USGS Fire Data)
  * api.weather.gov (Nationwide extreme weather alerts)
  * api.airvisual.com (Air Quality by location, requires personal key)

#### Intended for use in California (for now)

Requires personal `secrets.h` file with WiFi network name and password, as well as latitude and longitude coordinates and air quality server key.

Anticipated updates:
  * Automatically use your own location
  * Find a different server for weather alerts (inconsistent responses)
