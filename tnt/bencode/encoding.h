#pragma once

#include "types.h"

namespace Bencode {
    /*
     * Encode bencode entity and write it to `stream`.
     */
	void WriteEntity(std::ostream&, std::shared_ptr<Entity> entity);

    /*
     * Encode bencode integer and write it to `stream`.
     */
    void WriteInteger(std::ostream&, std::shared_ptr<Integer> entity);

    /*
     * Encode bencode string and write it to `stream`.
     */
    void WriteString(std::ostream&, std::shared_ptr<String> entity);

    /*
     * Encode bencode list and write it to `stream`.
     */
    void WriteList(std::ostream&, std::shared_ptr<List> entity);

    /*
     * Encode bencode dictionary and write it to `stream`.
     */
    void WriteDict(std::ostream&, std::shared_ptr<Dict> entity);
}
