# ModelViewerDx12
Personal project to make sure I fully understand how to setup a good C++ development environment on Windows for graphics programming. The goal is to have a more stable env this time.

I would like to re-implement the Monkey Ball map viewer so I have some good set of starting features. I would also like to use Dx12 this time in order to practice using a more recent graphics API.
Since Dx12 is a very lightweight API, I will start off of a hello world project found online.

The toolset I would like to use to compile and link this project as well as fine tune performance

* (X) Cmake
* (X) Clang/Clangd or MSBuild
* ( ) [Orbit](https://github.com/google/orbit) (Performance profiling from Google)

I would also like to link to the following libraries to help with development

* (X) DirectX 12 (Main graphics API)
* ( ) ImGUI (Immediate mode UI)
* ( ) Assimp (Help to load 3d model files)
* (X) Tracy (Performance profiling)

I would like to use the [Dx12 hello world project](https://gpuopen.com/learn/hellod3d12-directx-12-sdk-sample/) from GPUOpen to start off. I had to adapt this project a little bit as by default the behaviour of this application is to run for a given amount on frames before closing automatically.

My first goal is to replicate the model viewer application I made with DirectX 11 before.

* Loading models
* Create VertexBuffers(views) as well as IndexBuffers(views)
* Load textures based on the mtllib file linked to the model (.obj)
* Upload texture data to GPU memory
* Render models with one light source in the scene
* Debug controls to move camera, view parameters and light position

Some ideas for new features to add on afterwards

* Try and organize resources in one big heap allocation (w/ CreatePlacedResource calls)
* Idem with DescriptorHeaps
* Be able to change the model .obj file at runtime
    * Clear out the CPU and GPU memory for the old model.
    * Load the new data for the new model to render.
    * Re-upload all needed data to GPU.
    * Start rendering again.
* Try to build TopLevel/BottomLevel acceleration structures to test the DXR api (HWRT)
* Implement some simple RT shadows with MissShaders
