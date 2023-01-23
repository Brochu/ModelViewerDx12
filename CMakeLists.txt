cmake_minimum_required (VERSION 3.10)

project (ModelViewerDx12)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)

set (headers
    ${CMAKE_CURRENT_LIST_DIR}/include/Shaders.h
    ${CMAKE_CURRENT_LIST_DIR}/include/RubyTexture.h

    ${CMAKE_CURRENT_LIST_DIR}/include/Window.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Utility.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ImageIO.h

    ${CMAKE_CURRENT_LIST_DIR}/include/D3D12Sample.h
    ${CMAKE_CURRENT_LIST_DIR}/include/D3D12TexturedQuad.h
)
set (imgui_headers
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imconfig.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imgui.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imgui_impl_dx12.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imgui_impl_win32.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imgui_internal.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imstb_rectpack.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imstb_textedit.h
    ${CMAKE_CURRENT_LIST_DIR}/include/imgui/imstb_truetype.h
)
set (tracy_headers
    ${CMAKE_CURRENT_LIST_DIR}/include/tracy/Tracy.hpp
)

set (source
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/Window.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Utility.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ImageIO.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/D3D12Sample.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/D3D12TexturedQuad.cpp
)
set (imgui_source
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui_impl_dx12.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui_impl_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/imgui/imgui_widgets.cpp
)
set (tracy_source
    ${CMAKE_CURRENT_LIST_DIR}/src/tracy/TracyClient.cpp
)

set (libs
    d3d12.lib
    d3dcompiler.lib
    dxgi.lib

    assimp-vc143-mt.lib
)

# Setup Tracy integration
option(TRACY_ENABLE "" on)
option(TRACY_ON_DEMAND "" off)
add_subdirectory(libs/tracy)

add_executable (ModelViewerDx12 WIN32
    ${source} ${imgui_source} ${tracy_source}
    ${headers} ${imgui_headers} ${tracy_headers}
)

target_include_directories(ModelViewerDx12
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/include/imgui
        ${CMAKE_CURRENT_LIST_DIR}/include/tracy
)
target_link_directories(ModelViewerDx12
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/libs
)

target_link_libraries(ModelViewerDx12 ${libs} Tracy::TracyClient)

set_target_properties( ModelViewerDx12
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY       "${CMAKE_CURRENT_LIST_DIR}/"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/"
)
