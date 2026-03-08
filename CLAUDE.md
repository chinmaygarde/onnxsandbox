# ONNX Sandbox

A sandbox project for exploring ONNX Runtime and authoring a Vulkan Execution Provider.

## Building and Testing

Always use the justfile tasks to build and test the project. After making any code changes:
1. Run `just format` to format all source files.
2. Run `just build` and `just test` to build and test, fixing any errors before finishing.

## Code Style

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
