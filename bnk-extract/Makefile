CFLAGS := -std=gnu18 -Wall -Wextra -pedantic -Os -flto -I../libogg-1.3.5/include -I../libvorbis-1.3.7/include
CXXFLAGS := -std=c++17 -Wall -Wextra -Wno-unused-function -Wno-missing-field-initializers -Os -flto
target := libbnk-extract.a

all: $(target)

sound_OBJECTS=general_utils.o extract.o

general_utils.o: general_utils.h defs.h
bin.o: bin.h defs.h list.h
bnk.o: bin.h defs.h extract.h static_list.h
extract.c: defs.h general_utils.h
wpk.o: bin.h defs.h extract.h static_list.h
sound.o: bin.h bnk.h defs.h wpk.h

BIT_STREAM_HEADERS=ww2ogg/Bit_stream.hpp ww2ogg/crc.h ww2ogg/errors.hpp
WWRIFF_HEADERS=ww2ogg/wwriff.hpp $(BIT_STREAM_HEADERS)

ww2ogg_OBJECTS=ww2ogg/ww2ogg.o ww2ogg/wwriff.o ww2ogg/codebook.o ww2ogg/crc.o

ww2ogg/ww2ogg.o: $(WWRIFF_HEADERS) defs.h general_utils.h
# ww2ogg/wwriff.o: CXXFLAGS:=$(CXXFLAGS) -fno-lto # necessary because of some lto bug?
ww2ogg/wwriff.o: ww2ogg/codebook.hpp $(WWRIFF_HEADERS) defs.h
ww2ogg/codebook.o: ww2ogg/codebook.hpp $(BIT_STREAM_HEADERS) defs.h
ww2ogg/crc.o: ww2ogg/crc.h

revorb_OBJECTS=revorb/revorb.o
revorb/revorb.o: defs.h general_utils.h list.h

$(target): $(ww2ogg_OBJECTS) $(sound_OBJECTS)
	$(AR) -rcs $@ $^


clean:
	rm -f libbnk-extract.a $(sound_OBJECTS) $(ww2ogg_OBJECTS) $(revorb_OBJECTS)
