# Detect emscripten
CROSS_FILE :=
EMMAKE :=
ifneq (,$(findstring emcc,$(CC)))
CROSS_FILE := --cross-file=$(PWD)/deps/mustach/emscripten.cross
EMMAKE := emmake
endif

results.mk:
	if ! [ -f v1.7.19.tar.gz ]; then wget https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.19.tar.gz; fi
	if [ -d cJSON-1.7.19.tar.gz ]; then rm -rf cJSON-1.7.19; fi
	tar xf v1.7.19.tar.gz
	cd cJSON-1.7.19 && $(EMMAKE) make -j1 all && $(EMMAKE) make -j1 install PREFIX=$(PWD)/deps/mustach/cJSON-1.7.19/install
	cd cJSON-1.7.19 && sh ../cjsonpkgcfg.sh
	if ! [ -f mustach-1.2.10.tar.gz ]; then wget https://gitlab.com/jobol/mustach/-/archive/1.2.10/mustach-1.2.10.tar.gz; fi
	if [ -d mustach-1.2.10 ]; then rm -rf mustach-1.2.10; fi
	tar xf mustach-1.2.10.tar.gz
	ln -sv ../cJSON-1.7.19/install mustach-1.2.10/install
	cd mustach-1.2.10 && PKG_CONFIG_PATH=$(PWD)/deps/mustach/cJSON-1.7.19/install/lib/pkgconfig meson setup build --prefix=$(PWD)/deps/mustach/cJSON-1.7.19/install --default-library=static -Dtool=none -Dcjson=enabled -Djsonc=disabled -Djansson=disabled $(CROSS_FILE)
	cd mustach-1.2.10 && ninja -C build libmustach-cjson.a libmustach.a libmustach-core.a
	touch results.mk
