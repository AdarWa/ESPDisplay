#pragma once
#include <map>
#include <string>
#include <functional>

class Actions {
public:
    using ActionFunc = std::function<void(const std::string&)>;
    using ActionMap = std::map<std::string, ActionFunc>;

protected:
    ActionMap actions_;

public:
    virtual ~Actions() = default;

    virtual void doAction(const std::string& actionName, const std::string& param);
    void addAction(const std::string& name, ActionFunc func);
};
