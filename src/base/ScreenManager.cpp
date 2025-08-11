#include "ScreenManager.h"

// Static singleton instance
ScreenManager& ScreenManager::getInstance() {
    static ScreenManager instance;
    return instance;
}

void ScreenManager::addScreen(const std::string& name, std::shared_ptr<Screen> screen) {
    screens_[name] = screen;
}

void ScreenManager::removeScreen(const std::string& name) {
    if (activeScreen_ && activeScreen_->getName() == name) {
        activeScreen_->onUnload();
        activeScreen_.reset();
    }
    screens_.erase(name);
}

bool ScreenManager::switchTo(const std::string& name) {
    auto it = screens_.find(name);
    if (it == screens_.end()) {
        return false; // Screen not found
    }

    if (activeScreen_) {
        activeScreen_->onUnload();
    }

    activeScreen_ = it->second;
    activeScreen_->onLoad();
    return true;
}

void ScreenManager::update() {
    if (activeScreen_) {
        activeScreen_->onUpdate();
    }
}

std::shared_ptr<Screen> ScreenManager::getActiveScreen() const {
    return activeScreen_;
}

bool ScreenManager::goBack() {
    if (!activeScreen_) return false;
    const std::string& backName = activeScreen_->getBackScreen();
    if (backName.empty()) return false; // No back screen defined
    return switchTo(backName);
}
