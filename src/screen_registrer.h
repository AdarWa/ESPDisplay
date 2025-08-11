#pragma once
#include "base/ScreenManager.h"
#include "utils/stdutils.h"
#include <memory>
#include "test/TestScreen.h"

void register_screens(){
    ScreenManager::getInstance().addScreen("TestScreen", std::make_shared<TestScreen>("TestScreen"));
}