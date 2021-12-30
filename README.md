# TFT_Rework_station
Soldering and rework station with TFT ili9341 display absed on stm32

Here is a binary firmware of the controller and the controller shcematics files only.
No sources available at the moment.

The description of the project you can find on hackster https://www.hackster.io/sfrwmaker/stm32-soldering-and-rework-station-with-tft-display-b23f73

Revision history:

Nov 04 2021, v.1.02
* Fixed error unformatted flash detection
* Fixed black screen if the flash is not formatted
* Introduce the debug mode

Dec 17 2021, v.1.03
* Fixed issue the hot Air Gun calibration data rewrites current tip calibration
* Iron anf Hot Air Gun messages changed to icons to simplify project localization

Dec 21 2021, v.1.04
* Fixed Hot Air Gun setup temperature or fan speed issue in the main working mode
* Added Native Language Support using UTF-8 encoding. Please, consult article on hackster.io how lo upload NLS files to the SPI FLASH.
* Added russian language messages (Cyryllic font + message file)
* Added Portugeese language messages (Western Europe font + message file). Thank to my friend Armindo for translation.

Dec 22 2021, v.1.05
* Extend diagnistic messages when uploading NLS data from SD-CARD  

Dec 30 2021, v.1.06
* Minor bugs fixed
* Added polish language messages (Western Europe font - impact + message file). In my opinion, the impact font is not very clear, it is better to use unifont_we.font.
