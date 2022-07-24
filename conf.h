//
// Created by SP on 2022/7/10.
//

#ifndef CONF_H
#define CONF_H

#include <arduino.h>
#include <EEPROM.h>
#include "vkml.h"

class CONF {
private:
    String text;
public:
    CONF();

    ~CONF();

    void set(String key, String value);

    String get(String key);
};

union rope_int_byte {
    int n;
    byte b[2];
};
rope_int_byte t;

CONF::CONF() {
    EEPROM.begin(1024);
    //初始化
    t.b[0] = EEPROM.read(0);
    t.b[1] = EEPROM.read(1);
    if (t.n != 1234) {
        t.n = 1234;
        EEPROM.write(0, t.b[0]);
        EEPROM.write(1, t.b[1]);
        for (int i = 2; i < 1024; ++i) {
            EEPROM.write(i, 0);
        }
        EEPROM.commit();
    }
    t.b[0] = EEPROM.read(2);
    t.b[1] = EEPROM.read(3);
    for (int i = 0; i < t.n; ++i) {
        this->text += (char) EEPROM.read(i + 4);
    }
}

CONF::~CONF() {
    EEPROM.end();
}

void CONF::set(String key, String value) {
    //判断是否已存在
    VKML data;
    data.text = this->text;
    data.set(key, value);
    this->text = data.text;
    //写入
    t.n = this->text.length();
    EEPROM.write(2, t.b[0]);
    EEPROM.write(3, t.b[1]);
    for (int i = 0; i < t.n; ++i) {
        EEPROM.write(i + 4, int(this->text[i]));
    }
    EEPROM.commit();
}

String CONF::get(String key) {
    VKML data;
    data.text = this->text;
    return data.get(key);
}

#endif //CONF_H
