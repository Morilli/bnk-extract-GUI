CFLAGS := -Wall -Wextra -g
CXXFLAGS := -Wall -Wextra -Wno-unused-function
target := testshit

ifeq ($(OS),Windows_NT)
    target := $(target).exe
endif

all: $(target)

objects := main.o
libs := ../libs/libbnk-extract.a ../libs/libvorbis.a ../libs/libogg.a

$(target): $(objects)
	$(LINK.cpp) $^ $(libs) -o $@


clean:
	rm -f $(target) $(objects)
