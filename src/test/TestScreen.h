#pragma once
#include "base/Screen.h"
#include "TestComponent.h"
#include <memory>
#include <iostream>

class TestScreen : public Screen {
public:
    TestScreen(const std::string& name, const std::string& backScreen = "")
        : Screen(name, backScreen) 
    {
        addComponent(std::make_shared<TestComponent>("test"));
    }

    void onLoad() override {
        std::cout << "Loading screen: " << name_ << std::endl;
        Screen::onLoad();  // call base to load components and modules
    }

    void onUpdate() override {
        std::cout << "Updating screen: " << name_ << std::endl;
        Screen::onUpdate(); // update components and modules
    }

    void onUnload() override {
        std::cout << "Unloading screen: " << name_ << std::endl;
        Screen::onUnload(); // unload components and modules
    }
};
