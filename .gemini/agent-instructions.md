# Agent Operating Instructions

## General Strategy
- Analyze errors before retrying; use an alternative approach if something fails.
- Prefer high-level tools (`read_file`, `write_file`, `replace`, `list_directory`, `glob`) over shell commands.

## Filesystem Operations
Shell-based filesystem commands may be unavailable—use provided tools instead.

### Creating Directories
- Use `write_file` with a file path inside the target directory (e.g., `dir/.gitkeep`).
- Parent directories are created automatically.

### Copying Files and Directories
- Avoid shell copy commands.
- **Copy a file**: `read_file` → `write_file`
- **Copy a directory**:
  1. `list_directory`
  2. Copy each file
  3. Recursively process subdirectories

> **Limitation**: `read_file` cannot access ignored paths (e.g., `build/`, `dist/`, `.gitignore` entries).

## Glob Usage
- Always use forward slashes (`/`) in glob patterns on all platforms.