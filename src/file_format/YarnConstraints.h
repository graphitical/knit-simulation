#include <string>
#include <vector>
#include <set>

namespace file_format {

// This file format stores the constrain information for yarns in JSON
// Eample: {"constraints": [{"yarnID": 0, "pinnedControlPoints": [0, 1]},
//                         {"yarnID": 3, "pinnedControlPoints": [1, 2]}]}
class YarnConstraints {
 public:
  struct ConstraintsForYarn {
    // Index of the yarn
    int yarnID;
    // A list of control point indices where the control points should be pinned
    std::set<int> pinnedPoints;
  };
  std::vector<ConstraintsForYarn> constraints;

  // Read the information from the file
  // filename: path to the file
  static YarnConstraints read(std::string filename);
};

}  // namespace file_format
