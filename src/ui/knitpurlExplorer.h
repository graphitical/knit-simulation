#include <Eigen/Core>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/png/texture_from_file.h>
#include <vector>
#include <string>
#include <cstdlib>

namespace UI {

static std::string knitout_header = ";!knitout-2\n"
";;Machine: SWG091N2\n"
";;Gauge: 7\n"
";;Width: 250\n"
";;Carriers: 1 2 3 4 5 6 7 8 9 10\n"
";;Position: Center";

static std::string knitout_to_smobj_exe = "../third_party/smobj/utilities/knitout-to-smobj";
static std::string smobj_to_yarns_exe = "../third_party/smobj/utilities/smobj-to-yarns";
static std::string face_library = "../third_party/smobj/faces/knitout.sf";

class KnitpurlExplorer : public igl::opengl::glfw::imgui::ImGuiMenu {
public:
    KnitpurlExplorer() {
        std::system("pwd");
        plugin_name = "KnitpurlExplorer";   
    }
    ~KnitpurlExplorer() {
        clear();
    }

    void init(igl::opengl::glfw::Viewer *_viewer);
    void draw_viewer_menu();
    void draw_custom_window();

    void inhook();
    void releasehook();
    void outhook();
    void caston(int n);
    void tuck(std::string dir, int needle);
    void knit(std::string dir, std::string bed, int needle);
    void knit_row(std::vector<int> row, std::string dir);

    void swatch_to_knitout();
    void knitout_to_smobj();
    void smobj_to_yarns();

    void clear();

private:
    int swatch_width = 4, swatch_height = 4;
    std::string yarns_file, path_prefix;
    std::vector<std::vector<int>> swatch;
    std::vector<std::string> lines;
    int carrier = 3;
    int sn = 50;
    GLuint knit_tex, purl_tex;
    std::vector<std::string> texture_names{};
    int current_texture = 0;
    bool knitting_textures_loaded = false;
};

}
