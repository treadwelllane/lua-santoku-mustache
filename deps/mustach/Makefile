# Detect emscripten
EMMAKE :=
ifneq (,$(findstring emcc,$(CC)))
EMMAKE := emmake
endif

MUSTACH_CFLAGS := -fPIC -Wall -Wextra -DVERSION=1.2.10
CJSON_INSTALL := $(PWD)/deps/mustach/cJSON-1.7.19/install

results.mk:
	if ! [ -f v1.7.19.tar.gz ]; then wget https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.19.tar.gz; fi
	if [ -d cJSON-1.7.19.tar.gz ]; then rm -rf cJSON-1.7.19; fi
	tar xf v1.7.19.tar.gz
	cd cJSON-1.7.19 && $(EMMAKE) make -j1 all && $(EMMAKE) make -j1 install PREFIX=$(CJSON_INSTALL)
	cd cJSON-1.7.19 && sh ../cjsonpkgcfg.sh
	if ! [ -f mustach-1.2.10.tar.gz ]; then wget https://gitlab.com/jobol/mustach/-/archive/1.2.10/mustach-1.2.10.tar.gz; fi
	if [ -d mustach-1.2.10 ]; then rm -rf mustach-1.2.10; fi
	tar xf mustach-1.2.10.tar.gz
	cd mustach-1.2.10 && $(CC) -c $(MUSTACH_CFLAGS) $(CFLAGS) -o mustach.o mustach.c
	cd mustach-1.2.10 && $(CC) -c $(MUSTACH_CFLAGS) $(CFLAGS) -o mustach-wrap.o mustach-wrap.c
	cd mustach-1.2.10 && $(CC) -c $(MUSTACH_CFLAGS) $(CFLAGS) -I$(CJSON_INSTALL)/include -o mustach-cjson.o mustach-cjson.c
	cd mustach-1.2.10 && $(AR) rcs libmustach-core.a mustach.o mustach-wrap.o
	cd mustach-1.2.10 && $(AR) rcs libmustach-cjson.a mustach.o mustach-wrap.o mustach-cjson.o
	cd mustach-1.2.10 && $(AR) rcs libmustach.a mustach.o mustach-wrap.o mustach-cjson.o
	touch results.mk
