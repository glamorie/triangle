@echo off
where cl.exe /Q || (
  echo Msvc compiler not found! 
  echo Launch x64 Native Tools Command Prompt and run this script
  exit /b 1
)
pushd %~dp0%

if not exist build\ mkdir build\
pushd build
echo Building...
rc /nologo /fo assets.res ..\assets\assets.rc
cl /Fe:Triangle.exe /Fo:Triangle.obj /nologo ..\main.c assets.res
if not exist Shaders\ mkdir Shaders\
if not exist Shaders\Shader.hlsl copy ..\Shaders\Shader.hlsl Shaders\Shader.hlsl> nul
echo Build Success!
popd
popd
