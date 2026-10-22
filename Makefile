CXX := g++
CXXFLAGS := -std=c++17 -fPIC -O3
LDFLAGS := -shared
PREFIX := /usr
JSON := libbcn_layer.json

# Ruta optimizada para Winlator Bionic / Box64 (x86_64)
INSTALL := $(PREFIX)/lib/x86_64-linux-gnu
JSON_INSTALL := $(PREFIX)/share/vulkan/implicit_layer.d

SOURCES := src/bcn_layer.cpp \
		   src/image.cpp \
	       src/buffer.cpp \
	       src/bcn.cpp \
	       src/command_buffer.cpp \
	       src/queue.cpp \
	       src/fence.cpp \
	       src/logger.cpp

HEADERS := src/bcn_layer.hpp \
		   src/image.hpp \
		   src/buffer.hpp \
		   src/bcn.hpp \
		   src/command_buffer.hpp \
		   src/queue.hpp \
		   src/fence.hpp \
		   src/logger.hpp \
		   src/vk_func.hpp \
		   src/vulkan/vk_layer.h

SPIRV_SHADERS := src/s3tc.spv \
                 src/s3tc_iv.spv \
                 src/rgtc.spv \
                 src/rgtc_iv.spv \
                 src/bc6.spv \
                 src/bc6_iv.spv \
                 src/bc7.spv \
                 src/bc7_iv.spv

SPIRV_HEADERS := src/s3tc_spv.h \
				 src/s3tc_iv_spv.h \
				 src/rgtc_spv.h \
				 src/rgtc_iv_spv.h \
				 src/bc6_spv.h \
				 src/bc6_iv_spv.h \
				 src/bc7_spv.h \
				 src/bc7_iv_spv.h
	      
OUTPUT := libbcn_layer.so

all : $(OUTPUT)

# CORRECCIÓN: Usamos glslangValidator -V para compilar los shaders sin depender de glslc
src/%.spv : src/%.comp
	glslangValidator -V $< -o $@

src/%_spv.h : src/%.spv
	xxd -i $< > $@
	
$(OUTPUT) : $(SOURCES) $(SPIRV_HEADERS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) -o $(OUTPUT)

.PHONY: clean install

install: $(OUTPUT)
	install -d $(INSTALL)
	install -m 755 $(OUTPUT) $(INSTALL)
	install -d $(JSON_INSTALL)
	install -m 644 $(JSON) $(JSON_INSTALL)

clean:
	rm -rf $(OUTPUT)
	rm -rf $(SPIRV_SHADERS)
	rm -rf $(SPIRV_HEADERS)
