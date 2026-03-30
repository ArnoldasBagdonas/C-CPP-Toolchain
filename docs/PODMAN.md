# Using Podman Instead of Docker (Windows)

This project supports Podman as a drop-in replacement for Docker for local development, including VS Code Dev Containers.

## 1. Install Podman

- Download and install Podman: [Podman Installation Guide](https://podman.io/getting-started/installation)
- After installation, initialize and start the Podman machine:

  ```powershell
  podman machine init
  podman machine start
  ```
- Confirm the machine is running and you have an active connection:

  ```powershell
  podman machine list
  podman system connection list
  podman info
  ```

  If `podman system connection list` shows multiple connections, set the default one:

  ```powershell
  podman system connection default <connection-name>
  ```
- Verify installation:

  ```powershell
  podman --version
  ```

## 2. Locate the Podman Executable

Find where `podman.exe` is installed:

```powershell
(Get-Command podman).Source
```

Example output:

```
C:\Users\<you>\AppData\Local\Programs\Podman\podman.exe
```

## 3. Create Docker Compatibility Symlink (Optional)

Some tools expect `docker.exe`. You can map it to Podman:

1. Open **Command Prompt as Administrator**
2. Run:

   ```cmd
   cd "<directory containing podman.exe>"
   mklink docker.exe podman.exe
   ```

   Example:

   ```cmd
   cd "C:\Users\<you>\AppData\Local\Programs\Podman"
   mklink docker.exe podman.exe
   ```

3. Verify:

   ```cmd
   where.exe docker
   docker --version
   ```

## 4. Configure VS Code (Recommended)

Set Podman explicitly as the container runtime in your settings:

```json
"dev.containers.dockerPath": "podman"
```

Restart VS Code after applying this change.

If you still see Dev Containers running `docker ...` commands, ensure:

- `podman machine` is started (the error `connectex: No connection could be made because the target machine actively refused it` usually means the VM/socket isn’t running)
- `where.exe docker` and `where.exe podman` point to what you expect (no stale `docker.exe` earlier in `PATH`)

## 5. Test the Setup

Run the following to confirm everything works:

```powershell
podman info
podman run hello-world
```

If using the compatibility symlink:

```cmd
docker run hello-world
```
## 6. Recreate the Podman machine

If the machine reports as running but the API endpoint refuses connections, the state is corrupted (stale connection, broken port forward, or WSL desync). At this point, stop troubleshooting incrementally and rebuild the machine cleanly:


```cmd
podman machine stop
wsl --shutdown
Start-Sleep -Seconds 5
podman machine start
podman system connection list
podman info
```

---

### Notes
- The symlink is a compatibility workaround, not a full Docker replacement.
- Podman runs containers inside a WSL2-based VM on Windows.
- If `podman run` fails, fix Podman itself before troubleshooting this project.
