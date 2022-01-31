# Co2fly

# Hardware
## Sensor
Sensirion SCD40/41 will both work
For ease of assembly, I suggest using the adafruit PCBs that provide I2C STEMMA QT connectors
https://www.adafruit.com/product/5187

SCD41 will work out of the box, it's a bit more expensive to allow it to go to 10 000ppm (not really relevant for us)
https://www.adafruit.com/product/5190

SCD41 could be a good fallback solution if SCD40 is out of stock

SCD4X datasheet can be found at: https://sensirion.com/media/documents/C4B87CE6/61652F80/Sensirion_CO2_Sensors_SCD4x_Datasheet.pdf

Sensirion SCD30 could be made to work as well, with minor modifications, but is more expensive, bulky, etc.
https://www.adafruit.com/product/4867

- SCD40/41 use photoacoustic measurements, as first described here in 2015: https://www.sciencedirect.com/science/article/pii/S1877705815025072
- SCD30 uses the old tried and tested NDIR (non-dispersive-infra-red)

They should provide very similar accuracy, the photoacoustic is newer and slightly cheaper to produce. Also much more compact.

They both measure "true" Co2 concentration, instead of trying to guess Co2 concentration by indirect means like cheaper sensors.
They should both be perfectly adequate for COVID usecases, as long as they are properly calibrated by the user.

## Main board
I chose to use the RP2040 adafruit feather for the following reasons:

- cheap
- includes STEMMA/QT connector for I2C plug-and-play
- low power draw (no need for ESP32 wifi/bluetooth nonsense)
- compact form-factor
- USB-C connector, auto-charging
- JST connector for LI-Po battery, perfect form factor for a 400mah unit

Sadly, this board does not allow us to measure battery voltage/charge. This is another reason to maximize battery life so it's less of a concern for the user

https://www.adafruit.com/product/4884

WATCH OUT: current I2C implementation uses main I2C pins as "Wire1" instead of "Wire". Anything using the Stemma/QT port or the main SDA/SCD pins needs to be initiated with "Wire1" in the code, not the default and commonly used "Wire" in all libraries/examples. This cost me a few nights to figure out.

## Display
I chose the monochromatic Adafruid 128x64 OLED I2C display
https://www.adafruit.com/product/4650

We need buttons for the user interface, as we absolutely need at least to provide forced manual calibration capability.

While the display can be drawn through I2C, buttons are simple pull-down pins, so they need to be soldered to the main board :(
Adafruit does not sell assembled versions of this display, so this requires soldering a male header set

WATCH OUT: this display uses SH1107 controller instead of SSD1306! I wasted two nights of code on this confusion.

## Battery

Used a Li-Po 400mah battery with a perfect form-factor for Adafruit Featers
https://www.adafruit.com/product/3898

From my testing it seems to provide around 6-8h of battery life using the OLED display.
Maybe this can be optimized but I doubt it.

Being unable to measure battery voltage from the board makes it impossible to go to any kind of "power saving mode" when battery is low
Usecase for the device also requires the screen to be always-on and legible, so it can be put on a table and checked at a glance.

## Cables

1x Stemma/QT I2C cable, connecting the rp2040 feather to the SCD40 
https://www.adafruit.com/product/4399

## Cost

Total cost is around $80 per unit, plus shipping and importation taxes (for Europe for example)

Shipping to europe was about 30€ with UPS

Not sure how cost effective this is compared to alternatives, the total price is dominated by the sensor, followed by display and main board. I don't think we can go much lower without sacrificing important things.

## Repairability

A critical goal of this project was to remain easy to disassemble and allow the replacement of *any* single part and comporent in case of failure, upgrade, etc.

This means the battery can be replaced, the display can be replaced, the sensor can be replaced, the main board can be replaced, or swapped.

I'm not interested in a design that requires more soldering (less accessible to the general public) and sacrifices repairability.
E-waste is a serious issue and covid is not an excuse for bad design :)

I hightly encourage you to keep repairability and ease of assembly as main focus of any units you might want to assemble on your own.

## Assembly

COMING SOON

# Software

## Libraries

### Sensor
https://github.com/Sensirion/arduino-i2c-scd4x

### Display

Arduino basic GFX lib:
https://github.com/adafruit/Adafruit-GFX-Library

Adafruit SH110X OLED library:
https://github.com/adafruit/Adafruit_SH110x


# Features and usage

## Main screen

TODO

## Graph mode

TODO

## Calibration

TODO

## Credits

TODO

## Design philosophy

This project was made for people close to me who need assistance with no-nonsense covid health protocols.
Fully assembled co2 sensors are sold out, hugely expensive, and not calibrated for COVID usecases and thresholds.

I wanted to get this into the hands of teachers and people who might have a high impact in bending the Omicron surge down.
As a result, it was very important to keep UX absolutely simple and clear to use.

I aimed for a zero-solder assembly, but the main feather rp2040 does not come with female headers assembled and the display does not come with male headers assembled.
They need to be coupled for the buttons to be usable, and we absolutely need buttons for the calibration process.

Whatever you do with this code/project, do NOT waste CO2 sensors to play around juste because you think this is cool to have in a drawer.
Make sure every single CO2 sensor you buy will find its way into a functional assembled unit, and this unit into the hands of someone who might have real-life use for it.

Teachers and people forced to work in closed spaces with numerous people, who can reduce risk by properly managing airflow and ventilation should be the main focus.

Keep in mind we all play a role in the supply chain stress of the co2 sensors around the world.
Be responsible.


Also don't add a million overkill features that only nerds will ever use and that will confuse the heck of "regular people".
Keep usability in mind.

For example, the design of the main screen was driven by the decision that the end user cares less about the co2 ppm number than the high-level threat assessment and safety instructions. Focus on what the user needs to be SAFE.
