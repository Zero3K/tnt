#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>

namespace Bencoding {
	/*
	 * Base class for all `bencoding` entities.
	 */
    struct Entity {
        virtual ~Entity() = default;
    };

    struct Integer : public Entity {
        int64_t value;
    };

    struct String : public Entity {
        std::string value;
    };

    struct List : public Entity {
        std::vector<std::shared_ptr<Entity>> value;
    };

    struct Dict : public Entity {
        std::map<std::string, std::shared_ptr<Entity>> value;
    };
}