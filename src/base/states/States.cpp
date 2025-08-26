#include "States.h"

const States::StateMap& States::getStates() const {
    return states_;
}

void States::publish() {
    // Stub for publishing states
}

void States::addCallback(Callback callback){
    callbacks_.push_back(callback);
}
