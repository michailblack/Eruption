# Eruption

A modern Vulkan renderer written in C++23 for learning purposes. Eruption wraps Vulkan behind a small,
RHI-style abstraction (an `nvrhi`/Hazel-inspired layer) so that day-to-day work can focus on *rendering*
rather than boilerplate: instance/device/swapchain management, memory allocation, command submission,
shaders, pipelines, descriptors, meshes, materials and a render thread are all handled for you.

## Features

- **Vulkan 1.4** via `vulkan_raii.hpp` (C++ RAII bindings), feature/extension selection driven by
  **Vulkan Profiles** (`VP_KHR_ROADMAP_2022`).
- **Dynamic rendering** (no `VkRenderPass`/framebuffers) and **synchronization2**.
- **VMA** (Vulkan Memory Allocator) for all buffer/image allocations.
- **Slang** shaders compiled to SPIR-V **at runtime**, with hot-reload (press `R`).
- **Multithreaded**: a dedicated render thread drains a double-buffered command queue while the main
  thread runs application/game logic; frames-in-flight synchronisation with per-frame command pools.
- Resource wrappers: `Buffer`, `Texture2D` (staging upload + GPU mip generation), `Shader`,
  `GraphicsPipeline`, growable descriptor allocator, `Mesh`, `Material`, `Camera`.
- **Mesh import** via **assimp** (OBJ/FBX/glTF/…): meshes merged into one vertex/index buffer with
  submeshes, plus base-colour materials/textures (including embedded).
- Core engine: window/input/events, layer stack, logging (spdlog).

## Requirements

- A C++23 compiler (GCC 14+ or Clang 18+) and CMake ≥ 3.30.
- The **Vulkan SDK** (LunarG), with `VULKAN_SDK` set (e.g. by sourcing its `setup-env.sh`). It provides
  Vulkan-Hpp, the RAII headers, Vulkan Profiles, VMA (`include/vma/`) and Slang (`find_package(slang)`).
- System development packages: **glfw3**, **glm**, **spdlog**, **assimp**.
  - Fedora: `sudo dnf install glfw-devel glm-devel spdlog-devel assimp-devel`
- Git submodules (header-only): **EnTT** and **stb**.

## Building

```sh
git submodule update --init            # EnTT + stb
source /path/to/VulkanSDK/x.y.z/setup-env.sh   # sets VULKAN_SDK / CMAKE_PREFIX_PATH

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

./build/Binaries/Eruption-Editor
```

If `find_package(slang)` is not found automatically, add the SDK to the prefix path:
`-DCMAKE_PREFIX_PATH="$VULKAN_SDK"`.

### Helper scripts (Bash)

The scripts require Clang (`clang` and `clang++`), CMake, Ninja, and the dependencies above.
They work from any working directory. Builds are isolated in `build/clang-<configuration>`.

```sh
./Scripts/build.sh                 # Configure and build Debug
./Scripts/build.sh Release         # Debug, Release, or Dist
./Scripts/run.sh                   # Build Debug and launch the editor from the repository root
./Scripts/run.sh Release           # Additional arguments are forwarded to the editor
./Scripts/format.sh                # Format engine and editor C/C++ sources with clang-format
./Scripts/format.sh --check        # Check formatting without changing files
```

After configuration, `build.sh` links the root `compile_commands.json` to the selected build's
database for editor tooling. An existing regular file is preserved as `compile_commands.json.backup.*`.
Set `CMAKE_BUILD_PARALLEL_LEVEL` to limit build parallelism. Formatting requires `clang-format`;
builds enable `ER_ENABLE_CPPCHECK` to run cppcheck as part of compilation. If cppcheck is missing,
CMake prints a warning and continues without analysis. Use `format.sh --check` to verify formatting
separately. These scripts do not install
dependencies or initialize submodules automatically.

## Controls (Editor sample)

- `R` — recompile and hot-reload the Slang shader (`Eruption/Assets/Shaders/Mesh.slang`).

The sample orbits the camera around a lit, textured cube. To render an imported model instead, see the
commented `MeshImporter::Load(...)` call in `Editor/Source/Editor/EditorLayer.cpp`.

## Layout

```
Eruption/Source/Eruption/
  Core/                 Application, Window, events, input, layers, logging
  Platform/Vulkan/      Instance, PhysicalDevice, Device, SwapChain, VulkanContext,
                        VulkanAllocator (VMA), Buffer, Texture, Shader + ShaderCompiler (Slang),
                        Descriptors, Pipeline
  Renderer/             Renderer (+ render thread), Camera, Mesh, Material, SceneRenderer, MeshImporter
  Assets/Shaders/       Mesh.slang
Editor/                 Sample application (EditorLayer)
Vendor/                 entt, stb (git submodules)
```
