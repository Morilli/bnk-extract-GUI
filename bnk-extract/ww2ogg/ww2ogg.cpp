#define __STDC_CONSTANT_MACROS
#include <cstring>
#include "wwriff.hpp"
#include "stdint.h"
#include "errors.hpp"
#include "../defs.h"
// #include "../general_utils.h"

using namespace std;

class ww2ogg_options
{
    AudioData* in_filedata;
    string in_filename;
    string out_filename;
    string codebooks_filename;
    bool inline_codebooks;
    bool full_setup;
    ForcePacketFormat force_packet_format;
public:
    ww2ogg_options(void) : in_filename(""),
                           out_filename(""),
                           codebooks_filename("packed_codebooks.bin"),
                           inline_codebooks(false),
                           full_setup(false),
                           force_packet_format(kNoForcePacketFormat)
      {}
    void parse_args(int argc, char **argv);
    const AudioData& get_in_filedata(void) const {return *in_filedata;}
    const string& get_in_filename(void) const {return in_filename;}
    const string& get_out_filename(void) const {return out_filename;}
    const string& get_codebooks_filename(void) const {return codebooks_filename;}
    bool get_inline_codebooks(void) const {return inline_codebooks;}
    bool get_full_setup(void) const {return full_setup;}
    ForcePacketFormat get_force_packet_format(void) const {return force_packet_format;}
};


extern "C" BinaryData* ww2ogg(int argc, char **argv)
{
    // cout << "Audiokinetic Wwise RIFF/RIFX Vorbis to Ogg Vorbis converter " VERSION " by hcs" << endl << endl;

    ww2ogg_options opt;

    BinaryData* ogg_data = (BinaryData*) calloc(1, sizeof(BinaryData));
    AudioData x;
    string y;
    ForcePacketFormat z;

    Wwise_RIFF_Vorbis ww(x, y, false, false, z);

    ww.generate_ogg(*ogg_data);
    return NULL;
}
