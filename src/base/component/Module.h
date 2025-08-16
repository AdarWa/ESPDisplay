#pragma once
#include "../Entity.h"
#include "../states/States.h"
#include "utils/stdutils.h"
#include <memory>

class Module : public Entity {
protected:
    std::unique_ptr<States> states_;

public:
    Module();
    virtual ~Module() = default;

    States* getStates() const;

    virtual void config();

    void onLoad() override;
    void onUpdate() override;
    void onUnload() override;
};
