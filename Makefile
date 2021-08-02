NATIVE_LIBDIR=native_lib
NATIVE_LIBS = $(NATIVE_LIBDIR)/libhdf5.a $(NATIVE_LIBDIR)/libhdf5_hl.a $(NATIVE_LIBDIR)/libhdf5_cpp.a
NATIVE_HELPERS = $(NATIVE_LIBDIR)/H5detect $(NATIVE_LIBDIR)/H5make_libsettings
NATIVE_BUILDDIR=native_build
WASM_BUILDDIR = wasm_build
WASM_LIBS = $(WASM_BUILDDIR)/src/.libs/libhdf5.a $(WASM_BUILDDIR)/c++/src/.libs/libhdf5_cpp.a $(WASM_BUILDDIR)/hl/src/.libs/libhdf5_hl.a
SRC=libhdf5
CONFIGURE=$(SRC)/configure

all: $(NATIVE_LIBS) $(WASM_LIBS)

$(CONFIGURE): $(SRC)/autogen.sh
	cd $(SRC) && ./autogen.sh;

$(NATIVE_LIBS) $(NATIVE_HELPERS): $(CONFIGURE)
	mkdir -p $(NATIVE_BUILDDIR);
	mkdir -p $(NATIVE_LIBDIR);
	cd $(NATIVE_BUILD) \
          && ../$(CONFIGURE) LIBS=-lz \
                      --disable-tests \
                      --enable-cxx \
                      --enable-build-mode=production \
                      --disable-tools \
                      --disable-shared \
                      --disable-deprecated-symbols;
	cd $(NATIVE_BUILDDIR) && make -j8;

	cp $(NATIVE_BUILDDIR)/src/.libs/*.a $(NATIVE_LIBDIR)/;
	cp $(NATIVE_BUILDDIR)/c++/src/.libs/*.a $(NATIVE_LIBDIR)/;
	cp $(NATIVE_BUILDDIR)/hl/src/.libs/*.a $(NATIVE_LIBDIR)/;
	cp $(NATIVE_BUILDDIR)/src/H5detect $(NATIVE_LIBDIR)/;
	cp $(NATIVE_BUILDDIR)/src/H5make_libsettings $(NATIVE_LIBDIR)/;

$(WASM_LIBS): $(NATIVE_LIBS) $(NATIVE_HELPERS)
	mkdir -p $(WASM_BUILDDIR)/src;
	cd $(WASM_BUILDDIR) \
          && emconfigure ../$(CONFIGURE) LIBS=-lz \
                      --disable-tests \
                      --enable-cxx \
                      --enable-build-mode=production \
                      --disable-tools \
                      --disable-shared \
                      --disable-deprecated-symbols;
	cp $(NATIVE_LIBDIR)/H5detect $(WASM_BUILDDIR)/src/ && chmod a+x $(WASM_BUILDDIR)/src/H5detect;
	cp $(NATIVE_LIBDIR)/H5make_libsettings $(WASM_BUILDDIR)/src/ && chmod a+x $(WASM_BUILDDIR)/src/H5make_libsettings;
	cd $(WASM_BUILDDIR) && emmake make -j8;

