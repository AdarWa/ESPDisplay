#include "Module.h"

Module::Module()
    : states_(std::make_unique<States>()) {}

States* Module::getStates() const {
    return states_.get();
}

void Module::config() {}

void Module::onLoad() {}
void Module::onUpdate() {}
void Module::onUnload() {}
