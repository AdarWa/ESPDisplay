#pragma once
#include <map>
#include <string>

class States {
public:
    using StateMap = std::map<std::string, std::string>;
    using Callback = std::function<void>;
    using CallbackList = std::vector<Callback>;

protected:
    StateMap states_;
    CallbackList callbacks_;

public:
    ~States() = default;

    const StateMap& getStates() const;
    void publish();
    void addCallback(Callback callback);
};
