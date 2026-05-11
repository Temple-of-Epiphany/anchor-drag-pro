# Anchor Drag Pro — top-level Makefile
#
# Wraps the common build / flash / OTA-staging cycles. Run from the repo
# root. Auto-detects ESP-IDF, the device serial port, and SD card mount.
#
# Quick reference:
#
#   make                  # show this help
#   make build            # build firmware
#   make flash            # flash device via USB (one-time install)
#   make monitor          # serial monitor via pyserial (no TTY needed)
#   make ota VER=0.2.1    # build, copy to SD as v0.2.1, write .sha256, eject
#   make clean            # idf.py fullclean
#   make doctor           # print all detected paths + device + SD
#   make show-bin         # print bin path + size + SHA256
#   make release-tag VER=0.2.1   # commit, tag, push (triggers CI release)
#
# Override defaults on the command line, e.g.:
#
#   make flash PORT=/dev/cu.usbmodem2101
#   make ota VER=0.2.1 SD=/Volumes/SDCARD
#
# ============================================================

# ---- Configuration (override on command line) -------------------
REPO_ROOT      := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
IDF_PATH       ?= /usr/local/src/esp-idf
IDF_EXPORT     := . $(IDF_PATH)/export.sh >/dev/null 2>&1
ESP_PROJECT    := $(REPO_ROOT)/platform/esp32
BUILD_DIR      := $(ESP_PROJECT)/build
BIN            := $(BUILD_DIR)/anchor-drag-pro.bin

# Auto-detect serial port. Only matches existing entries — empty if no
# device plugged in. Prefer USB-JTAG (cu.usbmodem) over UART bridges.
PORT ?= $(firstword $(wildcard /dev/cu.usbmodem*) \
                    $(wildcard /dev/cu.SLAB_USBtoUART) \
                    $(wildcard /dev/cu.usbserial-*))

# Auto-detect SD card mount via a shell command — robust against
# spaces in volume names ("Macintosh HD") and against system-volume
# shadows. Excludes the system disk and known cache volumes.
SD ?= $(shell ls -d /Volumes/*/ 2>/dev/null | \
        grep -vE '/(Macintosh HD|Macintosh HD - Data|com\.apple|Telegram|Recovery|Preboot|VM|Update|xarts|iSCPreboot)/' | \
        head -1 | sed 's|/$$||')

# Python from the IDF venv — used by the monitor target so we don't need a TTY.
IDF_PY_VENV := /Users/colin/.espressif/python_env/idf5.5_py3.14_env/bin/python

# Source files that hold the version (bumped by `make ota VER=...`).
MAIN_C   := $(ESP_PROJECT)/main/main.c
OTA_C    := $(ESP_PROJECT)/main/ota.c

# OTA filename pattern — must match what ota.c expects.
OTA_BIN_NAME = anchor-drag-pro_v$(VER).bin
OTA_SHA_NAME = anchor-drag-pro_v$(VER).sha256

# ============================================================

.PHONY: help
help:
	@grep -E '^[a-zA-Z_-]+:.*?## ' $(firstword $(MAKEFILE_LIST)) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[1m%-22s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "  Detected paths (override with VAR=value):"
	@echo "    REPO_ROOT  = $(REPO_ROOT)"
	@echo "    IDF_PATH   = $(IDF_PATH)"
	@echo "    PORT       = $(PORT)"
	@echo "    SD         = $(SD)"

.PHONY: doctor
doctor: ## Print all detected paths and device/SD presence
	@echo "=== Anchor Drag Pro — environment check ==="
	@echo ""
	@echo "Repo root:    $(REPO_ROOT)"
	@echo "ESP project:  $(ESP_PROJECT)"
	@echo "Build dir:    $(BUILD_DIR)"
	@echo "Binary:       $(BIN)"
	@echo ""
	@echo "ESP-IDF:      $(IDF_PATH)"
	@if [ -d "$(IDF_PATH)" ]; then echo "              ✓ found"; else echo "              ✗ MISSING"; fi
	@echo ""
	@echo "Serial port:  $(PORT)"
	@if [ -e "$(PORT)" ]; then echo "              ✓ device present"; else echo "              ✗ device not connected"; fi
	@echo ""
	@echo "SD mount:     $(SD)"
	@if [ -n "$(SD)" ] && [ -d "$(SD)" ]; then echo "              ✓ SD mounted, $$(df -h "$(SD)" | tail -1 | awk '{print $$4}') free"; else echo "              ✗ no SD card detected — insert + mount, then re-run"; fi
	@echo ""
	@echo "Built bin:    $$(if [ -f $(BIN) ]; then ls -lh $(BIN) | awk '{print $$5}'; else echo '(none — run make build)'; fi)"

.PHONY: build
build: ## Build firmware (idf.py build)
	@$(IDF_EXPORT) && cd $(ESP_PROJECT) && idf.py build

.PHONY: clean
clean: ## Full clean (idf.py fullclean)
	@$(IDF_EXPORT) && cd $(ESP_PROJECT) && idf.py fullclean

.PHONY: flash
flash: build check-port ## Flash via USB (one-time install)
	@$(IDF_EXPORT) && idf.py -p $(PORT) -C $(ESP_PROJECT) flash

.PHONY: monitor
monitor: check-port ## Serial monitor (pyserial, no TTY required, ^C exits)
	@echo "Monitoring $(PORT) at 115200 baud — press Ctrl+C to exit"
	@$(IDF_PY_VENV) -c "import serial,sys; \
s=serial.Serial('$(PORT)',115200,timeout=0.2); \
[sys.stdout.write(l.decode('utf-8',errors='replace')) or sys.stdout.flush() for l in iter(s.readline,b'')] \
if False else None; \
import time; \
end = time.time() + 86400; \
while time.time() < end:\n\
  line = s.readline()\n\
  if line:\n\
    sys.stdout.write(line.decode('utf-8',errors='replace')); sys.stdout.flush()" 2>&1 || true

.PHONY: monitor-reset
monitor-reset: check-port ## Reset device via RTS, then monitor
	@echo "Resetting $(PORT) and monitoring at 115200 — Ctrl+C exits"
	@$(IDF_PY_VENV) -c "import serial,time,sys; \
s=serial.Serial('$(PORT)',115200,timeout=0.2); \
s.setDTR(False); s.setRTS(True); time.sleep(0.1); s.setRTS(False); time.sleep(0.05); \
end=time.time()+86400; \
import time as t\n\
while t.time()<end:\n\
  line = s.readline()\n\
  if line:\n\
    sys.stdout.write(line.decode('utf-8',errors='replace')); sys.stdout.flush()" 2>&1 || true

.PHONY: show-bin
show-bin: ## Print current binary path + size + SHA256
	@if [ ! -f $(BIN) ]; then echo "no binary — run make build first" >&2; exit 1; fi
	@echo "Path:   $(BIN)"
	@echo "Size:   $$(ls -lh $(BIN) | awk '{print $$5}') ($$(stat -f %z $(BIN)) bytes)"
	@echo "SHA256: $$(shasum -a 256 $(BIN) | awk '{print $$1}')"

# ----- OTA staging cycle: bump version, build, copy to SD --------

.PHONY: ota
ota: check-version check-sd ## Build + stage on SD as v$(VER) for OTA install
	@echo "=== Staging OTA release v$(VER) ==="
	@echo "Bumping in-source version to $(VER) ..."
	@MAJOR=$$(echo $(VER) | cut -d. -f1); \
	 MINOR=$$(echo $(VER) | cut -d. -f2); \
	 PATCH=$$(echo $(VER) | cut -d. -f3 | sed 's/-.*//'); \
	 if [ -z "$$MAJOR" ] || [ -z "$$MINOR" ] || [ -z "$$PATCH" ]; then \
	   echo "VER must be MAJOR.MINOR.PATCH (got '$(VER)')"; exit 1; \
	 fi; \
	 sed -i '' -E 's|#define FIRMWARE_VERSION_STRING.*|#define FIRMWARE_VERSION_STRING "$(VER)"|' $(MAIN_C); \
	 sed -i '' -E "s|#define RUNNING_VERSION_MAJOR.*|#define RUNNING_VERSION_MAJOR   $$MAJOR|" $(OTA_C); \
	 sed -i '' -E "s|#define RUNNING_VERSION_MINOR.*|#define RUNNING_VERSION_MINOR   $$MINOR|" $(OTA_C); \
	 sed -i '' -E "s|#define RUNNING_VERSION_PATCH.*|#define RUNNING_VERSION_PATCH   $$PATCH|" $(OTA_C)
	@echo "Building ..."
	@$(IDF_EXPORT) && cd $(ESP_PROJECT) && idf.py build >/dev/null
	@echo "Staging on SD: $(SD)/firmware/"
	@mkdir -p "$(SD)/firmware"
	@cp $(BIN) "$(SD)/firmware/$(OTA_BIN_NAME)"
	@( cd "$(SD)/firmware" && shasum -a 256 $(OTA_BIN_NAME) > $(OTA_SHA_NAME) )
	@echo ""
	@echo "Files on SD:"
	@ls -lh "$(SD)/firmware/" | grep $(VER) | sed 's/^/  /'
	@echo ""
	@echo "SHA256: $$(awk '{print $$1}' "$(SD)/firmware/$(OTA_SHA_NAME)")"
	@echo ""
	@echo "Next: 'make ota-eject SD=$(SD)' then insert SD into device"

.PHONY: ota-eject
ota-eject: check-sd ## Eject the SD card (after staging)
	@echo "Ejecting $(SD) ..."
	@diskutil eject "$(SD)" || true

# ----- Release tagging (triggers CI workflow on push) ------------

.PHONY: release-tag
release-tag: check-version ## Tag and push to trigger the release workflow
	@if ! git diff --quiet || ! git diff --cached --quiet; then \
	   echo "Working tree not clean — commit or stash before tagging." >&2; \
	   git status --short; exit 1; \
	 fi
	@echo "Tagging v$(VER) ..."
	@git tag -a v$(VER) -m "v$(VER)"
	@echo "Pushing tag (triggers .github/workflows/release.yml) ..."
	@git push origin master
	@git push origin v$(VER)
	@echo "Watch the workflow:"
	@echo "  gh run watch"

# ----- Pre-flight checks -----------------------------------------

.PHONY: check-port
check-port:
	@if [ -z "$(PORT)" ] || [ ! -e "$(PORT)" ]; then \
	   echo ""; \
	   echo "ERROR: serial device not found." >&2; \
	   echo "  Plug in the device via USB-C and re-run, or pass PORT=..." >&2; \
	   echo "  Detected: PORT=$(PORT)" >&2; \
	   exit 1; \
	 fi

.PHONY: check-sd
check-sd:
	@if [ -z "$(SD)" ] || [ ! -d "$(SD)" ]; then \
	   echo ""; \
	   echo "ERROR: SD card not mounted." >&2; \
	   echo "  Insert the SD card in a card reader and wait for it to mount." >&2; \
	   echo "  If your SD has an unusual name, pass SD=/Volumes/YOUR_NAME" >&2; \
	   exit 1; \
	 fi

.PHONY: check-version
check-version:
	@if [ -z "$(VER)" ]; then \
	   echo "ERROR: VER not set. Use 'make $(MAKECMDGOALS) VER=0.2.1'" >&2; \
	   exit 1; \
	 fi

# Default goal — show help when called bare.
.DEFAULT_GOAL := help
