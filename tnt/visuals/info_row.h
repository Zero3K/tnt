#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <thread>


class InfoRow {
public:
    /*
     * Get string to display.
     */
    virtual std::string GetValue() = 0;
};