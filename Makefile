
CFLAGS ?= -std=gnu18 -Wall -Wextra -O3 -flto
LDFLAGS ?= -O3 -flto -lcomctl32 -lUxTheme -lgdi32 -lComdlg32 -static bnk-extract.dll
target := gui.exe

all: $(target)


object_files := test.o templatewindow.o resource.res manifest.res

%.res : %.rc
	windres --output-format=coff $< $@
test.o: resource.h templatewindow.h
resource.res: resource.h icon.ico
manifest.res: gui.exe.manifest


$(target): $(object_files) bnk-extract.dll
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -f gui.exe resource.res test.o
