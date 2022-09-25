cmake_minimum_required(VERSION 3.22)
project(FlowCV_AprilTag)

set(CMAKE_CXX_STANDARD 17)

find_package( FlowCV REQUIRED )

add_subdirectory(AprilTag_Detector)

