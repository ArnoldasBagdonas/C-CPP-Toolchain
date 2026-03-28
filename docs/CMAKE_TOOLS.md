# Using Non-Root CMakeLists.txt with VS Code CMake Tools

This guide explains two ways to configure VS Code CMake Tools to use a CMakeLists.txt file in a subfolder (not the workspace root).

## 1. `cmake.sourceDirectory` Setting (Single-Folder Workspace)

- Add the following to your `.vscode/settings.json`:
  ```json
  "cmake.sourceDirectory": "${workspaceFolder}/host"
  ```
- This tells CMake Tools to use the CMakeLists.txt in `/workspace/host` instead of the root.
- All CMake operations (configure, build, etc.) will use this subfolder as the project root.
- Use this if you want to work with only one CMake project at a time.

## 2. Multi-Root Workspace (Multiple Projects at Once)

- Open VS Code’s Command Palette (Ctrl+Shift+P) → **Workspaces: Add Folder to Workspace…**
- Add both `/workspace` and `/workspace/host` as separate folders.
- VS Code creates a `.code-workspace` file to manage both folders.
- Each folder can have its own `.vscode/settings.json`, so you can set different CMake settings for each (e.g., embedded in `/workspace`, host in `/workspace/host`).
- You can switch between and build both projects independently within the same VS Code window.
