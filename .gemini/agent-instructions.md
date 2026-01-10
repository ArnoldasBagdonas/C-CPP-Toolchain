# Agent Operating Instructions

This document provides guidance on preferred strategies and patterns to ensure efficient and successful task completion.

## General Problem-Solving Strategy

- If a tool or command fails, do not immediately retry the same action. Analyze the error message to understand the cause of the failure.
- Formulate an alternative approach using a different tool or a different sequence of operations.
- Prioritize using the specific, high-level tools provided (e.g., `read_file`, `write_file`, `replace`) over general-purpose shell commands whenever possible, as they are more reliable in this environment.

## Filesystem Operations

When performing filesystem operations, be aware of the potential limitations of the execution environment. Direct shell commands may not be available.

### Creating Directories

- **Problem**: The `mkdir` shell command (or platform-specific equivalents like `robocopy`) may not be permitted.
- **Preferred Solution**: To create a new directory, use the `write_file` tool. Specify a file path (e.g., `new_directory/.gitkeep`) within the desired directory. The `write_file` tool will automatically create all necessary parent directories in the path.

### Copying Files and Directories

- **Problem**: Shell commands for copying (e.g., `cp`, `xcopy`, `robocopy`) may not be permitted.
- **Preferred Solution**: To copy files or directories, use a combination of `list_directory`, `read_file`, and `write_file`.

**To copy a single file:**
1.  Use `read_file` to read the content of the source file.
2.  Use `write_file` to write the retrieved content to the destination path.

**To copy a directory:**
1.  Use `list_directory` to get the contents of the source directory.
2.  For each file found, perform the file copy procedure described above.
3.  For each subdirectory, recursively apply this same process.

> **Limitation**: The `read_file` tool respects project ignore rules (e.g., from `.gitignore`) and cannot be configured to read files from ignored paths. Therefore, this copy method will fail if the source files are located in a directory that is subject to an ignore pattern (e.g., the `build/` or `dist/` directories).
