nrf51-pwm-library
==================

This project includes a PWM library, and three examples showing how to use it. 
The PWM library can be configured with up to 4 PWM channels, and allows the PWM to run interrupted during high priority interrupts. Various configuration parameters exist to set the duty cycle, frequency, GPIO assignment, PPI channel usage and more. 

The library can be used with or without a SoftDevice, and a define exists in the header file to make the driver use the SoC API (required if you want to initialize the PMW driver after the SoftDevice is enabled). 

Requirements
------------
- nRF51 SDK version 7.0.1
- nRF51 DK

The project may need modifications to work with later versions or other boards. 

To compile it, clone the repository in the /examples/peripheral/ folder in the nRF51 SDK v7.0.1 or later (or any other folder under /examples/)

About this project
------------------
This application is one of several applications that has been built by the support team at Nordic Semiconductor, as a demo of some particular feature or use case. It has not necessarily been thoroughly tested, so there might be unknown issues. It is hence provided as-is, without any warranty. 

However, in the hope that it still may be useful also for others than the ones we initially wrote it for, we've chosen to distribute it here on GitHub. 

The application is built to be used with the official nRF51 SDK, that can be downloaded from https://www.nordicsemi.no, provided you have a product key for one of our kits.

Please post any questions about this project on https://devzone.nordicsemi.com.