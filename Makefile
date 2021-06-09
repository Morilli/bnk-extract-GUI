
CFLAGS ?= -std=gnu18 -Wall -Wextra -Os -flto
LDFLAGS ?= -Os -flto -g -lcomctl32 -lUxTheme -lgdi32 -lComdlg32 -lWinmm -lvorbisfile -logg -lvorbis libbnk-extract.a -static -Wl,--gc-sections
target := gui.exe

all: $(target)


object_files := test.o utility.o templatewindow.o resource.res manifest.res

%.res : %.rc
	windres --output-format=coff $< $@
test.o: resource.h templatewindow.h
resource.res: resource.h icon.ico alpecin.wav
manifest.res: gui.exe.manifest


$(target): $(object_files) libbnk-extract.a
	$(CXX) $(CFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -f gui.exe resource.res test.o
