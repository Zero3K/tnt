#pragma once

#include "../info_row.h"
#include <string>
#include <functional>
#include <tuple>
#include <thread>


class EmptyRow : public InfoRow {
public:
    std::string GetValue() override;
};
