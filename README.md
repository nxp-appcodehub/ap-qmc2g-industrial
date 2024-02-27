# NXP Application Code Hub
[<img src="https://mcuxpresso.nxp.com/static/icon/nxp-logo-color.svg" width="100"/>](https://www.nxp.com)

## i.MX RT INDUSTRIAL DRIVE DEVELOPMENT PLATFORM

The i.MX RT Industrial Drive Development Platform software package consists of a reference demo application and API that demonstrate how to take advantage of i.MX RT Industrial Drive Development Platform hardware capabilities to develop a secure, robust and reliable multi-motor control system which meets the requirements, standards and best practices required by commercial industrial products. This significantly reduces the effort required to develop multi-motor control applications and the time-to-market of the product.

The platform shows how a single i.MX RT1176 Crossover MCU can control up to four different motors while managing wired or wireless connectivity and an HMI interface. Leverage this flexible three-board kit to evaluate motor control performance, speed up motor control designs based on the i.MX RT Crossover MCUs and learn to use NXP EdgeLock® security technology to enable safer communications and industrial control.

**Documentation and more details not covered by this README are available at:**
* [AN13644 App Note](https://www.nxp.com/docs/en/application-note/AN13644.pdf): Getting started with i.MX RT Industrial Drive Development Platform – covers technical details for the bring up of the platform.
* [AN13643 App Note](https://www.nxp.com/docs/en/application-note/AN13643.pdf): i.MX RT Industrial Drive Development Platform software overview - covers the software feature overview of the application. Note: This Application Software Pack doesn't yet cover all the features described in the application note. Future releases will contain the complete set of features.
* [AN13642 App Note](https://www.nxp.com/docs/en/application-note/AN13642.pdf): i.MX RT Industrial Drive Development Platform hardware overview - covers hardware specifications of the boards.    

* [Project Website](https://www.nxp.com/design/designs/i-mx-rt-industrial-drive-development-platform:I.MX-RT-INDUSTRIAL-DRIVE-DEV-PLATFORM): This overview webpage contains information about the platform, links to documentation and instructions on how to order the HW this Application Software Pack is associated with.

**The current Application Software Pack for the i.MX RT Industrial Drive Development Platform covers the full set of features described in the documentation:**
* Motor control
* Fault handling
* TSN connectivity
* Data Logging
* Board Service – Temperature and Gate Driver status monitoring
* Local Service - GUI and display handling, buttons and IO handling
* Web Service + Web Server
* Cloud Service
* Cryptography
* Secure Watchdog
* Secure Bootloader
* User Management
* TSN Motion Controller - (in binary format)

**Features added in release 1.2:**
* Web Service + Web Server - used for a remote connection via a web server API with a control panel. The panel is controlled through user accounts with different roles and privileges.
* Cloud Service - used for a remote connection to a cloud platform.
* User Management - support for different user roles with specified access privileges.
* TSN Motion Controller - a binary for the i.MX RT1170, which shows an example TSN Motion Controller application that is able to send trajectory commands to the i.MX RT Industrial Drive Development Platform. 

#### Boards: ISI-QMC-DGC-02
#### Categories: Industrial, RTOS, Secure Provisioning, Security, Sensor, Motor Control, Graphics, Cloud Connected Devices, Time Sensitive Networking
#### Peripherals: ADC, CAN, CLOCKS, DISPLAY, DMA, ETHERNET, FLASH, GPIO, I2C, PWM, SENSOR, SPI, UART, USB, WATCHDOG, TIMER
#### Toolchains: MCUXpresso IDE

## Table of Contents
1. [Software](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Results](#step4)
5. [FAQs](#step5) 
6. [Support](#step6)
7. [Known Issues](#step7)
8. [Release Notes](#step8)

## 1. Software<a name="step1"></a>
To be able to run this software pack, make sure to first install the following tools:
* [MCUXpresso IDE v11.9.0](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE) or above. 
* [FreeMASTER tool 3.2](https://www.nxp.com/design/software/development-software/freemaster-run-time-debugging-tool:FREEMASTER) or above.
* [MCU-Link FW with the version appropriate for your IDE](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcu-link-debug-probe:MCU-LINK) if you want to use the MCU-Link Debug Probe.
* [J-Link FW with the latest available version](https://www.segger.com/downloads/jlink/) even if you don't want to use the J-Link Debug Probe.
* [USB-to-UART Drivers with the latest available version](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads) to be able to communicate with the platform over a serial port.
* Copy the JLinkDevices/ folder from the tools/ folder into %APPDATA%/SEGGER once your J-Link FW installation is complete.
* [West with the latest available version](https://docs.zephyrproject.org/latest/develop/west/install.html) to be able to create and SDK package from the Application Software Pack. Make sure to add your West installation path to your PATH environment variable.
* [Git with the latest available version](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git).

## 2. Hardware<a name="step2"></a>
* Purchase  [i.MX RT Industrial Drive Development Platform](https://www.nxp.com/design/designs/i-mx-rt-industrial-drive-development-platform:I.MX-RT-INDUSTRIAL-DRIVE-DEV-PLATFORM)
	* 1x ISI-QMC-DGC02. (Daughter Card) 
	* 1x ISI-QMC-DB02. (Digital Board) 
	* At least 1x ISI-QMC-PSB02 or 1x ISI-QMC-PSB02B. (Power Stage Board). Note that you can add up to four Power Stage Boards.
	* At least 1x M-2310P-LN-04K motor, if you want to use the motor control capabilities. 
	
* You will also need either a J-Link or an MCU-Link Debug Probe.

## 3. Setup<a name="step3"></a>
There are a few steps you need to make before you'll be able to run the application. The whole process is described in full detail in [AN13644 - Getting Started](https://www.nxp.com/docs/en/application-note/AN13644.pdf).

### 3.1 Step 1: Software Preparation
You need to have [West](https://docs.zephyrproject.org/latest/develop/west/index.html) and Git installed and configured into your PATH variable. There are two options for installing this application software pack into MCUXpresso:

* **Option 1:**
	Right-click in the "Installed SDKs" window in the MCUXpresso IDE and choose the "Import remote SDK Git repository" option. Fill the Repository box with ```https://github.com/nxp-appcodehub/ap-qmc2g-industrial``` and the Revision box with ```main```.

* **Option 2:**
	Open a Command Line and execute the commands below to gather the whole QMC2G-INDUSTRIAL delivery at revision ```${revision}``` and place it in a folder named ```ap_qmc2g_industrial```. 
	```
	west init -m https://github.com/nxp-appcodehub/ap-qmc2g-industrial --mr ${revision} ap_qmc2g_industrial
	cd ap_qmc2g_industrial
	west update
	```
	Replace ```${revision}``` with any SDK revision you wish to achieve. This can be ```main``` if you want the latest state, or any commit SHA.

	Finally, drag-and-drop the ap_qmc2g_industrial/ folder into the "Installed SDKs" window in the MCUXpresso IDE.
	
### 3.2 Step 2: Hardware Preparation
For information about how to assemble the HW, please refer to [AN13644 - Getting Started](https://www.nxp.com/docs/en/application-note/AN13644.pdf) and [AN13642 - Hardware overview](https://www.nxp.com/docs/en/application-note/AN13642.pdf).

### 3.3 Step 3: Software Configuration
You can now go into the ```industrial_app_master_cm7/source/qmc_features_config.h``` file and configure how many motors you're connecting (MC_MAX_MOTORS), whether a given PSB has an AFE installed (MC_HAS_AFE_MOTORx) and many other features further described in the [AN13643 App Note](https://www.nxp.com/docs/en/application-note/AN13643.pdf).

### 3.4 Step 4: Run the Application
**Before you can run the application, you need to first provision the Secure Element on the Digital Board. To do this, please follow the steps in the Provisioning chapter of either [AN13644 App Note](https://www.nxp.com/docs/en/application-note/AN13644.pdf) or the Provisioning and Secure Bootloader User Guide.pdf in the tools/ folder.**

**Make sure to put your device into SDP mode for initial debugging by configuring the boot pins on the Daughter Card to ON-OFF-OFF-OFF.**

**To run the application from the IDE using a debugger (Release and Debug targets of the CM7 and CM4 projects):**
1. Build both the CM4 and CM7 projects with the same target (Release or Debug).
2. Start the debug session for the CM7 project first. For CM7, use the pre-configured launch files included in the project (right-click on the launch file and go to Debug As) or re-configure the IDE-generated launch files to the same settings as the pre-configured ones. For the CM4 project, either use the included launch file again or you can let the IDE create the default launch file, open it, go into the JLinkDebugger settings and uncheck the "Attach to a Running Target" option.
3. Place a breakpoint at the line with ```ui32ClkADC = CLOCK_GetFreqFromObs(CCM_OBS_ADC1_CLK_ROOT);``` in **main_cm7.c**
4. Run the CM7 application until the breakpoint.
5. Keep the CM7 debug session **open** and start the CM4 debug session **at the same time**.
6. Run the CM4 application.
7. Run the CM7 application.

If you want to see the debug output of the application, you can open two serial terminals - one of them for the CM4 and the other for the CM7 project.

If you want to control the application through FreeMASTER, please close the CM4 serial terminal first. You can find the pre-configured FreeMASTER project in the **freemaster_exe/** directory. If anything doesn't work, double check the project settings and the path to the AXF file.

**To run the application without a debugger and boot from the internal memory (*_SBL targets of the CM7 and CM4 projects), follow the instructions in "tools/Provisioning and Secure Bootloader User Guide.pdf"**

## 4. Results<a name="step4"></a>
You should now be able to control the motors through FreeMASTER and make use of all the features available in this release. Please, refer to the abovementioned documentation for more details.

## 5. FAQs<a name="step5"></a>
No FAQs have been identified for this project

## 6. Support<a name="step6"></a>
If you have any questions or find a bug, please submit a New issue in the Issues tab of this GitHub repository.

## 7. Known Issues<a name="step7"></a>
* Due to the limitations of the App SW Pack project generation, there are some project configurations which can't be set correctly. It is recommended to change these options manually:
	1. Set the Debug and Debug_SBL targets' optimization to "optimize for debug".
	2. Turn on PRINTF FLOAT. (PRINTF_FLOAT_ENABLE=1)

* There is a known issue with the display output. Due to the framebuffers being stored in the Octal RAM and the logs in the Octal FLASH, writes and reads to both share the same Octal bus. Race conditions can lead to display flickering and "rolling" effects. This is purely cosmetic and once the display content stabilizes, the application continues running without any issues.

* Some SD cards fail to be correctly read from or written to by the application. If you run into issues after double checking that you performed all of the provisioning and SBL steps correctly, please try a different SD card.

#### Project Metadata
<!----- Boards ----->
[![Board badge](https://img.shields.io/badge/Board-ISI&ndash;QMC&ndash;DGC&ndash;02-blue)](https://github.com/search?q=org%3Anxp-appcodehub+ISI-QMC-DGC-02+in%3Areadme&type=Repositories) [![Board badge](https://img.shields.io/badge/Board-I&ndash;MX&ndash;RT&ndash;INDUSTRIAL&ndash;DRIVE&ndash;DEV&ndash;PLA-blue)](https://github.com/search?q=org%3Anxp-appcodehub+I-MX-RT-INDUSTRIAL-DRIVE-DEV-PLA+in%3Areadme&type=Repositories)

<!----- Categories ----->
[![Category badge](https://img.shields.io/badge/Category-SECURE%20PROVISIONING-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+sec_provi+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-SENSOR-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+sensor+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-INDUSTRIAL-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+industrial+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-SECURITY-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+security+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-MOTOR%20CONTROL-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+motor_control+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-CLOUD%20CONNECTED%20DEVICES-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+cc_devices+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-RTOS-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+rtos+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-TIME%20SENSITIVE%20NETWORKING-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+tsn+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-GRAPHICS-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+graphics+in%3Areadme&type=Repositories)

<!----- Peripherals ----->
[![Peripheral badge](https://img.shields.io/badge/Peripheral-USB-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+usb+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-ADC-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+adc+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-CLOCKS-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+clocks+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-DISPLAY-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+display+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-DMA-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+dma+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-ETHERNET-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+ethernet+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-FLASH-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+flash+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-GPIO-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+gpio+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-I2C-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+i2c+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-CAN-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+can+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-WATCHDOG-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+watchdog+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-PWM-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+pwm+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-SENSOR-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+sensor+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-SPI-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+spi+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-TIMER-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+timer+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-UART-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+uart+in%3Areadme&type=Repositories)

<!----- Toolchains ----->
[![Toolchain badge](https://img.shields.io/badge/Toolchain-MCUXPRESSO%20IDE-orange)](https://github.com/search?q=org%3Anxp-appcodehub+mcux+in%3Areadme&type=Repositories)

Questions regarding the content/correctness of this example can be entered as Issues within this GitHub repository.

>**Warning**: For more general technical questions regarding NXP Microcontrollers and the difference in expected functionality, enter your questions on the [NXP Community Forum](https://community.nxp.com/)

[![Follow us on Youtube](https://img.shields.io/badge/Youtube-Follow%20us%20on%20Youtube-red.svg)](https://www.youtube.com/@NXP_Semiconductors)
[![Follow us on LinkedIn](https://img.shields.io/badge/LinkedIn-Follow%20us%20on%20LinkedIn-blue.svg)](https://www.linkedin.com/company/nxp-semiconductors)
[![Follow us on Facebook](https://img.shields.io/badge/Facebook-Follow%20us%20on%20Facebook-blue.svg)](https://www.facebook.com/nxpsemi/)
[![Follow us on Twitter](https://img.shields.io/badge/Twitter-Follow%20us%20on%20Twitter-white.svg)](https://twitter.com/NXP)

## 8. Release Notes<a name="step8"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.2     | Full-featured release + bug fixes. | February 29<sup>th</sup> 2024 |
| 1.1     | Bug fixes and additional features including Cryptography, Watchdogs and the Secure Bootloader. | August 31<sup>st</sup> 2023 |
| 1.0     | Initial release on Application Code Hub        | December 19<sup>th</sup> 2022 |
