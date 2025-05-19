# Configuration based on raylib example CMake starter project
# Compiling for web untested and unsupported
#
cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(rainshader)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
set(resources_dir)
set(shaders_dir)
file(GLOB resources resources/*)
list(APPEND resources_dir ${resources})
file(GLOB shaders shaders/*)
list(APPEND shaders_dir ${shaders})

# Dependencies
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()


set(RAYGUI_VERSION 4.0)

# raygui
find_package(raygui ${RAYGUI_VERSION} QUIET)
if (NOT raygui_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raygui
        GIT_REPOSITORY https://github.com/raysan5/raygui.git
        GIT_TAG ${RAYGUI_VERSION}
    )
    FetchContent_GetProperties(raygui)
    if (NOT raygui_POPULATED)
        FetchContent_MakeAvailable(raygui)
    endif()
endif()



add_executable(${PROJECT_NAME} rain_simulator.c )

#add_executable(${PROJECT_NAME} shaders_basic_pbr.c)
#set(raylib_VERBOSE 1)
if(NOT MSVC)
    target_link_libraries(${PROJECT_NAME} raylib m)
else ()
    target_link_libraries(${PROJECT_NAME} raylib)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${raygui_SOURCE_DIR}/src 
    ${raygui_SOURCE_DIR}/styles/dark)


# Web Configurations
if (${PLATFORM} STREQUAL "Web")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html") # Tell Emscripten to build an example.html file.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

# Copy all of the resource files to the destination
file(COPY ${resources_dir} DESTINATION "resources/")
file(COPY ${shaders_dir} DESTINATION "shaders/")
