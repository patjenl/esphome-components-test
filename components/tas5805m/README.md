
## ESPHome component for ESP32-Louder (using ESP32-S3 with PSRAM and using TAS5805m DAC)

This ESPHome external component is based on a  TAS5805M DAC driver for ESP32 platform
https://github.com/sonocotta/esp32-tas5805m-dac/tree/main by Andriy Malyshenko
which is licenced under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007.


## Usage: Tas5805m component
This component requires Esphome version 2025.2.0 or later.

The following yaml can be used so ESPHome accesses the component files:
```
external_components:
  - source: github://mrtoy-me/esphome-components-test@main
    components: [ tas5805m ]
    refresh: 0s
```

The component configuration uses the esphome Audio DAC Core component,
so is a platform under the core audio dac component.

# Example configuration entry
```
audio_dac:
  - platform: tas5805m
    enable_pin: GPIOxx
```
***enable_pin:*** is required and is the enable pin of ESP32-Louder<BR>

Arduino or esp-idf frameworks can be used for defining the mediaplayer.
Example yaml configurations are provided in the repository. All examples
 1) create a switch so the tas5805m can be enabled/disabled(into deep sleep) by Homeassistant
 2) include an interval configuration to disable the tas65805m if there is not activity
 3) use mediaplayer on_play trigger to enable the tas5805m if it is disabled

The example yaml for arduino framework configures a mediaplayer that uses software for volume control and mute/unmute.
The compatible audio files are limited; mp3 audio files has been tested but other formats can cause th esp32 to reboot.
It is provided for compatibility and is not the recommended framework.

Two esp-idf example configurations are provided which both use the new speaker mediaplayer component. This component using
Homeassistant suports a wide range of common audio formats and has also been tested with text-to-speech.
The idf-basic yaml example has a single pipeline(announce) with volume control and mute/unmute undertaken by the tas5805m dac.
Note that this configuration does not support mediaplay pause.

The idf-media yaml example configures more use of psram and two pipelines - media and announce.
Volume control and mute/unmute are again undertaken by the tas5805m dac. This example reduces the volume of the media pipeline
if the announce pipeline plays and also increases the announce pipeline. Then reverts back when the announcement pipeline stops.
The announce pipeline can be adjusted through a number template which can be adjusted in Homeassistant.

