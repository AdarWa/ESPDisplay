#pragma once
#include <vector>
#include <memory>
#include "Module.h"

class Page {
protected:
    std::vector<std::unique_ptr<Module>> modules;

public:
    virtual ~Page() = default;

    // Add a module to the page
    void addModule(std::unique_ptr<Module> mod) {
        modules.push_back(std::move(mod));
    }

    // Called when the page is displayed
    virtual void onLoad() {
        for (auto& m : modules) m->onLoad();
    }

    // Called when the page is hidden
    virtual void onUnload() {
        for (auto& m : modules) m->onUnload();
    }

    // Update called periodically (e.g. LVGL task)
    virtual void update() {
        for (auto& m : modules) m->update();
    }

    // Render the page main UI
    virtual void render(lv_obj_t* parent) = 0;

    // Render the advanced settings screen for this page and its modules
    virtual void renderAdvancedSettings(lv_obj_t* parent) {
        // Render page-specific settings first
        renderPageSettings(parent);

        // Render settings for each module
        for (auto& m : modules) {
            m->renderSettings(parent);
        }
    }

protected:
    // Render page-specific settings (overridable)
    virtual void renderPageSettings(lv_obj_t* parent) = 0;
};