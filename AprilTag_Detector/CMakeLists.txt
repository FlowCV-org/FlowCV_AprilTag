project(AprilTag_Detector)

set(AprilTag_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/AprilTagCV")
file (GLOB_RECURSE AprilTag_SRC ${AprilTag_DIR}/src/*.cpp)
INCLUDE_DIRECTORIES( ${AprilTag_DIR}/include)

add_library(
        ${PROJECT_NAME} SHARED
        apriltag_plugin.cpp
        ${AprilTag_SRC}
        ${FlowCV_SRC}
        ${IMGUI_SRC}
        ${DSPatch_SRC}
)

target_link_libraries(
        ${PROJECT_NAME}
        ${OpenCV_LIBS}
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        SUFFIX ".fp"
        )