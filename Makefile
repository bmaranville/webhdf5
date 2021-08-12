NATIVE_BUILD_DIR = native_build
NATIVE_HELPERS = $(NATIVE_BUILD_DIR)/src/H5detect $(NATIVE_BUILD_DIR)/src/H5make_libsettings
WASM_BUILD_DIR = wasm_build
WASM_LIB_DIR = $(WASM_BUILD_DIR)/hdf5/lib
WASM_INCLUDE_DIR = $(WASM_BUILD_DIR)/hdf5/include
WASM_LIBS = $(WASM_LIB_DIR)/libhdf5.a $(WASM_LIB_DIR)/libhdf5_hl.a 

#WASM_LIBS = libhdf5.a
HDF5_SRC=libhdf5
SRC = .
APP_DIR = dist
APP = $(APP_DIR)/h5js_util.js
LIBHDF5 = $(APP_DIR)/libhdf5.js

app: $(APP)
all: $(APP) $(LIBHDF5) 
wasm: $(WASM_LIBS)
native: $(NATIVE_HELPERS)

C_FLAGS = \
   -Wno-incompatible-pointer-types-discards-qualifiers \
   -Wno-misleading-indentation \
   -Wno-missing-braces \
   -Wno-self-assign \
   -Wno-sometimes-uninitialized \
   -Wno-unknown-warning-option \
   -Wno-unused-but-set-variable \
   -Wno-unused-function \
   -Wno-unused-variable \

$(WASM_LIBS):
	mkdir -p $(WASM_BUILD_DIR);
	cd $(WASM_BUILD_DIR) \
        && emcmake cmake ../$(HDF5_SRC) \
		-DCMAKE_INSTALL_PREFIX=hdf5 \
	    -DH5_HAVE_GETPWUID=0 \
		-DH5_HAVE_SIGNAL=0 \
        -DBUILD_SHARED_LIBS=0 \
        -DBUILD_STATIC_LIBS=1 \
        -DBUILD_TESTING=0 \
        -DCMAKE_C_FLAGS=$(C_FLAGS) \
        -DHDF5_BUILD_EXAMPLES=0 \
        -DHDF5_BUILD_TOOLS=0 \
        -DHDF5_ENABLE_Z_LIB_SUPPORT=1 \
        -DHDF5_BUILD_MODE_PRODUCTION=1;
	-cd $(WASM_BUILD_DIR) && grep -rl "\-l\"\-O0" | xargs sed -i 's/-l//g';
	-cd $(WASM_BUILD_DIR) && grep -rl 'H5detect.js /' | xargs sed -i 's/H5detect.js \//H5detect.js > \//g';
	-cd $(WASM_BUILD_DIR) && grep -rl 'H5make_libsettings.js /' | xargs sed -i 's/H5make_libsettings.js \//H5make_libsettings.js > \//g';
	cd $(WASM_BUILD_DIR) && emmake make -j8 install;

$(LIBHDF5): $(WASM_LIBS)
	emcc -O3 $(WASM_LIBS) \
	  -o $(APP_DIR)/libhdf5.html \
	  -I$(WASM_INCLUDE_DIR)/src \
	  -s WASM_BIGINT \
	  -s FORCE_FILESYSTEM=1 \
	  -s USE_ZLIB=1 \
	  -s EXPORT_ALL=1 \
	  -s LINKABLE=1 \
	  -s INCLUDE_FULL_LIBRARY=1 \
	  -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'FS']"


#	  -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'FS']"

$(APP): $(SRC)/hdf5_util.cc $(WASM_LIBS)
	emcc -O3 $(WASM_LIBS) $(SRC)/hdf5_util.cc -o $(APP_DIR)/hdf5_util.js \
        -I$(WASM_INCLUDE_DIR) \
        --bind  \
        -s ALLOW_TABLE_GROWTH=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
		-s WASM_BIGINT \
		-s EXPORT_ES6=1 \
		-s MODULARIZE=1 \
		-s FORCE_FILESYSTEM=1 \
		-s USE_ZLIB=1 \
		-s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'FS']" \
		-s EXPORTED_FUNCTIONS="['_H5Fopen', '_H5Fclose']"
	  
clean:
	rm -rf $(NATIVE_BUILD_DIR);
	rm -rf $(WASM_BUILD_DIR);
	rm -f $(APP_DIR)/h5js_util.*;
	rm -f $(APP_DIR)/libhdf5.*;
