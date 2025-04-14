#include <string>
#include <memory>
#include <iostream>

#include "decoding.h"


namespace Bencoding {
    std::shared_ptr<Integer> ReadInteger(std::istream& stream) {
        if (stream.peek() != 'i')
            throw;

        stream.get();  // pop 'i'
        int64_t result = 0;
        while (stream.peek() != 'e')
            result = result * 10 + (stream.get() - '0');
        stream.get();  // pop 'e'

        auto ptr = std::make_shared<Integer>();
        ptr->value = result;

        return ptr;
    }

    std::shared_ptr<String> ReadString(std::istream& stream) {
        if (!isdigit(stream.peek()))
            throw;
        
        int64_t size = 0;
        while (stream.peek() != ':')
            size = size * 10 + (stream.get() - '0');
        stream.get();  // pop ':'

        auto ptr = std::make_shared<String>();
        ptr->value.resize(size);
        stream.read(&(ptr->value[0]), size);

        return ptr;
    }

    std::shared_ptr<List> ReadList(std::istream& stream) {
        if (stream.peek() != 'l')
            throw;
        stream.get();  // pop l
        
        auto ptr = std::make_shared<List>();
        while (stream.peek() != 'e') {
            auto entity = ReadEntity(stream);
            ptr->value.push_back(entity);
        }

        stream.get();  // pop e

        return ptr;
    }

    std::shared_ptr<Dict> ReadDict(std::istream& stream) {
        if (stream.peek() != 'd')
            throw;
        stream.get();  // pop d
        
        auto ptr = std::make_shared<Dict>();
        while (stream.peek() != 'e') {
            auto key = ReadEntity(stream);
            if (std::dynamic_pointer_cast<String>(key) == nullptr)
                throw;
            
            auto value = ReadEntity(stream);
            ptr->value[std::static_pointer_cast<String>(key)->value] = value;
        }

        stream.get();  // pop e

        return ptr;
    }

    std::shared_ptr<Entity> ReadEntity(std::istream& stream) {
        if (isdigit(stream.peek())) {
            return ReadString(stream);
        } else if (stream.peek() == 'i') {
            return ReadInteger(stream);
        } else if (stream.peek() == 'l') {
            return ReadList(stream);
        } else if (stream.peek() == 'd') {
            return ReadDict(stream);
        } else {
            throw;
        }
    }

    std::istream& operator>>(std::istream& stream, std::shared_ptr<Entity>& entity) {
        entity = ReadEntity(stream);
        return stream;
    }
}


