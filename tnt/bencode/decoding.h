#pragma once

#include "types.h"
#include <memory>


namespace Bencode {
    std::shared_ptr<Entity> ReadEntity(std::istream &stream);
    std::shared_ptr<Integer> ReadInteger(std::istream &stream);
    std::shared_ptr<String> ReadString(std::istream &stream);
    std::shared_ptr<List> ReadList(std::istream &stream);
    std::shared_ptr<Dict> ReadDict(std::istream &stream);

    std::istream& operator>>(std::istream&, std::shared_ptr<Entity>&);
}
