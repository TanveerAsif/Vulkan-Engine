# Vulkan Engine Build Setup

## Prerequisites

This project requires:
- **Vulkan SDK** (includes shaderc for shader compilation)
- **Bazel** or Bazelisk (build system)
- **GCC/G++** (C++ compiler)

## Installation

### Install Vulkan SDK

#### Ubuntu/Debian:
```bash
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.290-jammy.list \
  https://packages.lunarg.com/vulkan/1.3.290/lunarg-vulkan-1.3.290-jammy.list
sudo apt-get update
sudo apt-get install -y vulkan-sdk
```

#### Or download from:
https://vulkan.lunarg.com/sdk/home

### Install Bazelisk

```bash
npm install -g @bazel/bazelisk
# or
sudo wget -O /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64
sudo chmod +x /usr/local/bin/bazel
```

## Building

### Option 1: Use setup script (Recommended)

```bash
source setup_env.sh
bazelisk build //...
```

### Option 2: Manual environment setup

```bash
export VULKAN_SDK=/path/to/vulkan/sdk
export CPLUS_INCLUDE_PATH=$VULKAN_SDK/include:$CPLUS_INCLUDE_PATH
export LIBRARY_PATH=$VULKAN_SDK/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH

bazelisk build //...
```

### Build Modes

```bash
# Debug build
bazelisk build --compilation_mode=dbg //...

# Optimized build
bazelisk build --compilation_mode=opt //...

# Build specific target
bazelisk build //VulkanCore:VulkanCore
bazelisk build //VulkanDemo:VulkanDemo
```

## Running

```bash
bazel-bin/VulkanDemo/VulkanDemo
```

## CI/CD

The project uses GitHub Actions for CI. The workflow automatically:
1. Installs the Vulkan SDK
2. Sets up environment variables
3. Builds all targets
4. Runs tests

No hardcoded paths are used - everything is resolved dynamically based on the installed Vulkan SDK location.

## Troubleshooting

### "shaderc/shaderc.hpp: No such file or directory"

Make sure you've sourced the environment setup:
```bash
source setup_env.sh
```

Or verify your Vulkan SDK is installed and `VULKAN_SDK` environment variable is set.

### Linker errors with shaderc

Ensure `LIBRARY_PATH` and `LD_LIBRARY_PATH` include the Vulkan SDK lib directory:
```bash
echo $LIBRARY_PATH
echo $LD_LIBRARY_PATH
```
