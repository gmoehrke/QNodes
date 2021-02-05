# QNodes
*MQTT Based Controller Firmware and Framework for ESP8266*


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

Basic Configuration

WiFi and MQTT Connections are all handled internally within the framework.  The example sketch (examples/QNodeMaster.cpp) has constants setup for SSID, WiFi password, MQTT host and associated credentials.  These are passed into the constructor of the QNodeController object, which does all the work.  If desired, something like WiFiManager could be used to manage these independently - I chose not to wire those into this framework.  

Once connected to MQTT, all confuguration is done through messages on established MQTT topics.  On initialization, the node will look for messages on its basic confirutation topic, which defaults to qn/nodes/<nodename>/config.  The initial <nodename> is set to the default Chip ID returned by the ESP8266 Arduino library - it will be something like 'ESP-DDEEFF' (where DDEEFF are the last 3 hex digits of the MAC Address).  The config topic publishes a retained message that instructs the node which controllers to activate.  Example:

```json
{"items":[{"tag":"HOST"},{"tag":"LED"},{"tag":"PIR", "name":"MOTION_SENSOR"}]}
```

This example configuration spins up three "item" controllers.  A "HOST" controller, which allows firmware updates and control of the on-board LED(s) of the controller board.  A LED controller, which controls a single RGB LED (attached to 3 pins and ground).  Finally, a PIR sensor (attached to a single pin and ground).  Each controller is internally identified by a unique "name", which defaults to the tag element in the config message.  In this example, the PIR item controller's name is overridden to "MOTION_SENSOR" (by the name key in the JSON), the others will be named "HOST" and "LED" respectively.

Item Controller Configuration

For each item controller in the above list, a corresponding sub-topic of the main config topic will publish a retained message containg the configuration settings for that item.  In the example, the main configuration topic was qn/nodes/ESP-DDEEFF/config.  Based on the example configuration, we will have 3 sub-topics as follows:

    qn/nodes/ESP-DDEEFF/config/HOST
    qn/nodes/ESP-DDEEFF/config/LED
    qn/nodes/ESP-DDEEFF/config/MOTION_SENSOR

The configuration messages for each item controller instruct the controller where to communicate status, where to receive instructions/commands and where to publish event information.  Here are the three messages published by our example configuration:

**qn/nodes/ESP-DDEEFF/config/HOST**
```json 
{
    "statetopic":"qn/nodes/ESP-DDEEFF/status",
    "commandtopic":"qn/nodes/ESP-DDEEFF/commands",
    "eventtopic":"qn/nodes/ESP-DDEEFF/ events"
}
```
**qn/nodes/ESP-DDEEFF/config/LED**
```json
{
    "statetopic":"qn/nodes/ESP-DDEEFF/LED/state",
    "commandtopic":"qn/nodes/ESP-DDEEFF/LED/commands",
    "RGBPins":
    { 
        "r":5,
        "g":4,
        "b":0
    }
}
```
**qn/nodes/ESP-DDEEFF/config/MOTION_SENSOR**
```json
{
    "statetopic":"home/foyer/motion/state",
    "commandtopic":"home/foyer/motion/commands",
    "eventtopic":"home/foyer/motion/events",
    "pirpin":13,
    "timeout":300000
}
```
By publishing these 4 JSON messages, I have fully configured my node.  If I install the compiled firmware and boot the node, it will be fully configured and will be accepting commands, publishing states and publishing events on the configured MQTT topics.  Provided I have a PIR sensor wired to pin 13, I will begin seeing state messages on the associated topic (home/foyer/motion/state).  Provided I have a RGB LED wired to pins 5,4 and 0, I can control the LED by sending messages to the qn/nodes/ESP-DDEEFF/LED/commands topic.  I can control the on-board LED(s) of the controller, update the firmware, or restart the node be sending messages to the qn/nodes/ESP-DDEEFF/commands topic.

more to come...