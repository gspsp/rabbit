//
// Created by SP on 2022/7/11.
//

#ifndef VKML_H
#define VKML_H

#include <arduino.h>

class VKML {
private:

public:
    String text;

    void set(String key, String value);

    String get(String key);
};

void VKML::set(String key, String value) {
    //判断是否已存在
    int left = this->text.indexOf(key);
    if (left != -1) {
        //存在
        int right = this->text.indexOf(";", left);
        if (value == "") {
            //数据为空
            this->text = this->text.substring(0, left) + this->text.substring(right + 1);
        } else {
            //数据不为空
            this->text = this->text.substring(0, left) + key + ":" + value + ";" + this->text.substring(right + 1);
        }
    } else {
        //不存在
        if (value == "") {
            //数据为空
            return;
        } else {
            //数据不为空
            this->text += key + ":" + value + ";";
        }
    }
}

String VKML::get(String key) {
    //判断是否已存在
    int left = this->text.indexOf(key);
    if (left != -1) {
        //存在
        int right = this->text.indexOf(";", left);
        return this->text.substring(left + key.length() + 1, right);
    } else {
        //不存在
        return "";
    }
}

#endif //VKML_H
