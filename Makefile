
CFLAGS ?= -std=gnu18 -Wall -Wextra -Os -flto
CXXFLAGS ?= -std=c++17 -Wall -Wextra
LDFLAGS ?= -Os -flto -g -lole32 -luuid -lcomctl32 -lgdi32 -lComdlg32 -lWinmm -static -Wl,--gc-sections
target := gui.exe

all: $(target)
strip: LDFLAGS := $(LDFLAGS) -s
strip: all

libs := libs/libbnk-extract.a libs/libvorbis.a libs/libogg.a libs/libvorbisfile.a
object_files := test.o utility.o templatewindow.o IDropTarget.o resource.res manifest.res

%.res : %.rc
	windres --output-format=coff $< $@
test.o: resource.h templatewindow.h utility.h
utility.o: utility.h
IDropTarget.o: IDropTarget.h
resource.res: resource.h icon.ico
manifest.res: gui.exe.manifest


$(target): $(object_files) $(libs)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -f gui.exe resource.res test.o
