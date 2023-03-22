# This is a sample Python script.

import paho.mqtt.client as mqtt
import json
import argparse


def load_config(filename):
    with open(filename) as json_file:
        print("Reading config JSON file from:  " + filename)
        return [json.load(json_file)]


def parse_json(config_topic, config):
    result = list()
    for cfg in config:
        for node in cfg['Nodes']:
            curr_topic = config_topic + '/' + node['ID'] + '/config'
            node_name = (node['hostname'] if 'hostname' in node.keys() else node['ID'])
            # if hostname key is present, it will be the only config item under the node_id config topic
            # all others go to config/<hostname> topic
            result.append({'type': 'node',
                           'id': node['ID'],
                           'nodename': node_name,
                           'controllers': len(node['config']['items'])})
            if "hostname" in node.keys():
                # extract the hostname config key/value and write to config topic under the chip ID
                curr_payload = json.dumps({k: v for k, v in node.items() if k == 'hostname'}).strip(" ")
                result.append({'type': "pub", 'nodename': node_name, 'topic': curr_topic, 'payload': curr_payload})
                curr_topic = config_topic + '/' + node['hostname'] + '/config'
            # build the JSON array of items/controllers
            curr_payload = json.dumps(
                {'description' : node['config']['description'] if 'description' in node['config'].keys() else node_name,
                 'items': [{k: v for k, v in item.items() if k != 'config'} for item in node['config']['items']]})
            result.append({'type': 'pub', 'nodename': node_name, 'topic': curr_topic, 'payload': curr_payload})
            for item in node['config']['items']:
                result.append({'type': 'pub',
                               'nodename': node_name,
                               'topic': (curr_topic + "/" + (item['id'] if ('id' in item.keys()) else item['tag'])),
                               'payload': json.dumps(item['config'])})
        return result


def publish_config(mqtt_host, mqtt_username, mqtt_password, msg_list, name=None):
    client = mqtt.Client()
    client.username_pw_set(mqtt_username, mqtt_password)
    client.connect(mqtt_host)
    for msg in msg_list:
        if msg['type'] == 'pub' and (name is None or name == msg['nodename']):
            client.publish(msg['topic'], msg['payload'], retain=True)
    client.disconnect()
    return


def print_config(msg_list, name=None):
    for msg in msg_list:
        if msg['type'] == 'node' and (name is None or name == msg['nodename']):
            print("")
            print("Node:          " + msg['id'])
            print("  Name:        " + msg['nodename'])
            print("  Controllers: " + str(msg['controllers']))
            print("  Messages (topic : payload):")
        elif msg['type'] == 'pub' and (name is None or name == msg['nodename']):
            print("    " + msg['topic'] + " : " + msg['payload'])
    return

this_name = "QNode Config Tool"
version = 0.7

# Parse commandline arguments
parser = argparse.ArgumentParser(description=this_name + " v" + str(version))
parser.add_argument("-i", "--input", required=True, help="JSON configuration file")
parser.add_argument("-t", "--test", action='store_true', help="Only show configuration messages, don't publish to MQTT")
parser.add_argument("-q", "--mqtt_host", required=True, help="MQTT Host IP/Name" )
parser.add_argument("-u", "--user", required=True, help="MQTT username" )
parser.add_argument("-p", "--password", required=True, help="MQTT password" )
parser.add_argument("-r", "--configroot", required=True, help="MQTT topic that contains all configuration sub-topics" )
parser.add_argument("-n", "--node", required=False, help="Single node name for output, otherwise process ALL nodes" )

print(this_name + " v" + str(version))
print("")

args = parser.parse_args()
messages = parse_json(args.configroot, load_config(args.input))
print_config(messages, args.node )
if (args.test is None or not args.test):
  print("Publishing messages to MQTT Broker at :" + args.mqtt_host)
  publish_config(args.mqtt_host, args.user, args.password, messages, args.node)

