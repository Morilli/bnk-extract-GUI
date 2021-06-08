
CFLAGS ?= -std=gnu18 -Wall -Wextra -O3 -flto
LDFLAGS ?= -O3 -flto -lcomctl32 -lUxTheme -lgdi32 -lComdlg32 -static bnk-extract.dll
target := gui.exe

all: $(target)


object_files := test.o templatewindow.o resource.res

test.o: resource.h templatewindow.h
resource.res: resource.rc resource.h icon.ico
	windres --output-format=coff resource.rc $@


$(target): $(object_files) bnk-extract.dll
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -f gui.exe resource.res test.o
