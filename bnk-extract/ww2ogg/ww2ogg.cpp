#define __STDC_CONSTANT_MACROS
#include <cstring>
#include "wwriff.hpp"
#include "stdint.h"
#include "errors.hpp"
#include "../defs.h"
#include "../general_utils.h"

using namespace std;

class ww2ogg_options
{
    BinaryData* in_filedata;
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
    const BinaryData& get_in_filedata(void) const {return *in_filedata;}
    const string& get_in_filename(void) const {return in_filename;}
    const string& get_out_filename(void) const {return out_filename;}
    const string& get_codebooks_filename(void) const {return codebooks_filename;}
    bool get_inline_codebooks(void) const {return inline_codebooks;}
    bool get_full_setup(void) const {return full_setup;}
    ForcePacketFormat get_force_packet_format(void) const {return force_packet_format;}
};

void usage(void)
{
    fprintf(stderr, "usage: ww2ogg input.wav [-o output.ogg] [--inline-codebooks] [--full-setup]\n"
            "                        [--mod-packets | --no-mod-packets]\n"
            "                        [--pcb packed_codebooks.bin]\n\n");
}

extern "C" BinaryData* ww2ogg(int argc, char **argv)
{
    // cout << "Audiokinetic Wwise RIFF/RIFX Vorbis to Ogg Vorbis converter " VERSION " by hcs" << endl << endl;

    ww2ogg_options opt;

    try
    {
        opt.parse_args(argc, argv);
    }
    catch (const Argument_error& ae)
    {
        ae.print(stderr);

        usage();
        return NULL;
    }

    BinaryData* ogg_data = (BinaryData*) calloc(1, sizeof(BinaryData));

    try {
        Wwise_RIFF_Vorbis ww(opt.get_in_filedata(),
            opt.get_codebooks_filename(),
            opt.get_inline_codebooks(),
            opt.get_full_setup(),
            opt.get_force_packet_format()
        );

        ww.generate_ogg(*ogg_data);
    } catch (const Parse_error& pe) {
        pe.print(stderr);
        free(ogg_data);
        return NULL;
    }

    return ogg_data;
}

void ww2ogg_options::parse_args(int argc, char ** argv)
{
    bool set_input = false, set_output = false;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--binarydata"))
        {
            if (i+1 >= argc)
            {
                throw Argument_error("--binarydata needs an option");
            }

            hex2bytes(argv[++i], &in_filedata, 16);
        }
        if (!strcmp(argv[i], "-o"))
        {
            // switch for output file name
            if (i+1 >= argc)
            {
                throw Argument_error("-o needs an option");
            }

            if (set_output)
            {
                throw Argument_error("only one output file at a time");
            }

            out_filename = argv[++i];
            set_output = true;
        }
        else if (!strcmp(argv[i], "--inline-codebooks"))
        {
            // switch for inline codebooks
            inline_codebooks = true;
        }
        else if (!strcmp(argv[i], "--full-setup"))
        {
            // early version with setup almost entirely intact
            full_setup = true;
            inline_codebooks = true;
        }
        else if (!strcmp(argv[i], "--mod-packets") || !strcmp(argv[i], "--no-mod-packets"))
        {
            if (force_packet_format != kNoForcePacketFormat)
            {
                throw Argument_error("only one of --mod-packets or --no-mod-packets is allowed");
            }

            if (!strcmp(argv[i], "--mod-packets"))
            {
              force_packet_format = kForceModPackets;
            }
            else
            {
              force_packet_format = kForceNoModPackets;
            }
        }
        else if (!strcmp(argv[i], "--pcb"))
        {
            // override default packed codebooks file
            if (i+1 >= argc)
            {
                throw Argument_error("--pcb needs an option");
            }

            codebooks_filename = argv[++i];
        }
        else
        {
            // assume anything else is an input file name
            if (set_input)
            {
                throw Argument_error("only one input file at a time");
            }

            in_filename = argv[i];
            set_input = true;
        }
    }

    if (!set_input)
    {
        throw Argument_error("input name not specified");
    }

    if (!set_output)
    {
        size_t found = in_filename.find_last_of('.');

        out_filename = in_filename.substr(0, found);
        out_filename.append(".ogg");

        // TODO: should be case insensitive for Windows
        if (out_filename == in_filename)
        {
            out_filename.append("_conv.ogg");
        }
    }
}
