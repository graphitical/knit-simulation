#include "./easy_profiler_stub.h"
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

#include "ui/viewer.h"
#include "ui/knitpurlExplorer.h"

int main(int argc, const char* argv[]) {
  spdlog::set_pattern("[%b %d %Y %H:%M:%S] %^[%l] %v %$ [%s:%#]");
  SPDLOG_INFO("Application started");
  EASY_PROFILER_ENABLE;

  cxxopts::Options options("knit-simulation",
    "A program that simulates yarn interactions");

  options.add_options()
    ("e,explorer", "Branch to the knitpurl explorer UI",
      cxxopts::value<bool>()->default_value("false"))
    ("r,restore", "Restore the history from the output directory",
      cxxopts::value<bool>()->default_value("false"))
    ("no-gui", "Don't show GUI",
      cxxopts::value<bool>()->default_value("false"))
    ("o,output-dir", "The folder to store and load history",
      cxxopts::value<std::string>()->default_value("output/"))
    ("c,constraint-file", "JSON file with constraint information",
      cxxopts::value<std::string>()->default_value("test/"))
    ("v,verbose", "Show all logs",
      cxxopts::value<bool>()->default_value("false"))
    ("quiet", "Don't show any logs",
      cxxopts::value<bool>()->default_value("false"))
    ("file-name", "A '.yarns' file",
      cxxopts::value<std::string>());

  options.parse_positional({ "file-name" });
  options.positional_help("<a '.yarns' file>");

  try {
    // Parse arguments
    auto option = options.parse(argc, argv);

    // Set logging level
    if (option["verbose"].as<bool>()) {
      spdlog::set_level(spdlog::level::trace);
    }
    if (option["quiet"].as<bool>()) {
      spdlog::set_level(spdlog::level::err);
    }

    if (option["explorer"].as<bool>()) {
      // Launch knitpurl explorer
      igl::opengl::glfw::Viewer viewer;
      UI::KnitpurlExplorer knitpurlExplorer;
      viewer.plugins.push_back(&knitpurlExplorer);
      viewer.launch();
    } else {
      // Launch viewer
      UI::Viewer viewer(option["output-dir"].as<std::string>(),
        option["restore"].as<bool>());
      viewer.loadYarn(option["file-name"].as<std::string>(), option["constraint-file"].as<std::string>());

      if (option["no-gui"].as<bool>()) {
        viewer.launchNoGUI();
      }
      else {
        viewer.launch();
      }
    }
  }
  catch (cxxopts::OptionException& e) {
    SPDLOG_ERROR("Error while parsing arguments: {}", e.what());
    std::cout << options.help() << std::endl;
  }

#ifdef USE_EASY_PROFILER
  SPDLOG_INFO("Dumping profiler data");
  profiler::dumpBlocksToFile("test_profile.prof");
#endif

  SPDLOG_INFO("Application finished");
  return 0;
}
