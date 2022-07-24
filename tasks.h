//
// Created by SP on 2022/7/12.
//

#ifndef TASKS_H
#define TASKS_H

struct node {
    void (*f)();

    node *last;
    node *next;
};

class TASKS {
private:
    node left = {[]() {}, nullptr, nullptr};
    node right = {[]() {}, nullptr, nullptr};
public:
    TASKS();

    void add(void (*f)());

    void run();
};

TASKS::TASKS() {
    this->left.next = &this->right;
    this->right.last = &this->left;
}

void TASKS::add(void (*f)()) {
    node *cur = new node();
    cur->f = f;
    this->right.last->next = cur;
    cur->last = this->right.last;
    cur->next = &this->right;
    this->right.last = cur;
}

void TASKS::run() {
    node *cur = this->left.next;
    while (cur != &this->right) {
        cur->f();
        cur = cur->next;
    }
}


#endif //TASKS_H
