/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with ATOM DTU NB MQTT Client sample source code
* Visit the website for more
information：https://docs.m5stack.com/en/atom/atom_dtu_nb
* describe: ATOM DTU NB MQTT Clien Example.
* Libraries:
    - [TinyGSM - modify](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/lib/TinyGSM.zip)
    - [PubSubClient](https://github.com/knolleary/pubsubclient.git)
* date：2022/4/14
*******************************************************************************
*/

#include <Arduino.h>
#include <M5Atom.h>
#include <WiFi.h>

#include "ATOM_DTU_NB.h"
#include <PubSubClient.h>
#include <TinyGsmClient.h>
#include <time.h>
#include <sys/time.h>

#define MQTT_BROKER   "host.net"
#define MQTT_PORT     1883
#define MQTT_USERNAME "m5-dtu"
#define MQTT_PASSWORD "123"
#define MQTT_D_TOPIC  "/m5-dtu/D"
#define MQTT_U_TOPIC  "/m5-dtu/U"

#define UPLOAD_INTERVAL 10000
uint32_t lastReconnectAttempt = 0;

#if 1
    #include <StreamDebugger.h>
    StreamDebugger debugger(SerialAT, SerialMon);
    TinyGsm        modem(debugger,ATOM_DTU_SIM7020_RESET);
#else
    TinyGsm modem(SerialAT);
#endif

TinyGsmClient tcpClient(modem);
PubSubClient mqttClient(MQTT_BROKER, MQTT_PORT, tcpClient);

void mqttCallback(char *topic, byte *payload, unsigned int len);
bool mqttConnect(void);
void nbConnect(void);

// Your GPRS credentials, if any
const char apn[]      = "internet.t-mobile";
const char gprsUser[] = "t-mobile";
const char gprsPass[] = "tm";

//#define GSM_PIN		"9416"

char mqttIdent[32];

void log(String info) {
    SerialMon.println(info);
}

void setup() {
    M5.begin(true, false, true);
    delay(6000);
    log(">>ATOM DTU NB MQTT TEST");
#if 1
    SerialAT.begin(
        115200,
        SERIAL_8N1,
        ATOM_DTU_SIM7020_RX,
        ATOM_DTU_SIM7020_TX
    );
#else
    TinyGsmAutoBaud(SerialAT, 110, 460800L);
#endif
    M5.dis.fillpix(0x0000ff);
    nbConnect();
    mqttClient.setCallback(mqttCallback);
    
    sprintf(mqttIdent,"m5dtu-%s",WiFi.macAddress().c_str());

}

void loop() {
	char announceMsg[100];

    static unsigned long timer = 0;
    M5.update();

    if (!mqttClient.connected()) {
        log(">>MQTT NOT CONNECTED");
        // Reconnect every 10 seconds
        M5.dis.fillpix(0xff0000);
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 3000L) {
            lastReconnectAttempt = t;
            if (mqttConnect()) {
                lastReconnectAttempt = 0;
            }
        }
        delay(100);
    }
    if (millis() >= timer) {
        timer = millis() + UPLOAD_INTERVAL;
				sprintf(
					announceMsg,
					 "%s",
					 mqttIdent
				);

        mqttClient.publish("/m5-dtu/announce", announceMsg);
    }
    M5.dis.fillpix(0x00ff00);
    mqttClient.loop();
}

void mqttCallback(char *topic, byte *payload, unsigned int len) {
    char info[len];
    memcpy(info, payload, len);
    log("Message arrived [" + String(topic) + "]: ");
    log(info);
}

bool mqttConnect(void) {
    log("Connecting to ");
    log(MQTT_BROKER);
    
    // Connect to MQTT Broker
    bool status =
        mqttClient.connect(mqttIdent, MQTT_USERNAME, MQTT_PASSWORD);
    if (status == false) {
        log(" fail");
        return false;
    }
    log("MQTT CONNECTED!");
    mqttClient.publish(MQTT_U_TOPIC, "NB MQTT CLIENT ONLINE");
    mqttClient.subscribe(MQTT_D_TOPIC);
    return mqttClient.connected();
}

void nbConnect(void) {
    unsigned long start = millis();
    log("Initializing modem...");
    
    //modem.sendAT("+CRESET");
    //modem.waitResponse();
    
    //modem.factoryDefault();
    
    while (!modem.init()) {
        log("waiting for init...." + String((millis() - start) / 1000) + "s");
    };
    
    //** using this kills my modem **
    //modem.factoryDefault();

    String name = modem.getModemName();
    SerialMon.println("Modem Name: "+name);
#ifdef GSM_PIN
    if (GSM_PIN && (modem.getSimStatus() != 1)) {
    	modem.simUnlock(GSM_PIN);
    	SerialMon.println("SIM unlock...");
    }
#endif    
    start = millis();
    while (!modem.waitForNetwork()) {
        log("waiting for Network...." + String((millis() - start) / 1000) + "s");
        //CSQ response is mostly 99,99
        modem.sendAT("+CSQ");
        modem.waitResponse();
    }
    log("success");
    
    /*
    start = millis();
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        log("waiting for GPRS...." + String((millis() - start) / 1000) + "s");
    }
    log("success");
    */
}
