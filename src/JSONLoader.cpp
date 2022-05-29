#include "JSONLoader.h"

#include "edl/io.h"

#include "json.h"

#include <assert.h>
#include <iostream>

namespace edl {

void initJSONLoader(Toolchain& toolchain) {
    toolchain.add("JSONLoader", new JSONLoader());
}

void loadJSON(Toolchain& toolchain, Resource& res) {
    if (!toolchain.has("JSONLoader")) {
        assert("JSONLoader not found on toolchain!");
        return;
    }

    JSONLoader& loader = toolchain.getTool<JSONLoader>("JSONLoader");
    loader.load(toolchain, res);
}

void JSONSubTypeLoader::load(Toolchain& toolchain, Resource& res) const {
    std::string str;
    str.resize(filesize(res.path));
    loadFile(res.path, str);

    //TODO: Memory allocation
    json_parse_result_s result;
    json_value_s* root = json_parse_ex(str.c_str(), str.size(), json_parse_flags_default, NULL, nullptr, &result);
    
    //TODO: More error checking
    if (result.error != json_parse_error_none) {
        std::cout << "JSON parsing error, line: " << result.error_line_no << ", offset: " << result.error_offset << std::endl;
        return;
    }
    
    json_object_s* obj = json_value_as_object(root);

    if(loadFunction != nullptr) loadFunction(toolchain, res, obj);

    size_t size = obj->length;
    json_object_element_s* e = obj->start;
    for (size_t i = 0; i < size; i++) {
        if (loadTypeMap.find(e->name->string) != loadTypeMap.end()) {
            loadTypeMap.at(e->name->string)(toolchain, res, e->value);
        }

        e = e->next;
    }

    //TODO: Memory management
    free(root);
}

void JSONSubTypeLoader::setLoadFunction(JSONLoadFunction function) {
    loadFunction = function;
}

void JSONSubTypeLoader::addLoadFunction(const std::string& loadtype, JSONLoadFunction function) {
    loadTypeMap.insert({ loadtype, function });
}

void JSONLoader::load(Toolchain& toolchain, Resource& res) const {
    if (subTypeMap.find(res.subtype) == subTypeMap.end()) {
        assert("JSON loader not found for type: %s", res.subtype);
        return;
    }

    const JSONSubTypeLoader& loader = subTypeMap.at(res.subtype);
    loader.load(toolchain, res);
}

JSONSubTypeLoader& JSONLoader::createSubTypeLoader(const std::string& subtype) {
    return subTypeMap[subtype];
}

}