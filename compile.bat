@echo off

set "buildfolder=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin"
set "execpath=%buildfolder%\MSBuild.exe"

echo %execpath%
"%execpath%" ./build/ModelViewerDx12.sln
