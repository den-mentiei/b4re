PLATFORM := linux
CONFIGURATION := debug
PREMAKE := ./tools/$(PLATFORM)/premake5

PROJECT_DIR := ./.build/projects/$(PLATFORM)
TARGET_DIR := ./.build/bin/$(PLATFORM)/$(CONFIGURATION)

PROJECT := $(PROJECT_DIR)/Makefile
EXECUTABLE := $(TARGET_DIR)/entry

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
	@ gdb $(EXECUTABLE)

.PHONY: completion touch clean build run debug
