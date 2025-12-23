# Vulkan Engine

A Vulkan-based graphics engine built with C++ and Bazel.

## Prerequisites

- **Ubuntu 20.04+** (or compatible Linux distribution)
- **GCC 11+** with C++20 support
- **Bazel/Bazelisk** (build system)
- **Vulkan SDK** (graphics API)
- **GLFW3** (window management)

## Setup

### Install Dependencies

```bash
# Install build tools
sudo apt-get update
sudo apt-get install -y build-essential

# Install Vulkan SDK and shaderc
wget -qO /tmp/lunarg-signing-key-pub.asc https://packages.lunarg.com/lunarg-signing-key-pub.asc
sudo mkdir -p /etc/apt/keyrings
sudo cat /tmp/lunarg-signing-key-pub.asc | sudo gpg --dearmor -o /etc/apt/keyrings/lunarg-archive-keyring.gpg

# Add LunarG repository (for Ubuntu 22.04 jammy)
echo "deb [signed-by=/etc/apt/keyrings/lunarg-archive-keyring.gpg] https://packages.lunarg.com/vulkan jammy main" | sudo tee /etc/apt/sources.list.d/lunarg-vulkan.list

sudo apt-get update
sudo apt-get install -y vulkan-sdk libglfw3-dev

# Install Bazelisk
curl -L https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 -o bazel
chmod +x bazel
sudo mv bazel /usr/local/bin/bazel
```

### Configure Environment (Optional)

Create a `.bazelrc.user` file for local configuration:

```bash
# Example: Custom library paths
cat > .bazelrc.user << EOF
build --linkopt=-L/usr/lib/x86_64-linux-gnu
build --linkopt=-Wl,-rpath,/usr/lib/x86_64-linux-gnu
EOF
```

## Building

Build the entire project:

```bash
bazel build --compilation_mode=dbg //...
```

Build specific targets:

```bash
# Build only VulkanCore library
bazel build //VulkanCore:VulkanCore

# Build VulkanDemo application
bazel build //VulkanDemo:VulkanDemo
```

## Running

Run the demo application:

```bash
bazel run //VulkanDemo:VulkanDemo
```

Or run the compiled binary directly:

```bash
./bazel-bin/VulkanDemo/VulkanDemo
```

## Project Structure

```
.
├── VulkanCore/          # Core Vulkan wrapper library
│   ├── include/         # Header files
│   └── *.cpp           # Implementation files
├── VulkanDemo/          # Demo application
│   ├── shaders/        # GLSL shaders
│   └── *.cpp           # Application code
├── MODULE.bazel        # Bazel module configuration
├── BUILD               # Root build file
└── .bazelrc           # Bazel configuration
```

## Troubleshooting

**Build fails with "absolute path inclusion" errors:**
- The `.bazelrc` is configured to use `--strategy=CppCompile=local` to avoid sandboxing issues
- Ensure GCC is properly installed: `gcc --version`

**Vulkan SDK not found:**
- Verify installation: `vulkaninfo`
- Check library paths: `ldconfig -p | grep vulkan`

**Missing shaderc:**
- Install: `sudo apt-get install libshaderc-dev`

## License

See LICENSE file for details.
