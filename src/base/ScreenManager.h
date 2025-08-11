#pragma once
#include "Screen.h"
#include <memory>
#include <string>
#include <unordered_map>

class ScreenManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Screen>> screens_;
    std::shared_ptr<Screen> activeScreen_;

    // Private constructor for singleton
    ScreenManager() = default;

    // Prevent copying
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;

public:
    ~ScreenManager() = default;

    // Access singleton instance
    static ScreenManager& getInstance();

    void addScreen(const std::string& name, std::shared_ptr<Screen> screen);
    void removeScreen(const std::string& name);
    bool switchTo(const std::string& name);
    void update();
    std::shared_ptr<Screen> getActiveScreen() const;
    bool goBack();
};
