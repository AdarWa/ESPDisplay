#pragma once
#include <lvgl.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "Page.h"

class PageManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Page>> pages;
    std::shared_ptr<Page> currentPage;
    std::shared_ptr<Page> lastPage;

    PageManager() = default; // private constructor

    void loadCurrentPage();

public:
    static PageManager& getInstance(); // Singleton accessor

    void registerPage(const std::string& name, std::shared_ptr<Page> page);
    void loadPage(const std::string& name);
    void update();

    void backPage();

    PageManager(const PageManager&) = delete;
    PageManager& operator=(const PageManager&) = delete;
};