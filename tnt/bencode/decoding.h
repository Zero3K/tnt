#pragma once

#include "types.h"
#include <memory>


namespace Bencode {
    /*
     * Read bencode entity from `stream`.
     */
    std::shared_ptr<Entity> ReadEntity(std::istream &stream);

    /*
     * Read bencode integer from `stream`.
     */
    std::shared_ptr<Integer> ReadInteger(std::istream &stream);

    /*
     * Read bencode string from `stream`.
     */
    std::shared_ptr<String> ReadString(std::istream &stream);

    /*
     * Read bencode list from `stream`.
     */
    std::shared_ptr<List> ReadList(std::istream &stream);

    /*
     * Read bencode dictionary from `stream`.
     */
    std::shared_ptr<Dict> ReadDict(std::istream &stream);

    std::istream& operator>>(std::istream&, std::shared_ptr<Entity>&);
}
