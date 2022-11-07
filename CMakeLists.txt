cmake_minimum_required (VERSION 3.10)

project (ModelViewerDx12)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)

set (headers
    #TODO: Add header files here later
)
set (source
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
)

#TODO: Find and link the libraries needed for the project
# Probably good to start linking with Dx12 in order to make sure the HelloWorld works

add_executable (ModelViewerDx12 ${source} ${headers})
