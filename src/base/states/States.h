#pragma once
#include <map>
#include <string>

class States {
public:
    using StateMap = std::map<std::string, std::string>;

protected:
    StateMap states_;

public:
    virtual ~States() = default;

    virtual const StateMap& getStates() const;
    virtual void publish();
    virtual void onChange(const std::string& key, const std::string& value);
};
