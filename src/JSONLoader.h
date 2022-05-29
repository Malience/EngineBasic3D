#pragma once

#include "edl/resource.h"

#include "json.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <cstdlib>
#include <string>
#include <unordered_map>

namespace edl {

void initJSONLoader(Toolchain& toolchain);

void loadJSON(Toolchain& toolchain, Resource& res);

typedef void (*JSONLoadFunction)(Toolchain& toolchain, Resource& res, void* obj);

class JSONSubTypeLoader {
public:
    void load(Toolchain& toolchain, Resource& res) const;

    void setLoadFunction(JSONLoadFunction function);
    void addLoadFunction(const std::string& loadtype, JSONLoadFunction function);

private:
    JSONLoadFunction loadFunction;
    std::unordered_map<std::string, JSONLoadFunction> loadTypeMap;

};

class JSONLoader {
public:
    void load(Toolchain& toolchain, Resource& res) const;

    JSONSubTypeLoader& createSubTypeLoader(const std::string& subtype);

private:
    std::unordered_map<std::string, JSONSubTypeLoader> subTypeMap;
    
};

inline int json_value_as_int(json_value_s* const value) {
    return std::atoi(json_value_as_number(value)->number);
}

inline float json_value_as_float(json_value_s* const value) {
    return std::atof(json_value_as_number(value)->number);
}

inline glm::vec3 json_value_as_vec3(json_value_s* const value) {
    json_array_element_s* ae = json_value_as_array(value)->start;
    float x = json_value_as_float(ae->value);
    ae = ae->next;
    float y = json_value_as_float(ae->value);
    ae = ae->next;
    float z = json_value_as_float(ae->value);
    return glm::vec3(x, y, z);
}

inline glm::vec4 json_value_as_vec4(json_value_s* const value) {
    json_array_element_s* ae = json_value_as_array(value)->start;
    float x = json_value_as_float(ae->value);
    ae = ae->next;
    float y = json_value_as_float(ae->value);
    ae = ae->next;
    float z = json_value_as_float(ae->value);
    ae = ae->next;
    if (ae == nullptr) return glm::vec4(x, y, z, 1.0f);
    float w = json_value_as_float(ae->value);
    return glm::vec4(x, y, z, w);
}

inline glm::quat json_value_as_euler(json_value_s* const value) {
    json_array_element_s* ae = json_value_as_array(value)->start;
    float x = json_value_as_float(ae->value);
    ae = ae->next;
    float y = json_value_as_float(ae->value);
    ae = ae->next;
    float z = json_value_as_float(ae->value);
    return glm::quat(glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z)));
}

inline std::string json_string_as_cstring(json_string_s* const value) {
    return value->string;
}

inline std::string json_value_as_cstring(json_value_s* const value) {
    return json_string_as_cstring(json_value_as_string(value));
}

}