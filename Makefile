PLATFORM := linux
CONFIGURATION := debug

PROJECT_DIR := ./.build/projects/$(PLATFORM)
TARGET_DIR := ./.build/bin/$(PLATFORM)/$(CONFIGURATION)

PROJECT := $(PROJECT_DIR)/Makefile
EXECUTABLE := $(TARGET_DIR)/entry

$(EXECUTABLE): $(PROJECT) touch
	cd $(PROJECT_DIR) && make

$(PROJECT):
	./tools/linux/premake5 gmake

touch:
	@touch -c $(PROJECT)

clean:
	rm -rf .build

build: $(EXECUTABLE)

run: $(EXECUTABLE)
	$(EXECUTABLE)

.PHONY: touch clean build run
