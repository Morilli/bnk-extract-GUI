
CFLAGS ?= -std=gnu18 -Wall -Wextra -Os -g -flto
LDFLAGS ?= -Os -flto
target := gui.exe

all: $(target)


object_files := test.o resource.res

test.o: resource.h
resource.res: resource.rc resource.h icon.ico
	windres --output-format=coff resource.rc $@


$(target): $(object_files)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -f gui.exe resource.res test.o