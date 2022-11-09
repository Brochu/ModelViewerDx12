cmake_minimum_required (VERSION 3.10)

project (ModelViewerDx12)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)

set (headers
    ${CMAKE_CURRENT_LIST_DIR}/include/d3dx12.h

    ${CMAKE_CURRENT_LIST_DIR}/include/Window.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Utility.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Shaders.h
    ${CMAKE_CURRENT_LIST_DIR}/include/RubyTexture.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ImageIO.h

    ${CMAKE_CURRENT_LIST_DIR}/include/D3D12Sample.h
    ${CMAKE_CURRENT_LIST_DIR}/include/D3D12TexturedQuad.h
)
set (source
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/Window.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Utility.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ImageIO.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/D3D12Sample.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/D3D12TexturedQuad.cpp
)
set (libs
    d3d12.lib
    d3dcompiler.lib
    dxgi.lib
)

add_executable (ModelViewerDx12 WIN32 ${source} ${headers})

target_include_directories(ModelViewerDx12
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(ModelViewerDx12 ${libs})

set_target_properties( ModelViewerDx12
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY       "${CMAKE_CURRENT_LIST_DIR}/"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/"
)
