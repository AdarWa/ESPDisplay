#include "Component.h"

Component::Component(const std::string& uuid)
    : uuid_(uuid), states_(std::make_unique<States>()), actions_(std::make_unique<Actions>()) {}

const std::string& Component::getUUID() const {
    return uuid_;
}

States* Component::getStates() const {
    return states_.get();
}

Actions* Component::getActions() const {
    return actions_.get();
}

lv_obj_t* Component::render(lv_obj_t* parent) {
    return lv_obj_create(parent);
}

void Component::onLoad() {}
void Component::onUpdate() {}
void Component::onUnload() {}
