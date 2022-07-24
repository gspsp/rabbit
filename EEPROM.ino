#include "conf.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "vkml.h"
#include "tasks.h"
#include <Ticker.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <Servo.h>

Servo servo;

VKML vkml;

WiFiUDP Udp;
char udpPacketBuffer[UDP_TX_PACKET_MAX_SIZE + 1];

DNSServer dnsServer;

Ticker ticker;

CONF conf;
WiFiClient wifiClient;
PubSubClient MQTT(wifiClient);

void (*resetFunc)(void) = 0;

void mqttCallback(char *topic, byte *payload, unsigned int length);

void connectWiFi();

void connectMQTT();

void udpServerLoop();

String ssid = conf.get("ssid"), pwd = conf.get("pwd"), sn = conf.get("sn"), sk = conf.get("sk");

TASKS fastLoopTasks;
TASKS slowLoopTasks;
bool slowLoopReady = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    //检测是否被设置过
    if (conf.get("status") == "") {
        WiFi.mode(WIFI_AP_STA);
        //没有配网，开启热点
        Serial.print("Setting soft-AP ... ");
        Serial.println(WiFi.softAP("rabbit-chameleon", "") ? "Ready" : "Failed!");
        Serial.println("IP address: ");
        Serial.println(WiFi.softAPIP());
        //开启dns服务器
        dnsServer.start(53, "rabbit.com", WiFi.softAPIP());
        fastLoopTasks.add([]() { dnsServer.processNextRequest(); });
        //开启udp
        Udp.begin(8888);
        fastLoopTasks.add([]() { udpServerLoop(); });
    } else {
        //已经配网连接wifi
        connectWiFi();
        //开启mqtt
        connectMQTT();
        fastLoopTasks.add([]() { MQTT.loop(); });
        slowLoopTasks.add([]() {
            //定时检测wifi连接状态
            if (WiFi.status() != WL_CONNECTED) {
                connectWiFi();
            }
            //定时检测mqtt连接状态
            if (!MQTT.connected()) {
                connectMQTT();
            }
        });
        servo.attach(13);
    }
//    conf.set("tip","hello");
//    Serial.println(conf.get("tip"));
    pinMode(16, INPUT);
    ticker.attach(1, []() {
        slowLoopReady = true;
    });
}

void loop() {
    fastLoopTasks.run();
    if (digitalRead(16) == LOW) {
        conf.set("status", "");
        delay(1000);
        Serial.println("设备已重置！");
        resetFunc();
    }
    if (slowLoopReady) {
        //slowLoop
        slowLoopTasks.run();
        slowLoopReady = false;
    }
}


void mqttCallback(char *topic, byte *payload, unsigned int length) {
    char message[5] = {0x00};
    for (int i = 0; i < length; i++)
        message[i] = (char) payload[i];
    message[length] = 0x00;
    VKML data;
    data.text = String(message);
    String cmd = data.get("cmd");
    Serial.println(data.text);
    if (cmd == "reset") {
        conf.set("status", "");
        MQTT.publish(("from-" + sn + "-" + sk).c_str(), "设备已重置！");
        delay(1000);
        resetFunc();
    } else if (cmd == "hello") {
        MQTT.publish(("from-" + sn + "-" + sk).c_str(), "我在！");
    } else if (cmd == "turn") {
        int angle = data.get("angle").toInt();
        servo.write(angle);
        MQTT.publish(("from-" + sn + "-" + sk).c_str(), ("已转至" + String(angle) + "度！").c_str());
    }
}

//void httpCallback() {
//    conf.set("status", "already");
//    conf.set("ssid", WebServer.arg("ssid"));
//    conf.set("pwd", WebServer.arg("pwd"));
//    conf.set("sn", WebServer.arg("sn"));
//    conf.set("sk", WebServer.arg("sk"));
//    WebServer.send(200, "text/plain", "ok");
//    Serial.println(WebServer.arg("ssid"));
//    Serial.println(WebServer.arg("pwd"));
//    Serial.println(WebServer.arg("sn"));
//    Serial.println(WebServer.arg("sk"));
//    delay(1000);
//    resetFunc();
//}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pwd.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
        if (digitalRead(16) == LOW) {
            conf.set("status", "");
            delay(1000);
            Serial.println("设备已重置！");
            resetFunc();
        }
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Way IP: ");
    Serial.println(WiFi.gatewayIP().toString());
    Serial.println(WiFi.hostname());
}

void connectMQTT() {
    MQTT.setServer("119.29.115.242", 1883);
    MQTT.setCallback(mqttCallback);
    if (MQTT.connect((sn + "-" + sk).c_str(), NULL, NULL)) {
        Serial.println("Connected to MQTT Broker");
    } else {
        Serial.println("MQTT Broker connection failed");
        Serial.print(MQTT.state());
        delay(200);
    }
    MQTT.publish(("from-" + sn + "-" + sk).c_str(), "我他妈来啦！");
    MQTT.subscribe(("to-" + sn + "-" + sk).c_str());
}

void udpServerLoop() {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                      packetSize,
                      Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                      Udp.destinationIP().toString().c_str(), Udp.localPort(),
                      ESP.getFreeHeap());

        // read the packet into udpPacketBuffer
        int n = Udp.read(udpPacketBuffer, UDP_TX_PACKET_MAX_SIZE);
        udpPacketBuffer[n] = 0;
        Serial.println("Contents:");
        Serial.println(udpPacketBuffer);
        //验证是否为配置信息
        vkml.text = udpPacketBuffer;
        if (vkml.get("cmd") == "set") {
            ssid = vkml.get("ssid"), pwd = vkml.get("pwd"), sn = vkml.get("sn"), sk = vkml.get("sk");
            //回复ok
            Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
            Udp.write("201");
            Udp.endPacket();
            //打印信息
            Serial.println(ssid);
            Serial.println(pwd);
            Serial.println(sn);
            Serial.println(sk);
            //尝试连接
            Serial.println("正在尝试连接到WiFi");
            WiFi.begin(ssid.c_str(), pwd.c_str());
            int count = 0;
            while (WiFi.status() != WL_CONNECTED) {
                count++;
                if (count > 25) {
                    Serial.println("连接失败！");
                    //回复403
                    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
                    Udp.write("403");
                    Udp.endPacket();
                    return;
                }
                delay(500);
                Serial.print(F("."));
                if (digitalRead(16) == LOW) {
                    conf.set("status", "");
                    delay(1000);
                    Serial.println("设备已重置！");
                    resetFunc();
                }
            }
            //配置成功回复200
            Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
            Udp.write("200");
            Udp.endPacket();
            conf.set("status", "already");
            conf.set("ssid", ssid);
            conf.set("pwd", pwd);
            conf.set("sn", sn);
            conf.set("sk", sk);
            delay(1000);
            resetFunc();
        }
        // send a reply, to the IP address and port that sent us the packet we received

    }
}
