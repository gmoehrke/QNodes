# QNodes
 QNodes - MQTT Based Firmware Controller for ESP8266


Overview 

I developed this framework and firmware with a few simple goals in mind:

    1.  Create a single firmware build that can be used on multiple ESP8266 modules, each with different components and functions
    2.  Utilize MQTT for all communication and configuration of the devices
    3.  Include logging capabilities for visibility into node activity
    4.  Include OTA update capabilities so hardware doesn't have to be "touched" once deployed
    5.  Support a variety of different hardware components (sensors, LEDs, relays and switches)

QNodes is a set of Arduino/C++ classes that provides these capabilities.  The example found in examples/QNodeMaster.cpp is the main firmware sketch I currently have deployed in 10 nodes in my current configuration.  The framework currently has support for the following complonents:

    - Single-pin PIR sensors - providing motion detection with customizable time-outs, direct control through MQTT, overrides and countdowns
    - DHT22 Temperature and Humidity sensors
    - LDR light sensors (single analog pin)
    - Voltage sensors (single analog pin)
    - Relay controllers
    - Rudimentary X10 device controllor (through interface with MochaD server daemon)
    - Control and polling of TPLink Kasa switches
    - Control (On/Off/PWM) of single-color LEDs
    - Control (RGB Color/Brightness/Blending and Fading) of RGB LEDs
    - LED Pixels - using FastLED and FastFX framework

Each node is independently configured by retained messages (JSON) on its unique configuration topic (in MQTT).  Configuration is completely dynamic and can be changed on-the-fly by publishing updated configuration messages.

more to come...