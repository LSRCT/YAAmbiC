# YAAmbiC
Yet another ambilight clone. Many did it before, many will do it after, this is how I did it

## Setup
Hardware:
- Ws2812b LED Strip from amazon
- esp8266

The LEDs require 5V DC, using a reasonably sized screen and appropriate brightness a USB3 outlet should be able to power this.
Communication is done via UDP over Wifi to avoid using a Serial to USB adapter.
The color information is calculated by a python script, applying some smoothing before sending.

## Communication
The esp is connected to wifi and recieves color information via UDP. UDP avoids any overhead and is fine to use here since robustness is not a big concern.
Conveniently rgb values are encoded as a triple of bytes as the individual values range from 0-255 anyway, thus avoiding string encoding overhead.

## Screencap
Screencapture is done using https://pypi.org/project/d3dshot/, a fast screencap library.
After screencapture the image is cropped to the borders and filtered.
This method achieves ~60 fps.

## TODO
- remove hardcoded wifi password
- integrate mqtt interface to use as light without screencapture



