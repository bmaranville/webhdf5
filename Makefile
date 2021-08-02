NATIVE_BUILD_DIR = native_build
NATIVE_HELPERS = $(NATIVE_BUILD_DIR)/src/H5detect $(NATIVE_BUILD_DIR)/src/H5make_libsettings
WASM_BUILD_DIR = wasm_build
WASM_LIBS = $(WASM_BUILD_DIR)/src/.libs/libhdf5.a $(WASM_BUILD_DIR)/c++/src/.libs/libhdf5_cpp.a $(WASM_BUILD_DIR)/hl/src/.libs/libhdf5_hl.a
SRC=libhdf5
CONFIGURE=$(SRC)/configure
APP_DIR = dist
APP = $(APP_DIR)/libhdf5_hl.js

all: $(APP) 
wasm: $(WASM_LIBS)
native: $(NATIVE_HELPERS)

$(CONFIGURE): $(SRC)/autogen.sh
	cd $(SRC) && ./autogen.sh;

$(NATIVE_HELPERS): $(CONFIGURE)
	mkdir -p $(NATIVE_BUILD_DIR);
	cd $(NATIVE_BUILD_DIR) \
          && ../$(CONFIGURE) LIBS=-lz \
                      --disable-tests \
                      --enable-cxx \
                      --enable-build-mode=production \
                      --disable-tools \
                      --disable-shared \
                      --disable-deprecated-symbols;
	cd $(NATIVE_BUILD_DIR) && make -j8;

$(WASM_LIBS): $(NATIVE_HELPERS)
	mkdir -p $(WASM_BUILD_DIR)/src;
	cd $(WASM_BUILDDIR) \
          && emconfigure ../$(CONFIGURE) LIBS=-lz \
                      --disable-tests \
                      --enable-cxx \
                      --enable-build-mode=production \
                      --disable-tools \
                      --disable-shared \
                      --disable-deprecated-symbols;
	cp $(NATIVE_BUILD_DIR)/src/H5detect $(WASM_BUILD_DIR)/src/;
	chmod a+x $(WASM_BUILD_DIR)/src/H5detect;
	cp $(NATIVE_BUILD_DIR)/src/H5make_libsettings $(WASM_BUILD_DIR)/src/;
	chmod a+x $(WASM_BUILD_DIR)/src/H5make_libsettings;
	cd $(WASM_BUILD_DIR) && emmake make -j8;

$(APP): h5js_lib.cpp $(WASM_LIBS)
	emcc -O3 $(WASM_LIBS) h5js_lib.cpp -o $(APP_DIR)/h5js_lib.js \
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
