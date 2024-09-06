#include "anariWrapper.h"
#include <anari/anari_cpp/ext/std.h>
#include <pybind11/pybind11.h>
#include <anari/anari_cpp.hpp>
#include "ctsQueries.h"

typedef union ANARIExtensions_u
{
  ANARIExtensions extension;
  int vec[sizeof(ANARIExtensions) / sizeof(int)];
} ANARIExtensions_u;

namespace cts {

void statusFunc(const void *userData,
    ANARIDevice device,
    ANARIObject source,
    ANARIDataType sourceType,
    ANARIStatusSeverity severity,
    ANARIStatusCode code,
    const char *message)
{
  auto &logger = *reinterpret_cast<const pybind11::function *>(userData);
  (void)device;
  (void)source;
  (void)sourceType;
  (void)code;
  if (severity == ANARI_SEVERITY_FATAL_ERROR) {
    logger("[FATAL] " + std::string(message));
  } else if (severity == ANARI_SEVERITY_ERROR) {
    logger("[ERROR] " + std::string(message));
  } else if (severity == ANARI_SEVERITY_WARNING) {
    logger("[WARN ] " + std::string(message));
  } else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING) {
    logger("[PERF ] " + std::string(message));
  } else if (severity == ANARI_SEVERITY_INFO) {
    logger("[INFO ] " + std::string(message));
  } else if (severity == ANARI_SEVERITY_DEBUG) {
    logger("[DEBUG] " + std::string(message));
  }
}

// returns vector of pairs consisting of the feature name and whether it is
// supported by this libraries' device or default device
std::vector<std::tuple<std::string, bool>> queryExtensions(
    const std::string &libraryName,
    const std::optional<std::string> &device,
    const std::optional<pybind11::function> &callback)
{
  // load library
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

  // get default device name if no device was set
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

  // query extension
  std::vector<std::tuple<std::string, bool>> result;

  ANARIExtensions extension;
  if (anariGetDeviceExtensionStruct(&extension, lib, deviceName.c_str())) {
    printf("WARNING: library didn't return extension list for device\n");
    return result;
  }

  ANARIExtensions_u test;
  test.extension = extension;

  // compile extension and their availability into a vector of tuples
  const size_t featureCount = sizeof(ANARIExtensions) / sizeof(int);

  const char **featureNamesPointer = query_extensions();
  std::vector<std::string> featureNames(
      featureNamesPointer, featureNamesPointer + featureCount);

  for (int i = 0; i < featureCount; ++i) {
    result.push_back({featureNames[i], test.vec[i]});
  }

  anari::unloadLibrary(lib);

  return result;
}

std::string getDefaultDeviceName(const std::string &libraryName,
    const std::optional<pybind11::function> &callback)
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

  const char **devices = anariGetDeviceSubtypes(lib);
  if (!devices) {
    return "No device present";
  }
  return *devices;
}

SceneGeneratorWrapper::SceneGeneratorWrapper(const std::string &library,
    const std::optional<std::string> &device,
    const std::optional<pybind11::function> &callback)
{
  // load library
  m_callback = callback;
  if (m_callback.has_value()) {
    m_library =
        anari::loadLibrary(library.c_str(), statusFunc, &(m_callback.value()));
  } else {
    m_library = anari::loadLibrary(library.c_str(), statusFunc, nullptr);
  }

  if (m_library == nullptr) {
    throw std::runtime_error("Library could not be loaded: " + library);
  }

  // get default device name if no device was set
  std::string deviceName;
  if (device.has_value()) {
    deviceName = device.value();
  } else {
    const char **devices = anariGetDeviceSubtypes(m_library);
    if (!devices) {
      throw std::runtime_error("No device available");
    }
    deviceName = *devices;
  }

  // setup device
  anari::Device dev = anari::newDevice(m_library, deviceName.c_str());
  if (dev == nullptr) {
    throw std::runtime_error("Device could not be created: " + deviceName);
  }

  // create a SceneGenerator
  m_sceneGenerator = std::make_unique<SceneGenerator>(dev);

  anari::release(dev, dev);
}

SceneGeneratorWrapper::SceneGeneratorWrapper(pybind11::function &callback)
{
  m_callback = callback;

  m_secondLibrary = anariLoadLibrary("sink", statusFunc, &m_callback);

  m_library = anari::loadLibrary("debug", statusFunc, &m_callback);

  m_secondDevice = anariNewDevice(m_secondLibrary, "default");

  m_device = anariNewDevice(m_library, "debug");
  anariSetParameter(
      m_device, m_device, "wrappedDevice", ANARI_DEVICE, &m_secondDevice);

  anariCommitParameters(m_device, m_device);
  anariRelease(m_secondDevice, m_secondDevice);

  // create a SceneGenerator
  // TODO do we need to release the dev device in the destructor of SceneGenerator? What about the second device?
  m_sceneGenerator = std::make_unique<SceneGenerator>(m_device);

  // TODO remove debug code
  std::cout << "End SceneGeneratorWrapper Constructor" << std::endl;
}

SceneGeneratorWrapper::~SceneGeneratorWrapper()
{
  anariRelease(m_device, m_device);
  if (m_library != nullptr) {
    anari::unloadLibrary(m_library);
  }
  // unload second lib as well if exists
  if (m_secondLibrary != nullptr) {
    anari::unloadLibrary(m_secondLibrary);
  }
}

} // namespace cts
