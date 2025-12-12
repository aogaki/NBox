cmake_minimum_required(VERSION 3.16)
project(NBox_Simulation_MT)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

find_package(ROOT REQUIRED COMPONENTS Core Hist RIO)
include(${ROOT_USE_FILE})

if(Geant4_multithreaded_FOUND)
    message(STATUS "Geant4 MT mode enabled")
else()
    message(WARNING "Geant4 was built without MT support")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(nbox_sim nbox.cc ${sources} ${headers})
target_link_libraries(nbox_sim ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

file(GLOB macros ${PROJECT_SOURCE_DIR}/*.mac)
foreach(macro ${macros})
    configure_file(${macro} ${PROJECT_BINARY_DIR} COPYONLY)
endforeach()
