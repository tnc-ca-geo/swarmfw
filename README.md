# swarmfw

Firmware connecting SDI-12 sensors to SWARM satellite platform. We are using https://swarm.space as a platform to send messages from sensors. For the SWARM tile documentation see here https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf.''

## Current payload configuration/payload

### Sensors:
3 Campbell Scientific CV50 https://www.campbellsci.com/climavue-50

4 TekBox Leaf Wetness https://www.tekbox.com/product/tbslws1-sdi-12-leaf-wetness-sensor/

5 Meter Soil Moisture Teras 12 https://www.metergroup.com/environment/products/teros-12/

### Message format:

Example message:

```
000549,1638633620,3.59,SC,51,+26+0.000+0+0+0.93+246.3+2.65+11.0+1.18+100.83+0.900+10.8+0.2+1.7+0-0.38-0.86+2.65,52,+0.00,53,+2038.84+16.1+39
```

**Fields**

*index* ... number of messages since last reboot

*unix epoch time stamp*

*battery voltage*

*message type*

  SC ... SDI-12 messsage agquired with the ?C! command

*array of SDI-12 messages* in the form

   - sensor address as number (48 = '0')
   
   - concatenated sensor response from (?D1! ...)

first SDI-12 sensor (CV50 at address 51 ('3'))

  1. Solar flux density
  2. Precipitation
  3. Lightning strike count
  4. Strike distance
  5. Wind speed
  6. Wind direction
  7. Maximum wind speed
  8. Air temperature
  9. Vapor pressure
  10. Barometric pressure (absolute)
  11. Relative humidity
  12. Humidity sensor temperature
  13. Tilt North(+)/South(–) orientation
  14. Tilt West(+)/East(–) orientation
  15. Compass heading (disabled)
  16. North wind speed
  17. East wind speed
  18. Wind speed max. – 10 s gust
 
second SDI-12 sensor (TekBox at address 52 ('4'): leafwetness in %

third SDI-12 sensor (Meter Teras12 at address 53 ('5')

see http://manuals.decagon.com/Integrator%20Guide/18224%20TEROS%2011-12%20Integrator%20Guide.pdf

 1. calibratedCountsVWC
 2. temperature
 3. electricalConductivity
