#ifndef TOOL_HPP
#define TOOL_HPP

#include <string>
#include <json.hpp>

#include "points.hpp"

class Tool {
public:
    Tool() {
    }

    Tool(std::string tool_name, Point3D offset_) :
            name(tool_name), offset(offset_) {        
    }

    Tool(const std::filesystem::path &tool_file);

    void save_to_disk() const;

    std::string name;
    Point3D offset;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Tool, name, offset)

#endif 		// TOOL_HPP
