# =============================================================================
# host-app/Makefile
# =============================================================================
#
# Thin orchestrator for the two-stage build. All configuration (flags, paths,
# generators) lives in CMakePresets.json — this Makefile only detects available
# compilers, calls cmake --preset, and manages stamp files.
#
# Usage:
#   make help          Show available targets
#   make third-party   Build all third_party variants for detected compilers
#   make status        Show which variants are installed
#   make clean         Remove all build artifacts
#
# =============================================================================

# ----------------------------------------------------------------------------- 
# Configuration
# ----------------------------------------------------------------------------- 
# BUILD_ROOT env var is the single source of truth, defined in
# .devcontainer/devcontainer.json → remoteEnv → BUILD_ROOT.
ifndef BUILD_ROOT
    $(error BUILD_ROOT is not set. It should be defined in .devcontainer/devcontainer.json remoteEnv)
endif
TP_SRC     := third_party
TP_DIR     := $(BUILD_ROOT)/workspace2/third_party

# Detect available compilers
HAS_GCC   := $(shell command -v g++ 2>/dev/null)
HAS_CLANG := $(shell command -v clang++ 2>/dev/null)

# GCC presets: default + sanitizers (no msan — GCC doesn't support it)
GCC_PRESETS   := gcc-default gcc-asan gcc-tsan gcc-ubsan
CLANG_PRESETS := clang-default clang-asan clang-tsan clang-msan clang-ubsan

# Build the list of presets to install based on detected compilers
PRESETS :=
ifdef HAS_GCC
    PRESETS += $(GCC_PRESETS)
endif
ifdef HAS_CLANG
    PRESETS += $(CLANG_PRESETS)
endif

# Stamp files — one per preset
STAMPS := $(foreach p,$(PRESETS),$(TP_DIR)/$(p)/.installed)

# =============================================================================
# Help
# =============================================================================
.DEFAULT_GOAL := help

.PHONY: help
help:
	@echo ""
	@echo "  Usage: make <target>"
	@echo ""
	@echo "  Third-party targets:"
	@echo "    third-party        Build all third_party variants for detected compilers"
	@echo "    status             Show which variants are installed"
	@echo ""
	@echo "  Clean targets:"
	@echo "    clean              Remove all build artifacts"
	@echo "    clean-third-party  Remove only third_party build artifacts"
	@echo ""
	@echo "  After third-party is built, use CMake presets directly:"
	@echo "    cmake --preset gcc-debug && cmake --build --preset gcc-debug"
	@echo "    ctest --preset gcc-debug"
	@echo ""

# =============================================================================
# Third-party: build all detected variants
# =============================================================================
# Each stamp file depends on third_party/CMakeLists.txt — a change to the
# dependency definition triggers a rebuild.
# =============================================================================

.PHONY: third-party
third-party: $(STAMPS)
	@echo ""
	@echo "  All third_party variants installed."
	@echo ""

# Pattern rule: build and install a single third_party preset.
# The preset name is extracted from the path: build/third_party/<preset>/.installed
# Note: the build step must run from $(TP_SRC) so cmake reads the correct
# CMakePresets.json (third_party's presets define the right binaryDir).
$(TP_DIR)/%/.installed: $(TP_SRC)/CMakeLists.txt
	cmake --preset $* -S $(TP_SRC)
	cd $(TP_SRC) && cmake --build --preset $*
	@touch $@

# =============================================================================
# Status
# =============================================================================

.PHONY: status
status:
	@echo ""
	@echo "  Third-party install status:"
	@echo ""
ifdef HAS_GCC
	@echo "  GCC ($(shell g++ --version | head -1)):"
	@$(foreach p,$(GCC_PRESETS),\
		printf "    %-20s %s\n" "$(p)" \
		"$$([ -f $(TP_DIR)/$(p)/.installed ] && echo '✓ installed' || echo '✗ not installed')";)
	@echo ""
endif
ifdef HAS_CLANG
	@echo "  Clang ($(shell clang++ --version | head -1)):"
	@$(foreach p,$(CLANG_PRESETS),\
		printf "    %-20s %s\n" "$(p)" \
		"$$([ -f $(TP_DIR)/$(p)/.installed ] && echo '✓ installed' || echo '✗ not installed')";)
	@echo ""
endif
ifndef HAS_GCC
ifndef HAS_CLANG
	@echo "  No compilers detected (g++, clang++)."
	@echo ""
endif
endif

# =============================================================================
# Clean targets
# =============================================================================

.PHONY: clean
clean:
	rm -rf $(BUILD_ROOT)

.PHONY: clean-third-party
clean-third-party:
	rm -rf $(TP_DIR)
