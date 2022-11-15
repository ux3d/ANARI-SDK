#include "anariWrapper.h"
#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>
#include <pybind11/pybind11.h>
#include "ctsQueries.h"

typedef union ANARIFeatures_u
{
  ANARIFeatures features;
  int vec[sizeof(ANARIFeatures) / sizeof(int)];
} ANARIFeatures_u;

namespace cts {

    void statusFunc(const void *userData,
    ANARIDevice device,
    ANARIObject source,
    ANARIDataType sourceType,
    ANARIStatusSeverity severity,
    ANARIStatusCode code,
    const char *message)
    {
      auto& logger = *reinterpret_cast<const pybind11::function*>(userData);
      (void)device;
      (void)source;
      (void)sourceType;
      (void)code;
      if (severity == ANARI_SEVERITY_FATAL_ERROR) {
        logger("[FATAL] " + std::string(message) + "\n");
      } else if (severity == ANARI_SEVERITY_ERROR) {
        logger("[ERROR] " + std::string(message) + "\n");
      } else if (severity == ANARI_SEVERITY_WARNING) {
        logger("[WARN ] " + std::string(message) + "\n");
      } else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING) {
        logger("[PERF ] " + std::string(message) + "\n");
      } else if (severity == ANARI_SEVERITY_INFO) {
        logger("[INFO ] " + std::string(message) + "\n");
      } else if (severity == ANARI_SEVERITY_DEBUG) {
        logger("[DEBUG] " + std::string(message) + "\n");
      }
    }

    std::vector<std::tuple<std::string, bool>>
    queryFeatures(
        const std::string &libraryName,
        const std::optional<std::string> &device,
        const std::optional<pybind11::function>
                &callback)
    {
      anari::Library lib;
      if (callback.has_value()) {
        lib = anari::loadLibrary(
            libraryName.c_str(), statusFunc, &(callback.value()));
      } else {
        lib = anari::loadLibrary(libraryName.c_str(), statusFunc, nullptr);
      }
      
      if (lib == nullptr) {
        throw std::runtime_error("Library could not be loaded: " + libraryName);
      }

      std::string deviceName;
      if (device.has_value()) {
        deviceName = device.value();
      } else {
        const char **devices = anariGetDeviceSubtypes(lib);
        if (!devices) {
          throw std::runtime_error("No device available");
        }
        deviceName = *devices; 
      }



      std::vector<std::tuple<std::string, bool>> result;

      ANARIFeatures features;
      if (anariGetObjectFeatures(&features,
              lib,
              deviceName.c_str(),
              deviceName.c_str(),
              ANARI_DEVICE)) {
        printf("WARNING: library didn't return feature list\n");
        return result;
      }

      ANARIFeatures_u test;
      test.features = features;

      const size_t featureCount = sizeof(ANARIFeatures) / sizeof(int);
      
      const char** featureNamesPointer = query_extensions();
      std::vector<std::string> featureNames(
          featureNamesPointer, featureNamesPointer + featureCount);
      

      for (int i = 0; i < featureCount; ++i) {
        result.push_back({featureNames[i], test.vec[i]});
      }
      anari::unloadLibrary(lib);
      return result;

      
    }

    std::string getDefaultDeviceName(const std::string &libraryName,
        const std::optional<pybind11::function>
            &callback)
    {
      anari::Library lib;
      if (callback.has_value()) {
        lib = anari::loadLibrary(libraryName.c_str(), statusFunc, &(callback.value()));
      } else {
        lib = anari::loadLibrary(libraryName.c_str(), statusFunc, nullptr);
      }

      if (lib == nullptr) {
        throw std::runtime_error("Library could not be loaded: " + libraryName);
      }

      const char **devices = anariGetDeviceSubtypes(lib);
      if (!devices) {
        return "No device present";
      }
      return *devices; 
    }

    SceneGeneratorWrapper::SceneGeneratorWrapper(
        const std::string &library,
        const std::optional<std::string> &device,
        const std::optional<pybind11::function> &callback)
    {
        
      m_callback = callback;
      if (m_callback.has_value()) {
        m_library = anari::loadLibrary(
            library.c_str(), statusFunc, &(m_callback.value()));
      } else {
        m_library = anari::loadLibrary(library.c_str(), statusFunc, nullptr);
      }

      if (m_library == nullptr) {
        throw std::runtime_error("Library could not be loaded: " + library);
      }

      std::string deviceName;
      if (device.has_value()) {
        deviceName = device.value();
      } else {
        const char **devices =
            anariGetDeviceSubtypes(m_library);
        if (!devices) {
          throw std::runtime_error("No device available");
        }
        deviceName = *devices;
      }

      ANARIDevice dev =
          anariNewDevice(m_library, deviceName.c_str());
      if (dev == nullptr) {
        throw std::runtime_error("Device could not be created: " + deviceName);
      }
      m_sceneGenerator = std::make_unique<SceneGenerator>(dev);

    }

    SceneGeneratorWrapper::~SceneGeneratorWrapper() {
      if (m_library != nullptr) {
        anari::unloadLibrary(m_library);
      }
    }

} // namespace cts


    