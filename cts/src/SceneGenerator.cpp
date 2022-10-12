

// Copyright 2021 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#include "SceneGenerator.h"
#include "anariWrapper.h"


namespace cts {

SceneGenerator::SceneGenerator(const std::string& library, const std::string& device, const std::function<void(const std::string message)>& callback) : 
    m_library(anari::loadLibrary(library.c_str(), statusFunc, &callback)),
      m_device(anari::newDevice(m_library, device.c_str())),
        TestScene(m_device)

{
  //anari::commitParameters(m_device, m_device);
  m_world = anari::newObject<anari::World>(m_device);
}

SceneGenerator::~SceneGenerator()
{
  anari::release(m_device, m_world);
  anari::unloadLibrary(m_library);
}

std::vector<anari::scenes::ParameterInfo> SceneGenerator::parameters()
{
  return {
      {"geometrySubtype", ANARI_STRING, "triangle", "Which type of geometry to generate"},
      {"primitveMode", ANARI_STRING, "soup", "How the data is arranged (soup or indexed)"},
      {"primitiveCount", ANARI_UINT32, 1, "How many primtives should be generated"},
      {"image_height",
          ANARI_UINT32,
          1024,
          "Height of the image"},
      {"image_width",
          ANARI_UINT32,
          1024,
          "Width of the image"},
      //
  };
}

anari::World SceneGenerator::world()
{
  return m_world;
}

void SceneGenerator::commit()
{
  auto d = m_device;

  std::string geometrySubtype = getParam<std::string>("geometrySubtype", "triangle");
  std::string primitveMode = getParam<std::string>("primitveMode", "soup");
  size_t primitiveCount = getParam<size_t>("primitiveCount", 1);

  // Build this scene top-down to stress commit ordering guarantees

  setDefaultLight(m_world);

  auto surface = anari::newObject<anari::Surface>(d);
  auto geom = anari::newObject<anari::Geometry>(d, geometrySubtype.c_str());
  auto mat = anari::newObject<anari::Material>(d, "matte");
  anari::setParameter(d, mat, "color", "color");
  anari::commitParameters(d, mat);

  anari::setAndReleaseParameter(
      d, m_world, "surface", anari::newArray1D(d, &surface));

  anari::commitParameters(d, m_world);

  anari::setParameter(d, surface, "geometry", geom);
  anari::setParameter(d, surface, "material", mat);


  std::vector<glm::vec3> verticies;
  if (geometrySubtype == "triangle") {
    verticies = generateTriangles(primitveMode, primitiveCount);
  }

  

  anari::setAndReleaseParameter(d,
      geom,
      "vertex.position",
      anari::newArray1D(d, verticies.data(), verticies.size()));


  anari::commitParameters(d, geom);
  anari::commitParameters(d, mat);
  anari::commitParameters(d, surface);

  // cleanup

  anari::release(d, surface);
  anari::release(d, geom);
  anari::release(d, mat);
}

std::vector<glm::vec3> SceneGenerator::generateTriangles(
    const std::string& primitiveMode, size_t primitiveCount)
{
    std::vector<glm::vec3> verticies{{0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 0.0, 0.0}};

    return verticies;
}

std::vector<uint8_t> SceneGenerator::renderScene(const std::string &rendererType)
{
  size_t image_height = getParam<size_t>("image_height", 1024);
  size_t image_width = getParam<size_t>("image_width", 1024);
  auto camera = anari::newObject<anari::Camera>(m_device, "perspective");
  anari::setParameter(
      m_device, camera, "aspect", (float)image_height / (float)image_width);

  auto renderer =
      anari::newObject<anari::Renderer>(m_device, rendererType.c_str());
  //anari::setParameter(d, renderer, "pixelSamples", g_numPixelSamples);
  //anari::setParameter(
  //    m_device, renderer, "backgroundColor", glm::vec4(glm::vec3(0.1f), 1));
  anari::commitParameters(m_device, renderer);

  auto frame = anari::newObject<anari::Frame>(m_device);
  anari::setParameter(m_device, frame, "size", image_height * image_width);
  anari::setParameter(m_device, frame, "color", ANARI_UFIXED8_RGBA_SRGB);

  anari::setParameter(m_device, frame, "renderer", renderer);
  anari::setParameter(m_device, frame, "camera", camera);
  anari::setParameter(m_device, frame, "world", m_world);

  anari::commitParameters(m_device, frame);

    auto cam = createDefaultCameraFromWorld(m_world);
  anari::setParameter(m_device, camera, "position", cam.position);
anari::setParameter(m_device, camera, "direction", cam.direction);
anari::setParameter(m_device, camera, "up", cam.up);
anari::commitParameters(m_device, camera);


anari::render(m_device, frame);
anari::wait(m_device, frame);

auto fb = anari::map<uint8_t>(m_device, frame, "color");

std::vector<uint8_t> result(fb.data, fb.data + image_height * image_width);

anari::unmap(m_device, frame, "color");

  anari::release(m_device, camera);
anari::release(m_device, frame);
  anari::release(m_device, renderer);

  return result;
}

} // namespace cts