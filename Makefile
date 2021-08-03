NATIVE_BUILD_DIR = native_build
NATIVE_HELPERS = $(NATIVE_BUILD_DIR)/src/H5detect $(NATIVE_BUILD_DIR)/src/H5make_libsettings
WASM_BUILD_DIR = wasm_build
WASM_LIBS = $(WASM_BUILD_DIR)/src/.libs/libhdf5.a $(WASM_BUILD_DIR)/c++/src/.libs/libhdf5_cpp.a $(WASM_BUILD_DIR)/hl/src/.libs/libhdf5_hl.a
SRC=libhdf5
CONFIGURE=$(SRC)/configure
APP_DIR = dist
APP = $(APP_DIR)/h5js_module.js
LIBHDF5 = $(APP_DIR)/libhdf5.js

all: $(APP) $(LIBHDF5) 
wasm: $(WASM_LIBS)
native: $(NATIVE_HELPERS)

$(CONFIGURE): $(SRC)/autogen.sh
	cd $(SRC) && ./autogen.sh;

$(NATIVE_HELPERS): $(CONFIGURE)
	mkdir -p $(NATIVE_BUILD_DIR);
	cd $(NATIVE_BUILD_DIR) \
          && ../$(CONFIGURE) --host=i686-linux-gnu "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32" \
                      --disable-tests \
                      --disable-deprecated-symbols;
	cd $(NATIVE_BUILD_DIR)/src && make H5detect H5make_libsettings;

# Copy H5detect and H5detect.o, H5make_libsettings and H5make_libsettings.o
# to wasm_build directory so that they can be used in this build:
# (if .o files are not copied it tries to build tools again)
$(WASM_LIBS): $(NATIVE_HELPERS)
	mkdir -p $(WASM_BUILD_DIR)/src;
	cd $(WASM_BUILD_DIR) \
          && emconfigure ../$(CONFIGURE) LIBS=-lz \
                      --disable-tests \
                      --enable-cxx \
                      --enable-build-mode=production \
                      --disable-tools \
                      --disable-shared \
                      --disable-deprecated-symbols;
	cp $(NATIVE_BUILD_DIR)/src/H5detect* $(WASM_BUILD_DIR)/src/;
	chmod a+x $(WASM_BUILD_DIR)/src/H5detect;
	cp $(NATIVE_BUILD_DIR)/src/H5make_libsettings* $(WASM_BUILD_DIR)/src/;
	chmod a+x $(WASM_BUILD_DIR)/src/H5make_libsettings;
	cd $(WASM_BUILD_DIR) && emmake make -j8;

$(LIBHDF5): $(WASM_LIBS)
	emcc -O3 $(WASM_BUILD_DIR)/src/.libs/libhdf5.a $(WASM_BUILD_DIR)/hl/src/.libs/libhdf5_hl.a \
	  -o $(APP_DIR)/libhdf5.html \
	  -I$(WASM_BUILD_DIR)/src \
	  -I$(SRC)/src -I$(SRC)/hl/src/ \
	  -s WASM_BIGINT \
	  -s FORCE_FILESYSTEM=1 \
	  -s USE_ZLIB=1 \
	  -s EXPORT_ALL=1 \
	  -s LINKABLE=1

$(APP): h5js_lib.cpp $(WASM_LIBS)
	emcc -O3 $(WASM_LIBS) h5js_lib.cpp -o $(APP_DIR)/h5js_module.js \
	  --bind \
	  -I$(WASM_BUILD_DIR)/src \
	  -I$(SRC)/src -I$(SRC)/c++/src -I$(SRC)/hl/src/ \
	  --bind \
	  -s WASM_BIGINT \
	  -s EXPORT_ES6=1 \
	  -s MODULARIZE=1 \
	  -s FORCE_FILESYSTEM=1 \
	  -s USE_ZLIB=1 \
	  -s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'FS']"
	  
clean:
	rm -rf $(NATIVE_BUILD_DIR);
	rm -rf $(WASM_BUILD_DIR);
	rm -f $(APP_DIR)/h5js_module.*;
	rm -f $(APP_DIR)/libhdf5.*;