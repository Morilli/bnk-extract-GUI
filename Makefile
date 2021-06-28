CFLAGS ?= -std=gnu18 -Wall -Wextra -Os -flto -Ilibogg-1.3.5/include -Ilibvorbis-1.3.7/include
LDFLAGS ?= -Os -flto -g -lole32 -luuid -lcomctl32 -lgdi32 -lComdlg32 -lWinmm -static -Wl,--gc-sections
target := gui.exe

all: $(target)
strip: LDFLAGS := $(LDFLAGS) -s
strip: all

.prerequisites_built:
ifeq ($(wildcard ./.prerequisites_built),)
	cd libogg-1.3.5 && cmake --build build > /dev/null 2>&1 || (rm -rf build && mkdir build && cd build && \
	cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="-Os -flto" && \
	cmake --build . && cd ..) && \
	mv build/libogg.a ../libs

	cd libvorbis-1.3.7 && cmake --build build > /dev/null 2>&1 || (rm -rf build && mkdir build && cd build && \
	cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGG_INCLUDE_DIR="../../libogg-1.3.5/include" -DOGG_LIBRARY="../../libs/libogg.a" -DCMAKE_C_FLAGS_RELEASE="-Os -flto" && \
	cmake --build . && cd ..) && \
	mv build/lib/libvorbis.a ../libs && mv build/lib/libvorbisfile.a ../libs

	$(MAKE) -C bnk-extract && \
	mv bnk-extract/libbnk-extract.a libs

	touch .prerequisites_built
endif

libs := libs/libbnk-extract.a libs/libvorbis.a libs/libogg.a libs/libvorbisfile.a
object_files := gui.o utility.o IDropTarget.o settings.o resource.res manifest.res

%.res : %.rc
	windres --output-format=coff $< $@
gui.o: IDropTarget.h resource.h utility.h
utility.o: utility.h settings.h
settings.o: settings.h resource.h
IDropTarget.o: utility.h
resource.res: resource.h icon.ico
manifest.res: gui.exe.manifest


$(target): $(object_files) .prerequisites_built
	$(CXX) $(CXXFLAGS) $^ $(libs) $(LDFLAGS) -o $@


clean:
	rm -f $(target) $(object_files)

clean-all: clean
	rm -f .prerequisites_built $(libs)
	rm -rf libogg-1.3.5/build
	rm -rf libvorbis-1.3.7/build
	$(MAKE) -C bnk-extract clean
