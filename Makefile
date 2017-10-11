UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
         OS    := linux
         DEBUG := gdb
endif
ifeq ($(UNAME),Darwin)
	OS    := darwin
	DEBUG := lldb
endif

CONFIGURATION := debug
PREMAKE       := ./tools/$(OS)/premake5
PROJECT_DIR   := ./.build/projects/$(OS)
TARGET_DIR    := ./.build/bin/$(OS)/$(CONFIGURATION)
PROJECT       := $(PROJECT_DIR)/Makefile
EXECUTABLE    := $(TARGET_DIR)/entry

$(EXECUTABLE): $(PROJECT) touch
	@ cd $(PROJECT_DIR) && make

$(PROJECT):
	$(PREMAKE) gmake
	$(PREMAKE) clang-complete

touch:
	@touch -c $(PROJECT)

clean:
	rm -rf .build
	rm -f .clang_complete

build: $(EXECUTABLE)

run: $(EXECUTABLE)
	@ $(EXECUTABLE)

debug: $(EXECUTABLE)
	@ $(DEBUG) $(EXECUTABLE)

.PHONY: completion touch clean build run debug
