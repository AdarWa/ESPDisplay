#pragma once
#include "base/PageBase.h"
#include "BaseTest.h"

void load_pages(){
    PageManager& pm = PageManager::getInstance();
    auto page1 = std::make_shared<BaseTest>();
    page1->addModule(std::make_shared<TimerModule>());
    pm.registerPage("home", page1);
}