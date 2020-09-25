#pragma once

#include <igl/opengl/glfw/imgui/ImGuiMenu.h>

namespace UI {

// Display a file picker and store the result in `fileName`
void filePicker(const char *name, std::string *fileName) {
  ImGui::Text("%s", name);
  ImGui::SameLine();
  const char *buttonText;
  if (fileName->empty()) {
    buttonText = "<empty>";
  } else {
    buttonText = fileName->c_str();
  }
  if (ImGui::Button(buttonText)) {
    std::string newFileName = igl::file_dialog_open();
    if (!newFileName.empty()) {
      *fileName = newFileName;
    }
  }
}

}  // namespace UI
