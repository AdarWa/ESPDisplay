#include "States.h"

const States::StateMap& States::getStates() const {
    return states_;
}

void States::publish() {
    // Stub for publishing states
}

void States::onChange(const std::string& key, const std::string& value) {
    states_[key] = value;
    publish();
}
