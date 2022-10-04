include(FetchContent)

#set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.10.0
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  )

FetchContent_GetProperties(pybind11)

if(NOT pybind11_POPULATED)
    FetchContent_Populate(pybind11)
    add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
endif()

SET(BUILD_TESTS  OFF)
SET(DOWNLOAD_GTEST  OFF)
