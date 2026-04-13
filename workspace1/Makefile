# =============================================================================
# host-app/Makefile
# =============================================================================
#
# Orchestrates third_party and main project builds with proper dependency
# chains. Sanitizer builds automatically get a matching third_party built
# with the same instrumentation flags.
#
# Usage:
#   make help          Show all available targets
#   make debug         Build Debug variant (default third_party → app)
#   make asan          Build ASan variant  (asan third_party → app)
#   make test-debug    Build + run Debug tests
#
# =============================================================================

# ----------------------------------------------------------------------------- 
# Configuration
# ----------------------------------------------------------------------------- 
GENERATOR       ?= Ninja
BUILD_ROOT      := build
TP_SRC          := third_party

# Third-party build directories (one per variant group)
TP_DEFAULT_DIR  := $(BUILD_ROOT)/third_party
TP_ASAN_DIR     := $(BUILD_ROOT)/third_party-asan
TP_TSAN_DIR     := $(BUILD_ROOT)/third_party-tsan
TP_MSAN_DIR     := $(BUILD_ROOT)/third_party-msan
TP_UBSAN_DIR    := $(BUILD_ROOT)/third_party-ubsan

# Stamp files — make uses these to track whether third_party is up to date
TP_DEFAULT_STAMP := $(TP_DEFAULT_DIR)/.installed
TP_ASAN_STAMP    := $(TP_ASAN_DIR)/.installed
TP_TSAN_STAMP    := $(TP_TSAN_DIR)/.installed
TP_MSAN_STAMP    := $(TP_MSAN_DIR)/.installed
TP_UBSAN_STAMP   := $(TP_UBSAN_DIR)/.installed

# =============================================================================
# Help
# =============================================================================
.DEFAULT_GOAL := help

.PHONY: help
help:
	@echo ""
	@echo "  Usage: make <target>"
	@echo ""
	@echo "  Build targets (third_party is built automatically if needed):"
	@echo "    debug              Debug build"
	@echo "    release            Release build"
	@echo "    coverage           Coverage build (gcov / llvm-cov)"
	@echo "    valgrind           Valgrind-compatible debug build"
	@echo "    asan               AddressSanitizer build"
	@echo "    tsan               ThreadSanitizer build"
	@echo "    msan               MemorySanitizer build (Clang only)"
	@echo "    ubsan              UndefinedBehaviorSanitizer build"
	@echo ""
	@echo "  Test targets (builds first, then runs ctest):"
	@echo "    test-debug         Run tests — Debug"
	@echo "    test-release       Run tests — Release"
	@echo "    test-coverage      Run tests — Coverage"
	@echo "    test-asan          Run tests — AddressSanitizer"
	@echo "    test-tsan          Run tests — ThreadSanitizer"
	@echo "    test-msan          Run tests — MemorySanitizer"
	@echo "    test-ubsan         Run tests — UndefinedBehaviorSanitizer"
	@echo ""
	@echo "  Third-party targets (usually automatic, but can be run manually):"
	@echo "    third-party        Default third_party (no sanitizer flags)"
	@echo "    third-party-asan   Third_party with AddressSanitizer"
	@echo "    third-party-tsan   Third_party with ThreadSanitizer"
	@echo "    third-party-msan   Third_party with MemorySanitizer"
	@echo "    third-party-ubsan  Third_party with UndefinedBehaviorSanitizer"
	@echo ""
	@echo "  Utility targets:"
	@echo "    coverage-report    Generate HTML coverage report"
	@echo "    format             Run clang-format on source files"
	@echo "    format-check       Check formatting (dry run)"
	@echo "    clean              Remove all build artifacts"
	@echo "    clean-third-party  Remove only third_party build artifacts"
	@echo "    clean-app          Remove only application build artifacts"
	@echo ""

# =============================================================================
# Third-party: default (no sanitizer flags)
# =============================================================================
# Used by: debug, release, coverage, valgrind

$(TP_DEFAULT_STAMP):
	cmake -S $(TP_SRC) -B $(TP_DEFAULT_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(TP_DEFAULT_DIR)/install
	cmake --build $(TP_DEFAULT_DIR) --target install
	@touch $@

.PHONY: third-party
third-party: $(TP_DEFAULT_STAMP)

# =============================================================================
# Third-party: AddressSanitizer
# =============================================================================

$(TP_ASAN_STAMP):
	cmake -S $(TP_SRC) -B $(TP_ASAN_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(TP_ASAN_DIR)/install \
		-DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
		-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
		-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address"
	cmake --build $(TP_ASAN_DIR) --target install
	@touch $@

.PHONY: third-party-asan
third-party-asan: $(TP_ASAN_STAMP)

# =============================================================================
# Third-party: ThreadSanitizer
# =============================================================================

$(TP_TSAN_STAMP):
	cmake -S $(TP_SRC) -B $(TP_TSAN_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(TP_TSAN_DIR)/install \
		-DCMAKE_C_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g" \
		-DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g" \
		-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread"
	cmake --build $(TP_TSAN_DIR) --target install
	@touch $@

.PHONY: third-party-tsan
third-party-tsan: $(TP_TSAN_STAMP)

# =============================================================================
# Third-party: MemorySanitizer (Clang only)
# =============================================================================

$(TP_MSAN_STAMP):
	cmake -S $(TP_SRC) -B $(TP_MSAN_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(TP_MSAN_DIR)/install \
		-DCMAKE_C_FLAGS="-fsanitize=memory -fno-omit-frame-pointer -g" \
		-DCMAKE_CXX_FLAGS="-fsanitize=memory -fno-omit-frame-pointer -g" \
		-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=memory" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=memory"
	cmake --build $(TP_MSAN_DIR) --target install
	@touch $@

.PHONY: third-party-msan
third-party-msan: $(TP_MSAN_STAMP)

# =============================================================================
# Third-party: UndefinedBehaviorSanitizer
# =============================================================================

$(TP_UBSAN_STAMP):
	cmake -S $(TP_SRC) -B $(TP_UBSAN_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(TP_UBSAN_DIR)/install \
		-DCMAKE_C_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g" \
		-DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g" \
		-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=undefined"
	cmake --build $(TP_UBSAN_DIR) --target install
	@touch $@

.PHONY: third-party-ubsan
third-party-ubsan: $(TP_UBSAN_STAMP)

# =============================================================================
# Application builds
# =============================================================================
# Each target depends on the matching third_party variant.
# Make will only build third_party if the stamp file is missing.
# =============================================================================

# ----------------------------------------------------------------------------- 
# Debug
# ----------------------------------------------------------------------------- 
.PHONY: debug
debug: $(TP_DEFAULT_STAMP)
	cmake -S . -B $(BUILD_ROOT)/debug -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_DEFAULT_DIR)/install
	cmake --build $(BUILD_ROOT)/debug

# ----------------------------------------------------------------------------- 
# Release
# ----------------------------------------------------------------------------- 
.PHONY: release
release: $(TP_DEFAULT_STAMP)
	cmake -S . -B $(BUILD_ROOT)/release -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Release \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_DEFAULT_DIR)/install
	cmake --build $(BUILD_ROOT)/release

# ----------------------------------------------------------------------------- 
# Coverage
# ----------------------------------------------------------------------------- 
.PHONY: coverage
coverage: $(TP_DEFAULT_STAMP)
	cmake -S . -B $(BUILD_ROOT)/coverage -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Coverage \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_DEFAULT_DIR)/install
	cmake --build $(BUILD_ROOT)/coverage

# ----------------------------------------------------------------------------- 
# Valgrind
# ----------------------------------------------------------------------------- 
.PHONY: valgrind
valgrind: $(TP_DEFAULT_STAMP)
	cmake -S . -B $(BUILD_ROOT)/valgrind -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Valgrind \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_DEFAULT_DIR)/install
	cmake --build $(BUILD_ROOT)/valgrind

# ----------------------------------------------------------------------------- 
# AddressSanitizer
# ----------------------------------------------------------------------------- 
.PHONY: asan
asan: $(TP_ASAN_STAMP)
	cmake -S . -B $(BUILD_ROOT)/asan -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=AddressSanitizer \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_ASAN_DIR)/install
	cmake --build $(BUILD_ROOT)/asan

# ----------------------------------------------------------------------------- 
# ThreadSanitizer
# ----------------------------------------------------------------------------- 
.PHONY: tsan
tsan: $(TP_TSAN_STAMP)
	cmake -S . -B $(BUILD_ROOT)/tsan -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=ThreadSanitizer \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_TSAN_DIR)/install
	cmake --build $(BUILD_ROOT)/tsan

# ----------------------------------------------------------------------------- 
# MemorySanitizer
# ----------------------------------------------------------------------------- 
.PHONY: msan
msan: $(TP_MSAN_STAMP)
	cmake -S . -B $(BUILD_ROOT)/msan -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=MemorySanitizer \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_MSAN_DIR)/install
	cmake --build $(BUILD_ROOT)/msan

# ----------------------------------------------------------------------------- 
# UndefinedBehaviorSanitizer
# ----------------------------------------------------------------------------- 
.PHONY: ubsan
ubsan: $(TP_UBSAN_STAMP)
	cmake -S . -B $(BUILD_ROOT)/ubsan -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=UndefinedBehaviorSanitizer \
		-DTHIRD_PARTY_INSTALL_DIR=$(CURDIR)/$(TP_UBSAN_DIR)/install
	cmake --build $(BUILD_ROOT)/ubsan

# =============================================================================
# Test targets
# =============================================================================
# Each test target depends on its build target, which in turn depends on
# the matching third_party. The full chain is:
#
#   make test-asan  →  asan  →  third-party-asan  →  build + run tests
# =============================================================================

.PHONY: test-debug
test-debug: debug
	cd $(BUILD_ROOT)/debug && ctest --output-on-failure

.PHONY: test-release
test-release: release
	cd $(BUILD_ROOT)/release && ctest --output-on-failure

.PHONY: test-coverage
test-coverage: coverage
	cd $(BUILD_ROOT)/coverage && ctest --output-on-failure

.PHONY: test-asan
test-asan: asan
	cd $(BUILD_ROOT)/asan && ctest --output-on-failure

.PHONY: test-tsan
test-tsan: tsan
	cd $(BUILD_ROOT)/tsan && ctest --output-on-failure

.PHONY: test-msan
test-msan: msan
	cd $(BUILD_ROOT)/msan && ctest --output-on-failure

.PHONY: test-ubsan
test-ubsan: ubsan
	cd $(BUILD_ROOT)/ubsan && ctest --output-on-failure

# =============================================================================
# Coverage report
# =============================================================================

.PHONY: coverage-report
coverage-report: coverage
	cmake --build $(BUILD_ROOT)/coverage --target coverage_report

# =============================================================================
# Formatting
# =============================================================================

.PHONY: format
format: debug
	cmake --build $(BUILD_ROOT)/debug --target format

.PHONY: format-check
format-check: debug
	cmake --build $(BUILD_ROOT)/debug --target format-check

# =============================================================================
# Clean targets
# =============================================================================

.PHONY: clean
clean:
	rm -rf $(BUILD_ROOT)

.PHONY: clean-third-party
clean-third-party:
	rm -rf $(TP_DEFAULT_DIR) $(TP_ASAN_DIR) $(TP_TSAN_DIR) $(TP_MSAN_DIR) $(TP_UBSAN_DIR)

.PHONY: clean-app
clean-app:
	rm -rf $(BUILD_ROOT)/debug $(BUILD_ROOT)/release $(BUILD_ROOT)/coverage \
		$(BUILD_ROOT)/valgrind $(BUILD_ROOT)/asan $(BUILD_ROOT)/tsan \
		$(BUILD_ROOT)/msan $(BUILD_ROOT)/ubsan
