
#include "encoding.h"
#include <memory>
#include <iostream>


namespace Bencoding {
	void WriteEntity(std::ostream& stream, std::shared_ptr<Entity> entity) {
		if (std::dynamic_pointer_cast<Integer>(entity) != nullptr) {
			WriteInteger(stream, std::static_pointer_cast<Integer>(entity));
		} else if (std::dynamic_pointer_cast<String>(entity) != nullptr) {
			WriteString(stream, std::static_pointer_cast<String>(entity));
		} else if (std::dynamic_pointer_cast<List>(entity) != nullptr) {
			WriteList(stream, std::static_pointer_cast<List>(entity));
		} else if (std::dynamic_pointer_cast<Dict>(entity) != nullptr) {
			WriteDict(stream, std::static_pointer_cast<Dict>(entity));
		} else throw;
	}

    void WriteInteger(std::ostream& stream, std::shared_ptr<Integer> entity) {
		stream << "i" << entity->value << "e";
    }

    void WriteString(std::ostream& stream, std::shared_ptr<String> entity) {
    	stream << entity->value.size() << ":" << entity->value;
    }

    void WriteList(std::ostream& stream, std::shared_ptr<List> entity) {
	   	stream << "l";
	    for (auto& entity : entity->value)
	        stream << entity;
	    stream << "e";
    }

    void WriteDict(std::ostream& stream, std::shared_ptr<Dict> entity) {
    	stream << "d";
	    for (auto& [key, val] : entity->value) {
	        auto keyEntity = std::make_shared<String>();
	        keyEntity->value = key;
	        stream << keyEntity << val;
	    }
	    stream << "e";
    }

    std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<Entity>& entity) {
    	WriteEntity(stream, entity);
    	return stream;
    }
}