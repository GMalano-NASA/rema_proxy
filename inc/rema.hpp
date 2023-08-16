#ifndef REMA_HPP
#define REMA_HPP

#include <Open3D/Geometry/PointCloud.h>
#include <Open3D/Registration/ColoredICP.h>
#include <Open3D/IO/ClassIO/ImageIO.h>
#include <Eigen/Eigen>

#include <string>
#include <mutex>
#include <filesystem>
#include <json.hpp>

#include "rema.hpp"
#include "tool.hpp"
#include "ciaa.hpp"

static inline std::filesystem::path rema_dir = std::filesystem::path("rema");
static inline std::filesystem::path rema_file = rema_dir / "rema.json";
static inline std::filesystem::path tools_dir = rema_dir / "tools";

struct Point3D {
    double x, y, z;
};

struct temps {
    double x;
    double y;
    double z;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(temps, x, y, z)

struct telemetry {
    double x;
    double y;
    double z;
    bool probe_touching = false;
    bool x_y_on_condition = false;
    bool z_on_condition = false;
    bool x_stalled = false;
    bool y_stalled = false;
    bool z_stalled = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(telemetry, x, y, z, probe_touching, x_y_on_condition, z_on_condition, x_stalled, y_stalled, z_stalled)

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

    void update_telemetry(boost::asio::streambuf &rx_buffer);

    std::vector<Eigen::Matrix<double, 3, 1>> get_aligned_tubes(std::vector<Point3D> src_points, std::vector<Point3D> dst_points);

	void set_selected_tool(std::string tool);

	std::filesystem::path get_selected_tool() const;

    void save_to_disk() const;

	bool loaded = false;

	std::string last_selected_tool;

	CIAA rtu;

    bool cancel_cmd;

    // Telemetry values
    struct telemetry telemetry;
    struct temps temps;


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
