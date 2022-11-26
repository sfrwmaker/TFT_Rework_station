# TFT_Rework_station
Soldering and rework station with TFT ili9341 display based on stm32

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

Dec 31, 2021, v.1.07
* Fixed parameter menu incorrect value show issue

Feb 12, 2022, v.1.08
* Fixed errors in NLS files, added new sentenses to russian ang portugeese languages
* Added support for display rotaion (0, 90, 180, 270 degrees). Landscape display rotation is the default.
* Display rotation menu item added to the parameters menu

Mar 4, 2022, v.1.09
* Changed rotary endoder procedure. Increased encoder stability.

Oct 14, 2022, v.1.10
* Rebuilt the project using STM CubeIDE and new HAL firmware.
* Add second binary version with DMA support of the TFT SPI display.

Nov 14, 2022, v.1.11
* Rebuilt the project with an updateg graphic library version.
* Revisited internal controller logic.
* Minor bugs fixed.

Nov 26, 2022, v 1.12
* Fixed issues in the parameters menu
* Updated the NLS message files (removed one parameter item, old files should work properly)

