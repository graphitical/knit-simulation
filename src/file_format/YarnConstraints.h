#include <string>
#include <vector>
#include <set>

namespace file_format {

// This file format stores the constrain information for yarns in JSON
// Eample: {"constraints": [{"yarnID": 0, "pinnedControlPoints": [0, 1]},
//                         {"yarnID": 3, "pinnedControlPoints": [1, 2], "pbcXPairs": [0, 5, 15, 20]}]}
// The pinnedControlPoints specify a single point
// The pbc(X/Y)Pairs operate in twos, where (0, 5) is a pair and (15, 20) is another pair
// X/Y represents the axis that we  control
// Additionally, the first pair is used as the reference pair. 
// We pin the first element (0) and control the second (5) for PBC simulations
class YarnConstraints {
 public:
  struct ConstraintsForYarn {
    // Index of the yarn
    int yarnID;
    // A list of control point indices where the control points should be pinned
    std::set<int> pinnedPoints;

    // PBCs
    std::vector< std::pair<size_t, size_t> > pbcXPairs;
    std::vector< std::pair<size_t, size_t> > pbcYPairs;
  };
  std::vector<ConstraintsForYarn> constraints;

  // Read the information from the file
  // filename: path to the file
  static YarnConstraints read(std::string filename);
};

}  // namespace file_format
