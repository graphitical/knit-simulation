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
      "Falling back to no constraints.", filename);
    return result;
  }

  typedef std::pair<size_t, size_t> pbcPair;

  // Parse file
  auto jsonContent = nlohmann::json::parse(std::istreambuf_iterator<char>(fin),
                                           std::istreambuf_iterator<char>());
  for (auto constraintForYarn : jsonContent["constraints"]) {
    ConstraintsForYarn constraint;
    constraint.yarnID = constraintForYarn["yarnID"];
    for (int pinPoint : constraintForYarn["pinnedControlPoints"]) {
      if (constraint.pinnedPoints.find(pinPoint) != constraint.pinnedPoints.end()) {
        SPDLOG_WARN("Duplicated pinned constraint for point {}", pinPoint);
      }
      constraint.pinnedPoints.insert(pinPoint);
    }
    auto pbcPairs = constraintForYarn["pbcPairs"];
    for (auto it = pbcPairs.begin(); it != pbcPairs.end();) {
      auto i = *it++;
      auto j = *it++;
      constraint.pbcPairs.push_back(pbcPair(i,j));
    }

    result.constraints.push_back(constraint);
  }
  return result;
}

}  // namespace file_format
