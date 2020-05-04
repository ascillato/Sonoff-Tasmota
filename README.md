## Tasmota KNX  [![Tasmota CI](https://github.com/ascillato/Tasmota_KNX/workflows/Tasmota%20CI/badge.svg)](https://github.com/ascillato/Tasmota_KNX/tree/development/build_output/firmware) 

<img src="/tools/logo/TASMOTA_FullLogo_Vector.svg" alt="Logo" align="right" height="90"/>

[Tasmota_KNX](https://github.com/ascillato/Tasmota_KNX) was a modification for [Tasmota](https://github.com/arendst/Tasmota) to add a basic functionality of the [KNX IP Protocol](https://www.knx.org/knx-en/index.php) (Multicast).
**Now it is integrated in [Tasmota](https://github.com/arendst/Tasmota)!**

[![GitHub version](https://img.shields.io/github/release/ascillato/Tasmota_KNX.svg)](https://github.com/ascillato/Tasmota_KNX/releases/latest) [![GitHub download](https://img.shields.io/github/downloads/ascillato/Tasmota_KNX/total.svg)](https://github.com/ascillato/Tasmota_KNX/releases/latest) [![License](https://img.shields.io/github/license/ascillato/Tasmota_KNX.svg)](https://github.com/ascillato/Tasmota_KNX/blob/development/LICENSE.txt) [![donate](https://img.shields.io/badge/donate-PayPal-yellow.svg)](https://www.paypal.me/ascillato)

This repository is for developing new KNX features for Tasmota.

If you like **Tasmota KNX**, give it a star or fork it and contribute! [![GitHub stars](https://img.shields.io/github/stars/ascillato/Tasmota_KNX.svg?style=social&label=Star)](https://github.com/ascillato/Tasmota_KNX/stargazers)

Any help or comment is very welcome.

## Table of Contents

* [KNX Explanation](#knx-explanation)
* [Integration](#integration)
* [Requirements](#requirements)
* [Firmware](#firmware)
* [Usage Examples](#usage-examples)
* [Console Commands](#console-commands)
* [Development Road Map](#development-road-map)
* [Modifications to Tasmota](#modifications-to-tasmota)
* [Contributors](#contributors)
* [Tasmota Readme](#tasmota-readme)

## KNX Explanation ##

[<img src="https://www.knx.org/wGlobal/wGlobal/layout/images/knx-logo.png" />](https://www.knx.org/knx-en/knx/association/what-is-knx/index.php)

The [KNX IP Protocol](https://www.knx.org/knx-en/knx/association/what-is-knx/index.php) is an _international open standard_ for smart homes and smart buildings automation. It is a decentralized system. Each device can talk directly to each other without the need of a central controller or server. Any panel or server is just for telesupervision and for sending requests. KNX IP Protocol uses a UDP multicast on _224.0.23.12 : 3671_, so there is no need for a KNX Router unless you want to communicate to KNX Devices that are not in the WIFI Network (Twisted Pair, RF, Powerline).

Each device has a physical address (like a fixed IP) as **1 . 1 . 0** and that address is used for configuration purposes.

Each device can be configured with group addresses as **2 / 2 / 1** and that address can be used for sending/receiving commands.
So, for example, if 2 devices that are configured with the **2 / 2 / 1** for turning on/off their outputs, and other device send _Turn ON_ command to **2 / 2 / 1**, both devices will turn on their outputs.

## Integration ##

Several home automation systems have KNX support. For example, [Home Assistant](https://github.com/home-assistant/home-assistant) has a [XKNX Python Library](https://github.com/XKNX/xknx) to connect to KNX devices using a KNX Router. If you don't have a **KNX Router**, you can use a **Software KNX Router** like [KNXd](https://github.com/knxd/knxd) on the same Raspberry Pi than Home Assistant. KNXd is used by Home Assistant for reading this UDP Multicast, although KNXd has other cool features that need extra hardware like connect to KNX devices by Twister Pair, Power Line or RF.

If using the Home Assistant distribution called **Hassio**, everything for KNX is already included by default.

If you use the ETS (KNX Configurator Software) you can add any Tasmota KNX as a dummy device.

## Requirements ##

All the libraries required for Tasmota are [here](https://github.com/ascillato/Tasmota_KNX/tree/development/lib) along with the extra required for the KNX Driver:

* [ESP KNX IP Library](https://github.com/ascillato/Tasmota_KNX/tree/development/lib/esp-knx-ip-0.5.2). A copy of this modified library is also available [here](https://github.com/ascillato/esp-knx-ip). The original is [here](https://github.com/envy/esp-knx-ip).

**Esp8266 board libraries:**
* v2.6.3 Works fine. No known issues. **(RECOMMENDED)**.
* v2.6.2 Works fine. No known issues. Bigger flash usage than 2.6.1.
* v2.6.1 Works fine. No known issues. Smaller flash usage than 2.6.0 
* v2.6.0 Works fine. No known issues.
* v2.5.2 Has some wifi issues. Sleep feature works fine but needs to be 0 for better KNX performance. Do not use.
* v2.5.1 Has some wifi and **security** issues. Do not use.
* v2.5.0 Has some wifi and **security** issues. Do not use.
* v2.4.2 Command sleep don't work in this version. It has **security** issues. Do not use.
* v2.4.1 Have some wifi and **security** issues. Do not use.
* v2.4.0 Have some wifi and **security** issues. Do not use.
* v2.3.0 It is slower than v2.4.2. Sleep feature works but it has **security** issues. Do not use.

## Firmware ##

You can download the precompiled binaries for flashing ESP8266, ESP8285 and ESP32 devices from:

* Latest [Release](https://github.com/ascillato/Tasmota_KNX/releases)
* Latest development version from [Build_Output](https://github.com/ascillato/Tasmota_KNX/tree/development/build_output/firmware) folder. 
* Or if you need any other feature enabled or disabled, or a different Arduino core, you can use [TasmoCompiler](https://gitpod.io/#https://github.com/benzino77/tasmocompiler) ([readme](https://github.com/benzino77/tasmocompiler)) or you can modify the **my_user_config.h** file and build your firmware as explained in the [docs](https://tasmota.github.io/docs/#/).

## Implemented Features ##

The implemented features (up to now) in KNX for Tasmota are:

General:
* Buttons (just push)
* Relays (on/off/toggle)
* Lights (led strips, etc. but just on/off)

Sensor lists that you can use in KNX is (only one sensor per type):
* Temperature
* Humidity
* Energy (v, i, power)

Features that can be used with Tasmota's rules:
* Send KNX command (on/off)
* Receive KNX command (on/off)
* Send values by KNX (any float type, temperature for example)
* Receive values by KNX (any float type, temperature for example)
* Receive a KNX read request
* View/Set the Physical Address
* View/Set Group Address to send data
* View/Set Group Address to receive data

## Usage Examples ##

There are multiple possible configurations. Here are explained just a few as example. The options for selecting relays, buttons, sensors, etc. are only available if were configured on _Configure Module Menu_.

To configure KNX, enter on the Configuration Menu of Tasmota and select Configure KNX.

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/Config_Menu.jpg" />
<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/KNX_menu.jpg" />

_Note on KNX communication enhancement option: As Wifi Multicast communication is not reliable in some wifi router due to IGMP problems or Snooping, an enhancement was implemented. This option increase the reliability by reducing the chances of losing telegrams, sending the same telegram 3 times. In practice it works really good and it is enough for normal home use. When this option is on, Tasmota will ignore toggle commands by KNX if those are sent more than 1 toggle per second. Just 1 toggle per second is working fine._

**1) Setting Several Tasmotas to be controlled as one by a Home Automation System:**

We can set one of the group address to be the same in all the devices so as to turn them on or off at the same time.
In this case, so as to inform the status of all the relays to the Automation System, just one of the devices have to be configured as the responder. If you use the same Group Address for sending and receiving, you have to take into account not to make loops.

DEVICE 1

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/1.jpg" />

DEVICE 2

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/2.jpg" />

**2) Setting 2 Tasmotas to be linked as stair lights:**

We can set one device to send the status of its output and another to read that and follow. And the second device can send the status of its button and the first device will toggle. With this configuration we can avoid to make a loop.

DEVICE 1

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/3.jpg" />

DEVICE 2

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/4.jpg" />

**3) Setting a button as initiator of a scene:**

Just setting one device to send the push of a button, and the rest just use that value to turn them on. In this case, there is no toggle. Every time the button is pushed, the turn on command is sent.

DEVICE 1

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/5.jpg" />

DEVICE 2

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/6.jpg" />

**4) Setting a Temperature sensor:**

We can configure to send the value of temperature or humidity every teleperiod. This teleperiod can be configured. See Tasmota [wiki](https://github.com/arendst/Tasmota/wiki/Commands). It is recommended also to set the reply temperature address.

<img src="https://github.com/ascillato/Tasmota_KNX/blob/development/.github/7.jpg" />

**5) Using rules:**

More functionality can be added to Tasmota using rules.

* In the KNX Menu, can be set a Group Address to send data or commands by rules, as **KNX TX1** to **KNX TX5**

In rules we can use the command ``KnxTx_Cmnd1 1`` to send an ON state command to the group address set in **KNX TX1** slot of the KNX menu.
Also, we can use the command ``KnxTx_Val1 15`` to send a 15 value to the group address set in **KNX TX1** slot of the KNX menu.

* In the KNX Menu can be set a Group Address to receive commands by rules as **KNX RX1** to **KNX RX5**

In rules we can use the events to catch the reception of COMMANDS from KNX to those RX Slots.

Example: ``rule on event#knxrx_cmnd1 do var1 %value% endon`` to store the command received in the variable VAR1

In rules we can use the events to catch the reception of VALUES from KNX to those RX Slots.

Example: ``rule on event#knxrx_val1 do var1 %value% endon`` to store the value received in the variable VAR1

Also, if a Read request is received from KNX Network, we can use that in a rule as for example: ``rule on event#knxrx_req1 do knxtx_val1 %var3% endon``

## Console Commands ##

Command                | Payload | Description
-----------------------|---------|---------------------------------------------------------------------------
KnxTx_Cmnd\<x\>       | 0 / 1 | Send KNX Commands using the Group Address set on KNX Menu at KNX_TX\<x\> slot
KnxTx_Val\<x\>       | value | Send KNX float values using the Group Address set on KNX Menu at KNX_TX\<x\> slot
KNX_ENABLED            |       | View Status of KNX Communications
|                      | 0 / 1 | 0 - Set Disable to KNX Communications / 1 - Set Enable to KNX Communications
KNX_ENHANCED           |       | View Status of Enhanced mode for KNX Communications
|                      | 0 / 1 | 0 - Set to Disable / 1 - Set to Enable Enhanced KNX Communications Mode
KNX_PA                 |       | View the device KNX Physical Address (0.0.0 means not set)
|                      | \<x\>.\<x\>.\<x\> | Set the device KNX Physical Address (like 1.1.0)
KNX_GA                 |       | View the number of Group Address Configured to Send Data/Commands
|                      | \<x\>     | View the configuration for the Group Address number x Send Data/Commands
KNX_GA\<x\>            | \<y\>,\<z\>,\<z\>,\<z\> | Set the Group Address number \<x\> to Send Data/Commands
|                      |       | \<y\> is the parameter OPTION to send its status to the Group Address
|                      |       | \<z\>,\<z\>,\<z\> is the Group Address number to Send Data/Commands
KNX_CB                 |       | View the number of Group Address Configured to Receive Data/Commands
|                      | \<x\>     | View the configuration for the Group Address number x to Receive Data/Commands
KNX_CB\<x\>            | \<y\>,\<z\>,\<z\>,\<z\> | Set the Group Address number \<x\> to ReceiveData/Commands
|                      |       | \<y\> is the parameter OPTION to Receive its status from the Group Address
|                      |       | \<z\>,\<z\>,\<z\> is the Group Address number to Receive Data/Commands

Posible values for the parameter OPTION:

OPTION Value | Device Parameter
-----------------------|---------
1 | Relay 1
2 | Relay 2
3 | Relay 3
4 | Relay 4
5 | Relay 5
6 | Relay 6
7 | Relay 7
8 | Relay 8
9 | Button 1
10 | Button 2
11 | Button 3
12 | Button 4
13 | Button 5
14 | Button 6
15 | Button 7
16 | Button 8
17 | TEMPERATURE        
18 | HUMIDITY           
19 | ENERGY_VOLTAGE     
20 | ENERGY_CURRENT     
21 | ENERGY_POWER       
22 | ENERGY_POWERFACTOR 
23 | ENERGY_DAILY       
24 | ENERGY_START       
25 | ENERGY_TOTAL       
26 | KNX_SLOT1              
27 | KNX_SLOT2              
28 | KNX_SLOT3              
29 | KNX_SLOT4              
30 | KNX_SLOT5              
255 | EMPTY

## Development Road Map ##

**For Tasmota_KNX:**
- [x] Add Web Menu
- [x] Add Feature to Receive telegrams and modify Relay Status
- [x] Add Feature to Receive telegrams from multiple Group Addresses to modify just one relay status (useful for scenes)
- [x] Add Feature to Send telegrams of relay status change
- [x] Add Feature to Send telegrams of one relay status to multiple Group Addresses (useful for scenes)
- [x] Add Feature to Send telegrams of button pressed
- [x] Add Feature to receive telegrams to toggle relay status
- [x] Add Feature to read Temperature, Humidity from Tasmota
- [x] Add Feature to send Temperature, Humidity by a set interval (tasmota teleperiod)
- [x] Add Feature to receive command to read temperature, Humidity
- [x] Add Feature to recognize Tasmota config to show the same number of relays, buttons, etc.
- [x] Add Feature to Save Config
- [x] Add Feature to Load Config
- [x] Add Log Info
- [x] Complete all the language files with keys
- [x] Add support for other output devices supported by Tasmota
- [x] Add support for other sensors supported by Tasmota (TEMP, HUM, ENERGY)
- [x] Add commands for rules to send values and commands by KNX
- [x] Add commands for rules to set KNX Configurations
- [x] Add events for rules when receiving data from KNX and read requests
- [x] Add option for increase communication reliability (re send telegrams)
- [ ] Add option for multicast forced reconnection (needed for some routers that have IGMP conflict with actual esp8266 lib v2.3.0 to v2.5.2, and lwIP v1.4 to v2.1 - Send a telegram to itself. If it is received, multicast is ok, if not, reconnect)
- [ ] Add option to support KNX Snooping to debug KNX Network
- [ ] Add option for KNXnet/IP Tunneling
- [ ] Add option to repeat all KNX multicast broadcast (Tasmota to Tasmota communications) to KNXnet/IP Tunneling
- [ ] Add option to support ETS Programming
- [ ] Optimize code to reduce Flash and RAM

## Modifications to Tasmota ##

* Added the file _/tasmota/xdrv_11_KNX.ino_
* Added the entries `#define USE_KNX` and `#define USE_KNX_WEB_MENU` on _/tasmota/my_user_config.h_
* Added entries to the file _/tasmota/xdrv_01_webserver.ino_
* Added entries to the file _/tasmota/tasmota.ino_
* Added entries to the file _/tasmota/tasmota.h_
* Added entries to the file _/tasmota/settings.h_
* Added entries to the file _/tasmota/support.ino_
* Added entries to sensor files
* Added entries to language files

Up to now, enabling KNX uses +9.4 KB of code and +3.7 KB of memory for Tasmota. If it is enabled also the KNX Web Menu, it adds +8.3 KB more of code and +144 Bytes more of memory.

There is **NO CONFLICT** with MQTT, Home Assistant, Web, etc. Tests show fast response of all features running at same time.

## Contributors ##

* [ascillato](https://github.com/ascillato) ( Adrian Scillato )
* [sisamiwe](https://github.com/sisamiwe) - Thanks for the guide on using KNX and software testing and support
* [envy](https://github.com/envy) ( Nico Weichbrodt ) - Thanks for the patience and help with the modifications to ESP_KNX_IP.
* [arendst](https://github.com/arendst) ( Theo Arends ) - Thanks for the guide on Tasmota and for the ideas.
* [johannesbonn](https://github.com/johannesbonn) - Thanks for the patience on bug resolutions
* [RocketSience](https://github.com/RocketSience) - Thanks for the patience on bug resolutions
* [jeylites](https://github.com/jeylites) - Thanks for the patience on bug resolutions
* [Winni66](https://github.com/Winni66) - Thanks for the patience on bug resolutions
* [misc2000](https://github.com/misc2000) - Thanks for the testing on bug resolutions
* [mizrachiran](https://github.com/mizrachiran) ( Ran Mizrachi ) - Thanks for the testing on bug resolutions
* [smurfix](https://github.com/smurfix) ( Matthias Urlichs ) - Thanks for the KNX guiding and [KNXd](https://github.com/knxd/knxd) use.
* And many others providing testing, bug reporting and feature requests.

-----------------------------------------------------------------------------------------------------------------------------------
## Tasmota Readme ##

![Tasmota logo](/tools/logo/TASMOTA_FullLogo_Vector.svg)

Alternative firmware for [ESP8266](https://en.wikipedia.org/wiki/ESP8266) based devices with **easy configuration using webUI, OTA updates, automation using timers or rules, expandability and entirely local control over MQTT, HTTP, Serial or KNX**.
_Written for Arduino IDE and PlatformIO._

[![GitHub version](https://img.shields.io/github/release/arendst/Tasmota.svg)](https://github.com/arendst/Tasmota/releases/latest)
[![GitHub download](https://img.shields.io/github/downloads/arendst/Tasmota/total.svg)](https://github.com/arendst/Tasmota/releases/latest)
[![License](https://img.shields.io/github/license/arendst/Tasmota.svg)](LICENSE.txt)
[![Chat](https://img.shields.io/discord/479389167382691863.svg)](https://discord.gg/Ks2Kzd4)

If you like **Tasmota**, give it a star, or fork it and contribute!

[![GitHub stars](https://img.shields.io/github/stars/arendst/Tasmota.svg?style=social&label=Star)](https://github.com/arendst/Tasmota/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/arendst/Tasmota.svg?style=social&label=Fork)](https://github.com/arendst/Tasmota/network)
[![donate](https://img.shields.io/badge/donate-PayPal-blue.svg)](https://paypal.me/tasmota)

See [RELEASENOTES.md](RELEASENOTES.md) for release information.

In addition to the [release webpage](https://github.com/arendst/Tasmota/releases/latest) the binaries can also be downloaded from http://thehackbox.org/tasmota/release/

## Development

[![Dev Version](https://img.shields.io/badge/development%20version-v8.2.x.x-blue.svg)](https://github.com/arendst/Tasmota)
[![Download Dev](https://img.shields.io/badge/download-development-yellow.svg)](http://thehackbox.org/tasmota/)
![Tasmota CI](https://github.com/arendst/Tasmota/workflows/Tasmota%20CI/badge.svg)

See [tasmota/CHANGELOG.md](tasmota/CHANGELOG.md) for detailed change information.

Unless your Tasmota powered device exhibits a problem or you need to make use of a feature that is not available in the Tasmota version currently installed on your device, leave your device alone - it works so don't make unnecessary changes! If the release version (i.e., the master branch) exhibits unexpected behaviour for your device and configuration, you should upgrade to the latest development version instead to see if your problem is resolved as some bugs in previous releases or development builds may already have been resolved.

The Tasmota development codebase is checked every 1-2 hours for changes. If new commits have been merged and they compile successfuly, new binary files for every variant will be posted at http://thehackbox.org/tasmota/ (this web address can be used for OTA updates too). The last compiled commit number is also indicated on the same page. It is important to note that these binaries are based on the current development codebase. These commits are tested as much as is possible and are typically quite stable. However, it is infeasible to test on the hundreds of different types of devices with all the available configuration options permitted.

Note that there is a chance, as with any upgrade, that the device may not function as expected. You must always account for the possibility that you may need to flash the device via the serial programming interface if the OTA upgrade fails. Even with the master release, you should always attempt to test the device or a similar prototype before upgrading a device which is in production or is hard to reach. And, as always, make a backup of the device configuration before beginning any firmware update.

## Disclaimer

:warning: **DANGER OF ELECTROCUTION** :warning:

If your device connects to mains electricity (AC power) there is danger of electrocution if not installed properly. If you don't know how to install it, please call an electrician (***Beware:*** certain countries prohibit installation without a licensed electrician present). Remember: _**SAFETY FIRST**_. It is not worth the risk to yourself, your family and your home if you don't know exactly what you are doing. Never tinker or try to flash a device using the serial programming interface while it is connected to MAINS ELECTRICITY (AC power).

We don't take any responsibility nor liability for using this software nor for the installation or any tips, advice, videos, etc. given by any member of this site or any related site.

## Note

Please do not ask to add new devices unless it requires additional code for new features. If the device is not listed as a module, try using [Templates](https://tasmota.github.io/docs/Templates) first. If it is not listed in the [Tasmota Device Templates Repository](http://blakadder.github.io/templates) create your own [Template](https://tasmota.github.io/docs/Templates#creating-your-template).

## Quick Install
Download one of the released binaries from https://github.com/arendst/Tasmota/releases and flash it to your hardware [using our installation guide](https://tasmota.github.io/docs/Getting-Started).

## Important User Compilation Information
If you want to compile Tasmota yourself keep in mind the following:

- Only Flash Mode **DOUT** is supported. Do not use Flash Mode DIO / QIO / QOUT as it might seem to brick your device.
- Tasmota uses a 1M linker script WITHOUT spiffs **1M (no SPIFFS)** for optimal code space.
- To make compile time changes to Tasmota use the `user_config_override.h` file. It assures keeping your custom settings when you download and compile a new version. You have to make a copy from the provided `user_config_override_sample.h` file and add your setting overrides. To enable the override you have to set a compile define as documented in the `user_config_override_sample.h` file.

## Configuration Information

Please refer to the installation and configuration articles in our [documentation](https://tasmota.github.io/docs).

## Migration Information

See [wiki migration path](https://tasmota.github.io/docs/Upgrading#migration-path) for instructions how to migrate to a major version. Pay attention to the following version breaks due to dynamic settings updates:

1. Migrate to **Sonoff-Tasmota 3.9.x**
2. Migrate to **Sonoff-Tasmota 4.x**
3. Migrate to **Sonoff-Tasmota 5.14**
4. Migrate to **Sonoff-Tasmota 6.x**
5. Migrate to **Tasmota 7.x**

--- Major change in parameter storage layout ---

6. Migrate to **Tasmota 8.1**
7. Migrate to **Tasmota 8.x**

## Support Information

<img src="https://user-images.githubusercontent.com/5904370/68332933-e6e5a600-00d7-11ea-885d-50395f7239a1.png" width=150 align="right" />

For a database of supported devices see [Tasmota Device Templates Repository](https://templates.blakadder.com)

If you're looking for support on **Tasmota** there are some options available:

### Documentation

* [Documentation Site](https://tasmota.github.io/docs): For information on how to flash Tasmota, configure, use and expand it
* [FAQ and Troubleshooting](https://tasmota.github.io/docs/FAQ/): For information on common problems and solutions.
* [Commands Information](https://tasmota.github.io/docs/Commands): For information on all the commands supported by Tasmota.

### Support's Community

* [Tasmota Forum](https://groups.google.com/d/forum/sonoffusers): For usage and discussions.
* [Tasmota Support Chat](https://discord.gg/Ks2Kzd4): For support, troubleshooting and general questions. You have better chances to get fast answers from members of the Tasmota Community.
* [Search in Issues](https://github.com/arendst/Tasmota/issues): You might find an answer to your question by searching current or closed issues.

### Developers' Community

* [Bug Report](https://github.com/arendst/Tasmota/issues/new?template=Bug_report.md): For reporting Bugs of Tasmota Software.
* [Feature Request](https://github.com/arendst/Tasmota/issues/new?template=Feature_request.md): For requesting features/functions to Tasmota Software.
* [Troubleshooting](https://github.com/arendst/Tasmota/issues/new?template=Custom.md): As a last resort, you can open new *Troubleshooting* issue on GitHub if the solution could not be found using the other channels. Just remember: the more info you provide the more chances you'll have to get an accurate answer.
* [Issue a question](https://github.com/arendst/Tasmota/issues/new/choose): As a last resort, you can open a new *Question* issue on GitHub if the answer could not be found using the other channels. Just remember: the more info you provide the more chances you'll have to get an accurate answer.

## Contribute

You can contribute to Tasmota by
- providing Pull Requests (Features, Proof of Concepts, Language files or Fixes)
- testing new released features and report issues
- donating to acquire hardware for testing and implementing or out of gratitude
- contributing missing [documentation](https://tasmota.github.io/docs) for features and devices

[![donate](https://img.shields.io/badge/donate-PayPal-blue.svg)](https://paypal.me/tasmota)

## Credits

People helping to keep the show on the road:
- David Lang providing initial issue resolution and code optimizations
- Heiko Krupp for his IRSend, HTU21, SI70xx and Wemo/Hue emulation drivers
- Wiktor Schmidt for Travis CI implementation
- Thom Dietrich for PlatformIO optimizations
- Marinus van den Broek for his EspEasy groundwork
- Pete Ba for more user friendly energy monitor calibration
- Lobradov providing compile optimization tips
- Flexiti for his initial timer implementation
- reloxx13 for his [TasmoAdmin](https://github.com/reloxx13/TasmoAdmin) management tool
- Joachim Banzhaf for his TSL2561 library and driver
- Gijs Noorlander for his MHZ19, SenseAir and updated PubSubClient drivers
- Erik Montnemery for his HomeAssistant Discovery concept and many code tuning tips
- Federico Leoni for continued HomeAssistant Discovery support
- Aidan Mountford for his HSB support
- Daniel Ztolnai for his Serial Bridge implementation
- Gerhard Mutz for multiple sensor & display drivers, Sunrise/Sunset, and scripting
- Nuno Ferreira for his HC-SR04 driver
- Adrian Scillato for his (security)fixes and implementing and maintaining KNX
- Gennaro Tortone for implementing and maintaining Eastron drivers
- Raymond Mouthaan for managing Wemos Wiki information
- Norbert Richter for his [decode-config.py](https://github.com/tasmota/decode-config) tool
- Andre Thomas for providing [thehackbox](http://thehackbox.org/tasmota/) OTA support and daily development builds
- Joel Stein, digiblur and Shantur Rathore for their Tuya research and driver
- Frogmore42 for providing many issue answers
- Jason2866 for platformio support and providing many issue answers
- Blakadder for managing the new document site and providing template management
- Stephan Hadinger for refactoring light driver, enhancing HueEmulation and Zigbee support
- tmo for designing the official Tasmota logo
- Stefan Bode for his Shutter and Deep sleep drivers
- Jacek Ziółkowski for his [TDM](https://github.com/jziolkowski/tdm) management tool and [Tasmotizer](https://github.com/tasmota/tasmotizer) flashing tool
- Christian Staars for NRF24L01 and HM-10 Bluetooth sensor support
- Pail Diem for UDP Group communication support
- Jörg Schüler-Maroldt for his initial ESP32 port
- Many more providing Tips, Wips, Pocs, PRs and Donations

## License

This program is licensed under GPL-3.0
