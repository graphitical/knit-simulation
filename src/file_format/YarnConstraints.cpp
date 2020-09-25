#include "./YarnConstraints.h"

#include <fstream>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace file_format {

YarnConstraints YarnConstraints::read(std::string filename) {
  YarnConstraints result;

  // Open file
  if (filename.empty()) {
    SPDLOG_WARN("The file path for constraints file is empty");
    return result;
  }

  std::ifstream fin(filename);
  if (!fin) {
    SPDLOG_WARN("Failed to open constraint file \"{}\". "
      "Falling back to no constraints.");
    return result;
  }

  // Parse file
  auto jsonContent = nlohmann::json::parse(std::istreambuf_iterator<char>(fin),
                                           std::istreambuf_iterator<char>());
  for (auto constrainForYarn : jsonContent["constraints"]) {
    ConstraintsForYarn constraint;
    constraint.yarnID = constrainForYarn["yarnID"];
    for (int pinPoint : constrainForYarn["pinnedControlPoints"]) {
      if (constraint.pinnedPoints.find(pinPoint) != constraint.pinnedPoints.end()) {
        SPDLOG_WARN("Duplicated pinned constraint for point {}", pinPoint);
      }
      constraint.pinnedPoints.insert(pinPoint);
    }

    result.constraints.push_back(constraint);
  }
  return result;
}

}  // namespace file_format
