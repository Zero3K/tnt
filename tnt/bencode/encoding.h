#pragma once

#include "types.h"

namespace Bencode {
	void WriteEntity(std::ostream&, std::shared_ptr<Entity> entity);
    void WriteInteger(std::ostream&, std::shared_ptr<Integer> entity);
    void WriteString(std::ostream&, std::shared_ptr<String> entity);
    void WriteList(std::ostream&, std::shared_ptr<List> entity);
    void WriteDict(std::ostream&, std::shared_ptr<Dict> entity);
}
