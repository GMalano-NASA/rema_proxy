#ifndef REMA_HPP
#define REMA_HPP

#include <string>
#include <filesystem>
#include <json.hpp>

#include "rema.hpp"
#include "tool.hpp"
#include "ciaa.hpp"

static inline std::filesystem::path rema_dir = std::filesystem::path("rema");
static inline std::filesystem::path rema_file = rema_dir / "rema.json";
static inline std::filesystem::path tools_dir = rema_dir / "tools";

class REMA {
public:
    static REMA& get_instance() {
        static REMA instance(rema_file);    // Guaranteed to be destroyed.
                                            // Instantiated on first use.
        return instance;
    }

    static std::vector<Tool> tools_list() {
        std::vector<Tool> res;

        for (const auto &entry : std::filesystem::directory_iterator(tools_dir)) {
            Tool t(entry.path());
            res.push_back(t);
        }
        return res;
    }

    static void delete_tool(std::string tool) {
        std::filesystem::remove(tools_dir / (tool + std::string(".json")));
    }

	void set_selected_tool(std::string tool);

	std::filesystem::path get_selected_tool() const;

    void save_to_disk() const;

	bool loaded = false;

	std::string last_selected_tool;

	CIAA rtu;

private:
    REMA(std::filesystem::path path) {
        std::ifstream i(path);
        nlohmann::json j;
        i >> j;
        this->loaded = true;
        this->last_selected_tool = j["last_selected_tool"];
    }

    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want.


public:
    REMA(REMA const&) = delete;
    REMA& operator=(REMA const&) = delete;

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status


};

#endif 		// REMA_HPP
