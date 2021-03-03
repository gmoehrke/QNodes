# QNodes
*MQTT Based Controller Firmware and Framework for ESP8266*


## Overview 

Note:  This is a fully working project and all code is up-to-date.  Documentation is in-progress - but far from comprehensive at this point.  This started as more of a learning exercise for me as I wanted to incorporate a few micro-controllers (primarily for LED lighting) into my home.  As I reviewed other projects, I was getting frustrated because I really wanted to minimize maintenance and configuration.  I decided that if I was going to use MQTT for control and communication, why not use that for configuration as well?  My goal became to centralize all communication and configuration around MQTT and create a single firmware image that would allow me to do that with a variety of uses.  I currently have 10 live nodes running in my home.  Several for motion detection, 3 LED Strip controllers, 2 temperature/humidity sensors and 2 light detectors, and 1 node that manages a handfull of on/off switches (TPLink Kasa and older X10 switches) through a central MQTT service (on a Raspberry Pi). 

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
    - LED Pixels - using FastLED and FastFX framework - full control via MQTT command messages

Each node is independently configured by retained messages (JSON) on its unique configuration topic (in MQTT).  Configuration is completely dynamic and can be changed on-the-fly by publishing updated configuration messages.

## Basic Architecture

Each nodes contains a set of "Item Controllers".  These controllers are allocated and configured dynamically at bootup, and can also be configured on-the-fly by publishing configuration messages to the appropriate MQTT topic.  Each controller has 3 core interfaces - commands, state and events.  Commands are consumed by the controller and instruct it do do something (open a relay, light a LED, change an effect, etc).  Each controller is configured with a topic where it publishes its current state, this can include both summary information (ex. on/off, temperature, brightness, etc) and also more detailed state information (ex.  number of seconds remaining, timestamp of last change, trigger information, etc.)  Finally, each controller can publish "events" to a specified topic.  Events are things of interest that may or may not affect the state (ex. a notification was sent, a command was received, motion detection was overridden, etc.)   

Each item controller has a timer controller, which can control how often the controller will update.  Depending on the controller type, some may need to update every cycle, others may only need to update every few seconds, mintes or even hours.  The update cycle is where the controller will check hardware interfaces for state changes (ex. read the temperature, check for motion, read voltage, etc.)  Regardless of the update cycle, command messages are processed as they arrive on the associated topic, and events are generated as they occur (whether they arise as a result of a command or update operation).

### Example - Motion Detection

The PIR Item controller is probably one of the simpler examples to illustrate the basics of how the controllers work.  Once configured, it will publish basic status on the configured MQTT topic.  If configure to publish to the home/room/motion/state topic, when it starts it will publish the following:

home/room/motion/state:  { "motion":"off" }

Once motion is detected, it will publish a "motion on" state and also 3 sub-topics giving some further detail on the new state:

home/room/motion/state:  { "motion":"on" }
home/room/motion/state/timeout:  60   <this is the number of seconds retaining until motion is turned off>
home/room/motion/state/source: sensor <this is the source of what triggered the motion - more on this below>
home/room/motion/state/override:  none <this is the override status - more on this below>

the home/room/motion/state/timeout value will continue be pulished each second until it reaches 0, at which point, we will get the following:

home/room/motion.state:  { "motion":"off"}
home/room/motion/state/timeout: 0

If the controller is configured to accept commands on the topic home/room/motion/commands, I can send a command to trigger motion on the controller by publishing the following message:

home/room/motion/commands:  { "trigger":"on" }

The state will be updated as follows:

home/room/motion/state:  { "motion":"on" }
home/room/motion/state/timeout:  60  
home/room/motion/state/source: external
home/room/motion/state/override:  none 

The PIR item controller accepts the following commands, by default

{ "timeout": nnn }  - where nnn is the number of milliseconds after triggered until motion is turned off
{ "override": "none"/"on"/"off" } - sets override to on or off until set back to "none"
{ } 



## Basic Configuration

WiFi and MQTT Connections are all handled internally within the framework.  The example sketch (examples/QNodeMaster.cpp) has constants setup for SSID, WiFi password, MQTT host and associated credentials.  These are passed into the constructor of the QNodeController object, which does all the work.  If desired, something like WiFiManager could be used to manage these independently - I chose not to wire those into this framework.  

### Item Controllers

The QNodes framework uses a central controller, which manages a list of "Item Controllers".  Each item controller handles the interface to a particular component that is being controlled/monitored by the framework.  The number of item controllers is limited only by the physical and memory limitations of the microprocessor itself.  Each item controller has, by default, 3 topics where it will communicate with MQTT:

  1.  statetopic - this is the topic where the controller updates its state
  2.  commandtopic - this topic is used to send commands to the controller.  I can manually publish a message to this topic to instruct the controller to perform an action (ex.  trigger motion, turn on/off, change color, flash, change settings, etc.)
  3.  eventtopic - this is used by the controller to publish events that may be of interest.  For example, the PIR controller publishes details about trigger events (was it triggered by hardware, or manually by a command, has an ovverride command been issued, etc.)

## Configuration Tool

The examples directory contains a Python configuration script that may be used to configure a single node, or a group of nodes from a single JSON file.  The script will break up the json into individual messages and publish them on the appropriate topics in MQTT as descibed in the Manual Configuration section below.  Here is an example JSON configuration file for a single node containing an RGB LED wired to pins 5, 4 and 0 (D1, D2, and D3 on the NodeMCU board) and a PIR/Motion detector wired to pin 13 (D7):

```json
{
  "Nodes" : [
    {
      "ID" : "ESP-DDEEFF",
      "hostname" : "QNODE000",
      "config" : {
        "items" : [
          {
            "tag" : "HOST",
            "config" : {
              "statetopic" : "qn/nodes/QNODE000/status",
              "commandtopic" : "qn/nodes/QNODE000/commands",
              "eventtopic" : "qn/nodes/QNODE000/events"
            }
          },
          {
            "tag" : "PIR",
            "id" : "MOTION1",
            "config" : {
              "statetopic" : "home/foyer/motion/state",
              "commandtopic" : "home/foyer/motion/commands",
              "eventtopic" : "home/foyer/motion/events",
              "pirpin" : 13,
              "timeout" : 300000
            }
          },
          {
            "tag" : "LED",
            "config" : {
              "statetopic" : "qn/nodes/QNODE000/LED/state",
              "commandtopic" : "qn/nodes/ESP-DDEEFF/LED/commands",
              "RGBPins" : {
                "r" : 5,
                "g" : 4,
                "b" : 0
              }
            }
          }
        ]
      }
    }
  ]
}
```

Running the script in test mode, will show a list of all messages that will be published.  From the examples directory, run the following:

```
python3 QNode_config.py -i qn_config.json -q myMQTTserver -u myuser -p mypassword -r qn/nodes -t
```
The parameters are as follows:

  - -i or --input : Specifies the JSON file with the configuration
  - -q or --mqtt_host : The name of the local MQTT server
  - -y or --user : the username for authentication on the MQTT server
  - -p or --password : the password for authentication on the MQTT server
  - -r or --configroot : the root topic for the configuration messages (the QNodes framework defaults to qn/nodes)
  - -t or --test : runs in test mode, sends no messages to MQTT, just displays what would be sent

The output of the above command is as follows:

```
QNode Config Tool v0.7

Reading config JSON file from:  qn_config.json

Node:          ESP-DDEEFF
  Name:        QNODE000
  Controllers: 3
  Messages (topic : payload):
    qn/nodes/ESP-DDEEFF/config : {"hostname": "QNODE000"}
    qn/nodes/QNODE000/config : {"description": "QNODE000", "items": [{"tag": "HOST"}, {"tag": "PIR", "id": "MOTION1"}, {"tag": "LED"}]}
    qn/nodes/QNODE000/config/HOST : {"statetopic": "qn/nodes/QNODE000/status", "commandtopic": "qn/nodes/QNODE000/commands", "eventtopic": "qn/nodes/QNODE000/events"}
    qn/nodes/QNODE000/config/MOTION1: {"statetopic": "home/foyer/motion/state", "commandtopic": "home/foyer/motion/commands", "eventtopic": "home/foyer/motion/events", "pirpin": 13, "timeout": 300000}
    qn/nodes/QNODE000/config/LED : {"statetopic": "qn/nodes/QNODE000/LED/state", "commandtopic": "qn/nodes/ESP-DDEEFF/LED/commands", "RGBPins": {"r": 5, "g": 4, "b": 0}}
```

Running the coammand without thei -t parameter will send the listed messages to the appropriate topics on the MQTT server.  Once that is completed and the firmware is installed on the board (with a chip id of ESPDDEEFF, in this case), the board will do the following upon boot-up:

  - Boot and startup-WiFi (with the configured SSID and key)
  - Connect to MQTT and look for a message on the qn/nodes/ESP-DDEEFF/config topic
  - Find a message with the hostname key and rename itself to QNODE000 (and restart WiFi connection as QNODE000)
  - The controller now reinitializes and looks for messages at qn/nodes/QNODE000/config
  - It finds a list of items containing three controllers:  HOST, PIR, and LED
  - as each of those controllers is added, they will look for their configuration messages the main configuration topic using the item's "tag" or the item's "id", if specified.  The "id" can be used to differentiate between more than one item with the same tag.  So in the example, our three item controllers will look in the following topics for configuration messages: 
    - qn/nodes/QNODE000/config/HOST
    - qn/nodes/QNODE000/config/MOTION1
    - qn/nodes/QNODE000/config/LED

## Manual Configuration

Once connected to MQTT, all confuguration is done through retained messages on established MQTT topics.  On initialization, the node will look for messages on its basic confirutation topic, which defaults to qn/nodes/<nodename>/config.  The initial <nodename> is set to the default Chip ID returned by the ESP8266 Arduino library - it will be something like 'ESP-DDEEFF' (where DDEEFF are the last 3 hex digits of the MAC Address).  The config topic publishes a retained message that instructs the node which controllers to activate.  Example:

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

...more to come...