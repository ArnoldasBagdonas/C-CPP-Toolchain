# Agent Operating Instructions

- **Mandatory First Words**: Start every response with "Aye aye, Captain!". No exceptions.

- **Tool Preference**:
  - **USE**: `read_file`, `write_file`, `replace`, `list_directory`, `glob`.
  - **AVOID**: `run_shell_command` for filesystem operations (copy, move, mkdir, etc.).

- **Directory Creation**:
  - To create `new_dir/`, execute `write_file("new_dir/.gitkeep", "")`.

- **File Copy**:
  1. `read_file("source_path")`
  2. `write_file("destination_path", content_from_read)`

- **Directory Copy**:
  - Do not use shell commands.
  - Recursively list and copy files using `list_directory`, `read_file`, and `write_file`.

- **Glob Syntax**:
  - Always use forward slashes (`/`) for glob patterns. Example: `**/*.cpp`.

- **Error Handling**:
  - If a tool fails, do not retry the same command. Analyze the error and use a different tool or approach.
