#include "Module.h"

Module::Module()
    : states_(std::make_unique<States>()), actions_(std::make_unique<Actions>()) {}

States* Module::getStates() const {
    return states_.get();
}

Actions* Module::getActions() const {
    return actions_.get();
}

void Module::config() {}

void Module::onLoad() {}
void Module::onUpdate() {}
void Module::onUnload() {}
