#include "Actions.h"

void Actions::doAction(const std::string& actionName, const std::string& param) {
    auto it = actions_.find(actionName);
    if (it != actions_.end()) {
        it->second(param);
    }
}

void Actions::addAction(const std::string& name, ActionFunc func) {
    actions_[name] = func;
}
