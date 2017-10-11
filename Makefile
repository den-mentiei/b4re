UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
         OS    := linux
         DEBUG := gdb
endif
ifeq ($(UNAME),Darwin)
	OS    := darwin
	DEBUG := lldb

	SHADERC         := ./3rdparty/bgfx/bin/osx_x64/shadercRelease
	SHADERC_OS_ARGS := "--platform osx" # TODO: metal?
endif

CONFIGURATION   := debug
PREMAKE         := ./tools/$(OS)/premake5
PROJECT_DIR     := ./.build/projects/$(OS)
TARGET_DIR      := ./.build/bin/$(OS)/$(CONFIGURATION)
PROJECT         := $(PROJECT_DIR)/Makefile
EXECUTABLE      := $(TARGET_DIR)/entry
SHADERS         := $(wildcard src/shaders/*.shader)
SHADER_INCLUDES := "3rdparty/bgfx/include"
# APP

$(EXECUTABLE): $(PROJECT) touch
	@ cd $(PROJECT_DIR) && make

$(PROJECT): shaders
	$(PREMAKE) gmake
	$(PREMAKE) clang-complete

touch:
	@ touch -c $(PROJECT)

clean:
	rm -rf .build
	rm -f src/shaders/*.h
	rm -f .clang_complete

build: $(EXECUTABLE)

run: $(EXECUTABLE)
	@ $(EXECUTABLE)

debug: $(EXECUTABLE)
	@ $(DEBUG) $(EXECUTABLE)

# SHADERS

BUILT_SHADERS_VS := $(addsuffix _vs.h, $(basename $(SHADERS)))
BUILT_SHADERS_FS := $(addsuffix _fs.h, $(basename $(SHADERS)))

src/shaders/%_vs.h: src/shaders/%.shader
	@ echo "Compiling vertex $<..."
	@ $(SHADERC) $(SHADERC_OS_ARGS)                          \
		-i $(SHADER_INCLUDES)                            \
		--varyingdef $(patsubst %.shader, %.varying, $<) \
		--define VERTEX_SHADER --type vertex             \
		-f $< -o $@                                      \
		--bin2c $(addsuffix _vs, $(notdir $(basename $<)))

src/shaders/%_fs.h: src/shaders/%.shader
	@ echo "Compiling fragment $<..."
	@ $(SHADERC) $(SHADERC_OS_ARGS)                          \
		-i $(SHADER_INCLUDES)                            \
		--varyingdef $(patsubst %.shader, %.varying, $<) \
		--define FRAGMENT_SHADER --type fragment         \
		-f $< -o $@                                      \
		--bin2c $(addsuffix _fs, $(notdir $(basename $<)))

shaders: $(BUILT_SHADERS_VS) $(BUILT_SHADERS_FS)

# ETC

print-%  : ; @echo $* = $($*)

.PHONY: completion touch clean build run debug shaders
