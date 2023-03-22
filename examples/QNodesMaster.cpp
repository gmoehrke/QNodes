#define SKETCH_VERSION "QNode Master Firmware 1.4.0"

#include <ArduinoJson.h>
#include "FlexTimer.h"
#include "QNodes.h"
#include "CoreControllers.h"

/************ WIFI and MQTT Information  ******************/
const char *ssid = "********"; 
const char *password = "**********";
const char *mqtt_server = "192.168.1.1";
const char *mqtt_username = "*********";
const char *mqtt_password = "*********";

/************* QNode base topics  **************************/
const String mqtt_t_hosts = "qn/nodes";
const String mqtt_st_config = "/config";

ADC_MODE(ADC_TOUT);

QNodeController *qnc = nullptr;

void setup() {
  Serial.begin(9600);
  CoreControllers::registerControllers(); 
}

/********************************** START MAIN LOOP*****************************************/
void loop() {
  if (!qnc) {
    qnc = new QNodeController( ssid, password, mqtt_server, mqtt_username, mqtt_password, mqtt_t_hosts );
    qnc->setSketchVersion( SKETCH_VERSION );
    //qnc->logTopic = QNodeController::LOG_TO_SERIAL;
    qnc->setLogLevel( QNodeController::LOGLEVEL_DEBUG ) ;
  }
  else {
    qnc->loop();
  }
}
