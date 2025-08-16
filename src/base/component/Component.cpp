#include "Component.h"

Component::Component(const std::string& uuid)
    : uuid_(uuid), states_(std::make_unique<States>()) {}

const std::string& Component::getUUID() const {
    return uuid_;
}

States* Component::getStates() const {
    return states_.get();
}


lv_obj_t* Component::render(lv_obj_t* parent) {
    return lv_obj_create(parent);
}

void Component::onLoad() {}
void Component::onUpdate() {}
void Component::onUnload() {}
