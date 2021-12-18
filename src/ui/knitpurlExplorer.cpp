#include <cstdlib>

#include <igl/opengl/glfw/imgui/ImGuiTraits.h>
#include <igl/opengl/glfw/imgui/ImGuiHelpers.h>

#include "knitpurlExplorer.h"
#include "texture.h"

namespace UI {

void KnitpurlExplorer::init(igl::opengl::glfw::Viewer *_viewer) {
    igl::opengl::glfw::imgui::ImGuiMenu::init(_viewer);

    _viewer->core().set_rotation_type(igl::opengl::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);
    _viewer->core().background_color = Eigen::Vector4f(0.792f, 0.792f, 0.878f, 1.000f);
    _viewer->data().line_width = 0.1f;
    _viewer->data().point_size = 0.5f;
    _viewer->core().camera_zoom = 2.f;

    if (!knitting_textures_loaded) {
        init_textures();
        for (auto texture : textures) {
            texture_names.push_back(texture.name);
        }
        igl::png::texture_from_file("knit.png", knit_tex);
        igl::png::texture_from_file("purl.png", purl_tex);
        igl::png::texture_from_file("mat.png", mat_tex);
        knitting_textures_loaded = true;
    }
}

void KnitpurlExplorer::draw_viewer_menu() {
    // float w = ImGui::GetContentRegionAvailWidth();
    // float p = ImGui::GetStyle().FramePadding.x;

    if (ImGui::CollapsingHeader("Swatch Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::DragInt("Swatch Width", &swatch_width, 4, 1, 12)) { swatch_generated = false; }
        if (ImGui::DragInt("Swatch Height", &swatch_height, 4, 1, 12)) { swatch_generated = false; }
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(
            (float)textures[current_texture].color(0),
            (float)textures[current_texture].color(1),
            (float)textures[current_texture].color(2),
            (float)textures[current_texture].color(3)));
        if (ImGui::Combo("", &current_texture, texture_names)) { swatch_generated = false; }
        ImGui::PopStyleColor();
        if (ImGui::Button("Generate Swatch", ImVec2(-1, 0))) {
            swatch = std::vector<std::vector<int>>(swatch_height, std::vector<int>(swatch_width, 0));
            int nr = textures[current_texture].pattern.rows();
            int nc = textures[current_texture].pattern.cols();
            for (int i = 0; i < swatch_height; i++) {
                for (int j = 0; j < swatch_width; j++) {
                    swatch[i][j] = (int) (textures[current_texture].pattern(i % nr, j % nc));
                }
            }
            swatch_generated = true;
            show_mat = false;
        }
        if (ImGui::Button("Save Swatch to .yarns", ImVec2(-1, 0))) {
            if (swatch_generated) {
                yarns_file = igl::file_dialog_open();
                path_prefix = yarns_file.substr(0, yarns_file.find_last_of("."));
                swatch_to_knitout();
                knitout_to_smobj();
                smobj_to_yarns();
            } else {
                std::cerr << "no swatch yet!" << std::endl;
            }
        }
        if (ImGui::Button("Show Material Properties", ImVec2(-1, 0))) {
            show_mat = true;
        }
    }
}

void KnitpurlExplorer::swatch_to_knitout() {
    std::string knitout_path = path_prefix + ".k";
    std::ofstream knitout_file(knitout_path);
    knitout_file << knitout_header << std::endl;
    lines.clear();
    inhook();
    caston(swatch_width);
    releasehook();
    for (int i = 0; i < swatch_height; i++) {
        knit_row(swatch[i], (i % 2 == 0 ? "+" : "-"));
    }
    outhook();
    for (auto line : lines) {
        knitout_file << line << std::endl;
    }
    knitout_file.close();
}

void KnitpurlExplorer::knitout_to_smobj() {
    std::string knitout_path = path_prefix + ".k";
    std::string smobj_path = path_prefix + ".smobj";
    std::string command = knitout_to_smobj_exe + " " + knitout_path + " " + smobj_path;
    std::system(command.c_str());
}

void KnitpurlExplorer::smobj_to_yarns() {
    std::string smobj_path = path_prefix + ".smobj";
    std::string command = smobj_to_yarns_exe + " " + smobj_path + " " + face_library + " " + yarns_file;
    std::system(command.c_str());
}

void KnitpurlExplorer::draw_custom_window() {
    // Define next window position + size
    if (swatch_generated) {
        ImGui::SetNextWindowPos(ImVec2(180.f * menu_scaling(), 0.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2((swatch_width + 2) * 40.f, (swatch_height + 1) * 40.f + 10.f), ImGuiCond_Always);
        ImGui::Begin("Swatch", nullptr, ImGuiWindowFlags_NoSavedSettings);
        for (int i = 0; i < swatch_height; i++) {
            ImGui::NewLine();
            for (int j = 0; j < swatch_width; j++) {
                ImGui::PushID(i * swatch_width + j);
                int curr_tex = (swatch[i][j] == 0) ? knit_tex : purl_tex;
                if (ImGui::ImageButton((void*)(intptr_t)curr_tex, ImVec2(30.f, 30.f))) {
                    swatch[i][j] = 1 - swatch[i][j];
                }
                ImGui::PopID();
                ImGui::SameLine();
            }
        }
        ImGui::End();
    }
    if (show_mat) {
        ImGui::SetNextWindowPos(ImVec2(180.f * menu_scaling() + (swatch_width + 2) * 40.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(500.f, 300.f), ImGuiCond_Always);
        ImGui::Begin("Material Properties", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ImageButton((void*)(intptr_t)mat_tex, ImVec2(480.f, 270.f));
        ImGui::End();
    }
}

void KnitpurlExplorer::clear() {
    swatch.clear();
    lines.clear();
}

void KnitpurlExplorer::inhook() {
    lines.push_back("inhook " + std::to_string(carrier));
}

void KnitpurlExplorer::tuck(std::string dir, int needle) {
    lines.push_back("tuck " + dir + " f" + std::to_string(needle) + " " + std::to_string(carrier));
}
    
void KnitpurlExplorer::caston(int n) {
    int needle = sn;
    for (int i = 0; i < n; i++) { //(int) (n / 2)
        tuck("-", needle);
        needle -= 1;
    }
    // needle = sn - n + 1;
    // for (int i = (int) (n / 2); i < n; i++) {
    //     tuck("+", needle);
    //     needle += 2;
    // }
}

void KnitpurlExplorer::releasehook() {
    lines.push_back("releasehook " + std::to_string(carrier));
}

void KnitpurlExplorer::knit(std::string dir, std::string bed, int needle) {
    std::string instr = bed == "f" ? "knit " : "purl ";
    lines.push_back(instr + dir + " f" + std::to_string(needle) + " " + std::to_string(carrier));
}

void KnitpurlExplorer::xfer(int needle) {
    lines.push_back("xfer b" + std::to_string(needle) + " f" + std::to_string(needle));
}

void KnitpurlExplorer::knit_row(std::vector<int> row, std::string dir) {
    int n = row.size();
    int needle = dir == "-" ? sn : sn - n + 1;
    for (int i = 0; i < n; i++) {
        knit(dir, (row[i] == 0 ? "f" : "b"), needle);
        // if (row[i] != 0) {
        //     xfer(needle);
        // }
        needle = dir == "+" ? needle + 1 : needle - 1;
    }
}

void KnitpurlExplorer::outhook() {
    lines.push_back("outhook " + std::to_string(carrier));
}

}
