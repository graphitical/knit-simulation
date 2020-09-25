#include "./viewer.h"

#include <filesystem>
namespace stdfs = std::filesystem;

#include "spdlog/spdlog.h"

#include "file_format/yarns.h"
#include "file_format/yarnRepr.h"
#include "simulator/SimulatorParams.h"
#include "simulator/Simulator.h"
#include "simulator/DiscreteSimulator.h"
#include "simulator/Helper.h"
#include "./sweep.h"

namespace UI {

static constexpr const char* VIEWER_STATE_NAME = "viewer-state.txt";

Viewer::Viewer(std::string outputDirectory, bool reload) : _outputDirectory(outputDirectory), _reload(reload) {
  callback_pre_draw = [&](igl::opengl::glfw::Viewer&)-> bool {
    //_refreshLock.lock();
    if (_needRefresh) {
      refresh();
    }
    return false;
  };

  callback_post_draw = [&](igl::opengl::glfw::Viewer&)-> bool {
    //_refreshLock.unlock();
    return true;
  };

  //loadYarn(filename);
  if (_outputDirectory.back() != '/') {
    _outputDirectory.push_back('/');
  }
}

int Viewer::launch(bool resizable, bool fullscreen, const std::string& name, int width, int height) {
  assert(_loaded);
  // Add menu
  _menu.reset(new Menu());
  this->plugins.push_back(_menu.get());

  // Start trackball mode (allow panning)
  this->core().set_rotation_type(igl::opengl::ViewerCore::ROTATION_TYPE_TWO_AXIS_VALUATOR_FIXED_UP);

  // Disable wireframe display
  this->data().show_lines = 0;

  // Set background color
  this->core().background_color.data()[0] = 0;
  this->core().background_color.data()[1] = 0;
  this->core().background_color.data()[2] = 0;

  // Load yarns
  refresh();

  // Launch
  return igl::opengl::glfw::Viewer::launch(resizable, fullscreen, name, width, height);
}

void Viewer::launchNoGUI() {
  assert(_loaded);
  this->animationManager()->startSimulation();
  this->animationManager()->join();   // wait indefinitely
}

void Viewer::refresh() {
  assert(std::this_thread::get_id() == _threadId
    && "refresh() must be called from the thread the viewer has been constructed");
  std::lock_guard<std::recursive_mutex> lock(_mutex);
  // std::lock_guard<std::recursive_mutex> guard(_refreshLock);

  // Get yarn shape
  const file_format::YarnRepr yarns = _history->getFrame(_currentFrame);
  const simulator::SimulatorParams& params = _simulator->getParams();

  // Create Layers
  while (this->data_list.size() <= ViewerLayerID::YARNS + yarns.yarns.size()) {
    this->data_list.push_back(igl::opengl::ViewerData());
  }

  // Draw ground
  if (_simulator.get()->getParams().enableGround) {
    this->selected_data_index = ViewerLayerID::GROUND;

    Eigen::MatrixXd groundPoints(4, 3);
    groundPoints << 10, params.groundHeight, 10,
      10, params.groundHeight, -10,
      -10, params.groundHeight, -10,
      -10, params.groundHeight, 10;

    Eigen::MatrixXi groundTrianges(2, 3);
    groundTrianges << 2, 0, 1,
      3, 0, 2;

    this->data().clear();
    this->data().set_mesh(groundPoints.cast<double>(), groundTrianges);
  }

  // Draw frames
  this->selected_data_index = ViewerLayerID::MATERIAL_FRAMES;
  this->data().clear();
  if (showMaterialFrames || showBishopFrame) {

    Eigen::MatrixX3d V;
    Eigen::MatrixX2i E;
    Eigen::MatrixX3f C;
    visualizeMaterialAndBishopFrames(yarns, &V, &E, &C);

    this->data().set_edges(V, E, C.cast<double>());
    this->data().show_lines = true;
    this->data().line_width = 5;
  }

  for (size_t i = 0; i < yarns.yarns.size(); i++) {
    // Clear old mesh
    this->selected_data_index = i + ViewerLayerID::YARNS;
    this->data().clear();

    // Get curve
    auto& yarn = yarns.yarns[i];
    auto yarnPoints = yarns.getYarnPoints(i);

    // Use Catmull-Rom to smooth the curve
    Eigen::MatrixXd points = simulator::catmullRomSequenceSample(yarnPoints, curveSamples);

    // Create mesh for tube
    Eigen::MatrixXd vertices;
    Eigen::MatrixXi triangles;
    circleSweep(points, yarn.radius, circleSamples, &vertices, &triangles);
    this->data().set_mesh(vertices, triangles);

    // Set color
    this->data().set_colors(yarn.color);

    // Draw center line
    if (yarnPoints.rows() >= 2) {
      Eigen::MatrixXi E(yarnPoints.rows() - 1, 2);
      for (int i = 0; i < yarnPoints.rows() - 1; i++) {
        E(i, 0) = i;
        E(i, 1) = i + 1;
      }
      this->data().set_edges(yarnPoints,
        E, Eigen::RowVector3d(1, 1, 1));
    }

    // Set vertex labels
    std::vector<std::string> labels;
    for (int i = 0; i < yarnPoints.rows(); i++) {
      labels.push_back(std::to_string(i));
    }
    this->data().set_labels(yarnPoints, labels);
    this->data().label_color = Eigen::Vector4f(1, 1, 1, 1);
  }

  _needRefresh = false;
}

void Viewer::createSimulator() {
  delete (_animationManager.release());
  delete (_history.release());
  delete (_simulator.release());

  // Update simulator
  switch (simulatorType)
  {
  case simulator::SimulatorType::Continuous:
    SPDLOG_INFO("Using continuous simulator");
    _simulator.reset(new simulator::Simulator(_yarnsRepr, params));
    break;

  case simulator::SimulatorType::Discrete:
    SPDLOG_INFO("Using discrete simulator");
    _simulator.reset(new simulator::DiscreteSimulator(_yarnsRepr, params));
    break;

  default:
    assert(false && "Invalid simulator class");
    break;
  }

  // Reset manager
  _animationManager.reset(new AnimationManager(this));
  _history.reset(new HistoryManager(this, _simulator.get()->getYarns()));

  _currentFrame = 0;

  this->refresh();
}

void Viewer::loadYarn(const std::string& filename) {
  // Load .yarns file
  SPDLOG_INFO("Loading model: {}", filename);
  file_format::Yarns::Yarns yarns;
  try {
    yarns = file_format::Yarns::Yarns::load(filename);
  }
  catch (const std::runtime_error& e) {
    SPDLOG_ERROR("Runtime error while loading \"{}\": {}", filename, e.what());
  }

  _yarnsRepr = file_format::YarnRepr(yarns);

  if (_reload) {
    // Restoring State
    SPDLOG_INFO("Try restoring state and history from \"{}\"", _outputDirectory);
    stdfs::create_directory(_outputDirectory);

    file_format::ViewerState state(_outputDirectory + VIEWER_STATE_NAME);
    simulatorType = state.getType();
    params = state.getParams();
    int numSteps = state.getNumSteps();

    createSimulator();

    char positionName[200];
    char velocityName[200];
    for (int i = 2; i <= numSteps; i++) {
      snprintf(positionName, 200, "%sposition-%05d.yarns", _outputDirectory.c_str(), i);
      snprintf(velocityName, 200, "%svelocity-%05d.yarns", _outputDirectory.c_str(), i);
      if (!stdfs::exists(positionName)
        || !stdfs::exists(velocityName)) {
        SPDLOG_WARN("{} frames out of {} restored.", i - 1, numSteps);
        numSteps = i - 1;
        break;
      }

      file_format::Yarns::Yarns position = file_format::Yarns::Yarns::load(positionName);
      _history->addFrame(file_format::YarnRepr(position));
    }

    // Load last position and velocity
    if (numSteps > 1) {
      snprintf(velocityName, 200, "%svelocity-%05d.yarns", _outputDirectory.c_str(), numSteps);
      file_format::Yarns::Yarns velocity = file_format::Yarns::Yarns::load(velocityName);
      _simulator->setVelocity(file_format::YarnRepr(velocity));
      _simulator->setPosition(_history->getFrame(_history->totalFrameNumber() - 1));
    }

    SPDLOG_INFO("> Frame 1 to {} restored.", numSteps);
  }
  else {
    createSimulator();
  }

  _loaded = true;
}

void Viewer::saveYarn(const std::string& filename) {
  _history->getFrame(_currentFrame).toYarns().save(filename);
}

void Viewer::nextFrame() {
  std::lock_guard<std::recursive_mutex> lock(_mutex);
  if (_currentFrame + 1 < _history->totalFrameNumber()) {
    _currentFrame++;
    invalidate();
  }
}

void Viewer::prevFrame() {
  std::lock_guard<std::recursive_mutex> lock(_mutex);
  if (_currentFrame > 0) {
    _currentFrame--;
    invalidate();
  }
}

void Viewer::setAnimationMode(bool animating) {
  std::lock_guard<std::recursive_mutex> lock(_mutex);
  core().is_animating = animating;
}


void Viewer::saveState() const {
  file_format::ViewerState state(simulatorType,
    _simulator->getParams(),
    _history->totalFrameNumber());

  state.save(_outputDirectory + VIEWER_STATE_NAME);
}

void Viewer::visualizeMaterialAndBishopFrames(const file_format::YarnRepr& yarnRepr,
  Eigen::MatrixX3d* V, Eigen::MatrixX2i* E, Eigen::MatrixX3f* C) {
  std::vector<Eigen::RowVector3d> v;
  std::vector<Eigen::RowVector2i> e;
  std::vector<Eigen::Vector3f> c;

  // Insert vertices on the yarn
  for (Eigen::Index i = 0; i < yarnRepr.vertices.rows(); i++) {
    v.push_back(yarnRepr.vertices.row(i));
  }

  for (const auto& yarn : yarnRepr.yarns) {
    size_t i;
    // Show a rectangle in the direction of the axis
    auto addAxis = [&](Eigen::RowVector3d direction, Eigen::Vector3f& color) {
      // Insert vertices
      size_t vid1 = v.size();
      v.push_back(yarnRepr.vertices.row(i) + yarn.radius * 2 * direction);
      size_t vid2 = v.size();
      v.push_back(yarnRepr.vertices.row(i + 1) + yarn.radius * 2 * direction);

      // Insert lines
      e.push_back({ vid1, vid2 });
      e.push_back({ vid1, i });
      e.push_back({ vid2, i + 1 });

      // Insert colors
      c.push_back(color);
      c.push_back(color);
      c.push_back(color);
    };
    for (i = yarn.begin; i < yarn.end - 1; i++) {
      // Show the frames
      if (showBishopFrame) {
        addAxis(yarnRepr.bishopFrameU.row(i), bishopFrameUColor);
        addAxis(yarnRepr.bishopFrameV.row(i), bishopFrameVColor);
      }
      if (showMaterialFrames) {
        addAxis(yarnRepr.materialFrameU.row(i), materialFrameUColor);
        addAxis(yarnRepr.materialFrameV.row(i), materialFrameVColor);
      }
    }
  }

  // Convert to matrix
  V->resize(v.size(), 3);
  for (size_t i = 0; i < v.size(); i++)
    V->row(i) = v[i];

  E->resize(e.size(), 2);
  for (size_t i = 0; i < e.size(); i++)
    E->row(i) = e[i];

  C->resize(c.size(), 3);
  for (size_t i = 0; i < c.size(); i++)
    C->row(i) = c[i].transpose();
}

} // namespace UI
