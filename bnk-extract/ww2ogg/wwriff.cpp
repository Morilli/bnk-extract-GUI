#define __STDC_CONSTANT_MACROS
#include <cstring>
#include "stdint.h"
#include "errors.hpp"
#include "wwriff.hpp"
#include "Bit_stream.hpp"
#include "codebook.hpp"
// #include "codebook.bin"
#include "../defs.h"

using namespace std;

/* Modern 2 or 6 byte header */
class Packet
{
    long _offset;
    uint16_t _size;
    uint32_t _absolute_granule;
    bool _no_granule;
public:
    Packet(const AudioData& ad, long o, bool little_endian, bool no_granule = false) : _offset(o), _size(-1), _absolute_granule(0), _no_granule(no_granule) {
        // i.seekg(_offset);

        if (little_endian)
        {
            _size = read_16_le(&ad.data[_offset]);
            if (!_no_granule)
            {
                _absolute_granule = read_32_le(&ad.data[_offset + 2]);
            }
        }
        else
        {
            _size = read_16_be(&ad.data[_offset]);
            if (!_no_granule)
            {
                _absolute_granule = read_32_be(&ad.data[_offset + 2]);
            }
        }
    }

    long header_size(void) { return _no_granule?2:6; }
    long offset(void) { return _offset + header_size(); }
    uint16_t size(void) { return _size; }
    uint32_t granule(void) { return _absolute_granule; }
    long next_offset(void) { return _offset + header_size() + _size; }
};

/* Old 8 byte header */
class Packet_8
{
    long _offset;
    uint32_t _size;
    uint32_t _absolute_granule;
public:
    Packet_8(const AudioData& ad, long o, bool little_endian) : _offset(o), _size(-1), _absolute_granule(0) {
        if (little_endian)
        {
            _size = read_32_le(&ad.data[_offset]);
            _absolute_granule = read_32_le(&ad.data[_offset + 4]);
        }
        else
        {
            _size = read_32_be(&ad.data[_offset]);
            _absolute_granule = read_32_be(&ad.data[_offset + 4]);
        }
    }

    long header_size(void) { return 8; }
    long offset(void) { return _offset + header_size(); }
    uint32_t size(void) { return _size; }
    uint32_t granule(void) { return _absolute_granule; }
    long next_offset(void) { return _offset + header_size() + _size; }
};

class Vorbis_packet_header
{
    uint8_t type;

    static const char vorbis_str[6];

public:
    explicit Vorbis_packet_header(uint8_t t) : type(t) {}

    friend Bit_oggstream& operator << (Bit_oggstream& bstream, const Vorbis_packet_header& vph) {
        Bit_uint<8> t(vph.type);
        bstream << t;

        for ( unsigned int i = 0; i < 6; i++ )
        {
            Bit_uint<8> c(vorbis_str[i]);
            bstream << c;
        }

        return bstream;
    }
};

const char Vorbis_packet_header::vorbis_str[6] = {'v','o','r','b','i','s'};

Wwise_RIFF_Vorbis::Wwise_RIFF_Vorbis(
    const AudioData& indata,
    const string& codebooks_name,
    bool inline_codebooks,
    bool full_setup,
    ForcePacketFormat force_packet_format
    )
  :
    _infile_data(indata),
    _codebooks_name(codebooks_name),
    _little_endian(true),
    _is_wav(false),
    _fmt_offset(-1),
    _cue_offset(-1),
    _LIST_offset(-1),
    _smpl_offset(-1),
    _vorb_offset(-1),
    _data_offset(-1),
    _fmt_size(-1),
    _cue_size(-1),
    _LIST_size(-1),
    _smpl_size(-1),
    _vorb_size(-1),
    _data_size(-1),
    _ext_unk(0),
    _subtype(0),
    _cue_count(0),
    _loop_count(0),
    _loop_start(0),
    _loop_end(0),
    _setup_packet_offset(0),
    _first_audio_packet_offset(0),
    _uid(0),
    _blocksize_0_pow(0),
    _blocksize_1_pow(0),
    _inline_codebooks(inline_codebooks),
    _full_setup(full_setup),
    _header_triad_present(false),
    _old_packet_headers(false),
    _no_granule(false),
    _mod_packets(false),
    _read_16(NULL),
    _read_32(NULL)
{
    // check RIFF header
    {
        unsigned char riff_head[4], wave_head[4];
        memcpy(riff_head, _infile_data.data, 4);

        if (memcmp(&riff_head[0],"RIFX",4))
        {
            if (memcmp(&riff_head[0],"RIFF",4))
            {
                throw Parse_error_str("missing RIFF");
            }
            else
            {
                _little_endian = true;
            }
        }
        else
        {
            _little_endian = false;
        }

        if (_little_endian)
        {
            _read_16 = read_16_le;
            _read_32 = read_32_le;
        }
        else
        {
            _read_16 = read_16_be;
            _read_32 = read_32_be;
        }

        _riff_size = _read_32(&_infile_data.data[4]) + 8;

        if (_riff_size > _infile_data.length) {
            v_printf(1, ".?\" ");
            throw Parse_error_str("RIFF truncated");
        }

        memcpy(wave_head, &_infile_data.data[8], 4);
        if (memcmp(&wave_head[0],"WAVE",4)) throw Parse_error_str("missing WAVE");
    }

    // read chunks
    uint32_t chunk_offset = 12;
    while (chunk_offset < _riff_size)
    {

        if (chunk_offset + 8 > _riff_size) throw Parse_error_str("chunk header truncated");

        char chunk_type[4];
        memcpy(chunk_type, &_infile_data.data[chunk_offset], 4);
        uint32_t chunk_size;

        chunk_size = _read_32(&_infile_data.data[chunk_offset + 4]);

        if (!memcmp(chunk_type,"fmt ",4))
        {
            _fmt_offset = chunk_offset + 8;
            _fmt_size = chunk_size;
        }
        else if (!memcmp(chunk_type,"cue ",4))
        {
            _cue_offset = chunk_offset + 8;
            _cue_size = chunk_size;
        }
        else if (!memcmp(chunk_type,"LIST",4))
        {
            _LIST_offset = chunk_offset + 8;
            _LIST_size = chunk_size;
        }
        else if (!memcmp(chunk_type,"smpl",4))
        {
            _smpl_offset = chunk_offset + 8;
            _smpl_size = chunk_size;
        }
        else if (!memcmp(chunk_type,"vorb",4))
        {
            _vorb_offset = chunk_offset + 8;
            _vorb_size = chunk_size;
        }
        else if (!memcmp(chunk_type,"data",4))
        {
            _data_offset = chunk_offset + 8;
            _data_size = chunk_size;
        }

        chunk_offset = chunk_offset + 8 + chunk_size;
    }

    if (chunk_offset > _riff_size) throw Parse_error_str("chunk truncated");

    // check that we have the chunks we're expecting
    if (-1 == _fmt_offset && -1 == _data_offset) throw Parse_error_str("expected fmt, data chunks");

    // read fmt
    if (_vorb_offset == -1) {
        if (_fmt_size == 0x18) { // wav data
            _is_wav = true;
            v_printf(1, ".wav\"\n");
        } else if (_fmt_size == 0x42) { // ogg data
            // fake it out
            _vorb_offset = _fmt_offset + 0x18;
            v_printf(1, ".ogg\"\n");
        } else {
            throw Parse_error_str("expected fmt_size of 0x18 or 0x42 if vorb section missing");
        }
    } else if (_fmt_size != 0x28 && _fmt_size != 0x18 && _fmt_size != 0x12) {
        throw Parse_error_str("bad fmt size");
    }

    uint16_t codec_id = _read_16(&_infile_data.data[_fmt_offset]);
    if ((_is_wav && codec_id != 0xFFFE) || (!_is_wav && codec_id != 0xFFFF)) throw Parse_error_str("bad codec id");
    _channels = _read_16(&_infile_data.data[_fmt_offset + 2]);
    _sample_rate = _read_32(&_infile_data.data[_fmt_offset + 4]);
    _avg_bytes_per_second = _read_32(&_infile_data.data[_fmt_offset + 8]);
    _block_align = _read_16(&_infile_data.data[_fmt_offset + 12]);
    _bits_per_sample = _read_16(&_infile_data.data[_fmt_offset + 14]);
    if (_fmt_size-0x12 != _read_16(&_infile_data.data[_fmt_offset + 16])) throw Parse_error_str("bad extra fmt length");

    if (_fmt_size-0x12 >= 2) {
      // read extra fmt
      _ext_unk = _read_16(&_infile_data.data[_fmt_offset + 18]);
      if (_fmt_size-0x12 >= 6) {
        _subtype = _read_32(&_infile_data.data[_fmt_offset + 20]);
      }
    }

    if (_fmt_size == 0x28)
    {
        char whoknowsbuf[16];
        const unsigned char whoknowsbuf_check[16] = {1,0,0,0, 0,0,0x10,0, 0x80,0,0,0xAA, 0,0x38,0x9b,0x71};
        memcpy(whoknowsbuf, &_infile_data.data[_fmt_offset + 24], 16);
        if (memcmp(whoknowsbuf, whoknowsbuf_check, 16)) throw Parse_error_str("expected signature in extra fmt?");
    }

    if (_is_wav) return;

    // read cue
    if (-1 != _cue_offset)
    {
#if 0
        if (0x1c != _cue_size) throw Parse_error_str("bad cue size");
#endif
        _cue_count = _read_32(&_infile_data.data[_cue_offset]);
    }

    // read LIST
    if (-1 != _LIST_offset)
    {
#if 0
        if ( 4 != _LIST_size ) throw Parse_error_str("bad LIST size");
        char adtlbuf[4];
        const char adtlbuf_check[4] = {'a','d','t','l'};
        _infile.seekg(_LIST_offset);
        _infile.read(adtlbuf, 4);
        if (memcmp(adtlbuf, adtlbuf_check, 4)) throw Parse_error_str("expected only adtl in LIST");
#endif
    }

    // read smpl
    if (-1 != _smpl_offset)
    {
        _loop_count = _read_32(&_infile_data.data[_smpl_offset + 0x1C]);

        if (1 != _loop_count) throw Parse_error_str("expected one loop");

        _loop_start = _read_32(&_infile_data.data[_smpl_offset + 0x2C]);
        _loop_end = _read_32(&_infile_data.data[_smpl_offset + 0x30]);
    }

    // read vorb
    switch (_vorb_size)
    {
        case -1:
        case 0x28:
        case 0x2A:
        case 0x2C:
        case 0x32:
        case 0x34:
            break;

        default:
            throw Parse_error_str("bad vorb size");
            break;
    }

    _sample_count = _read_32(&_infile_data.data[_vorb_offset]);

    int file_pos;
    switch (_vorb_size)
    {
        case -1:
        case 0x2A:
        {
            _no_granule = true;

            // _infile.seekg(_vorb_offset + 0x4, ios::beg);
            uint32_t mod_signal = _read_32(&_infile_data.data[_vorb_offset + 0x4]);

            // set
            // D9     11011001
            // CB     11001011
            // BC     10111100
            // B2     10110010
            // unset
            // 4A     01001010
            // 4B     01001011
            // 69     01101001
            // 70     01110000
            // A7     10100111 !!!

            // seems to be 0xD9 when _mod_packets should be set
            // also seen 0xCB, 0xBC, 0xB2
            if (0x4A != mod_signal && 0x4B != mod_signal && 0x69 != mod_signal && 0x70 != mod_signal)
            {
                _mod_packets = true;
            }
            file_pos = _vorb_offset + 0x10;
            break;
        }

        default:
            file_pos = _vorb_offset + 0x18;
            break;
    }

    if (force_packet_format == kForceNoModPackets)
    {
        _mod_packets = false;
    }
    else if (force_packet_format == kForceModPackets)
    {
        _mod_packets = true;
    }

    _setup_packet_offset = _read_32(&_infile_data.data[file_pos]);
    _first_audio_packet_offset = _read_32(&_infile_data.data[file_pos + 4]);

    switch (_vorb_size)
    {
        case -1:
        case 0x2A:
            file_pos = _vorb_offset + 0x24;
            break;

        case 0x32:
        case 0x34:
            file_pos = _vorb_offset + 0x2C;
            break;
    }

    switch(_vorb_size)
    {
        case 0x28:
        case 0x2C:
            // ok to leave _uid, _blocksize_0_pow and _blocksize_1_pow unset
            _header_triad_present = true;
            _old_packet_headers = true;
            break;

        case -1:
        case 0x2A:
        case 0x32:
        case 0x34:
            _uid = _read_32(&_infile_data.data[file_pos]);
            _blocksize_0_pow = _infile_data.data[file_pos + 4];
            _blocksize_1_pow = _infile_data.data[file_pos + 5];
            break;
    }

    // check/set loops now that we know total sample count
    if (0 != _loop_count)
    {
        if (_loop_end == 0)
        {
            _loop_end = _sample_count;
        }
        else
        {
            _loop_end = _loop_end + 1;
        }

        if (_loop_start >= _sample_count || _loop_end > _sample_count || _loop_start > _loop_end)
            throw Parse_error_str("loops out of range");
    }

    // check subtype now that we know the vorb info
    // this is clearly just the channel layout
    switch (_subtype)
    {
        case 4:     /* 1 channel, no seek table */
        case 3:     /* 2 channels */
        case 0x33:  /* 4 channels */
        case 0x37:  /* 5 channels, seek or not */
        case 0x3b:  /* 5 channels, no seek table */
        case 0x3f:  /* 6 channels, no seek table */
            break;
        default:
            //throw Parse_error_str("unknown subtype");
            break;
    }
}

void Wwise_RIFF_Vorbis::print_info(void)
{
    printf("%s %d channel%s %u HZ %u bps %u samples\n",
           _little_endian ? "RIFF WAVE" : "RIFX WAVE",
           _channels,
           _channels != 1 ? "s" : "",
           _sample_rate,
            _avg_bytes_per_second*8,
           _sample_count);

    if (0 != _loop_count)
    {
        printf("loop from %u to %u\n", _loop_start, _loop_end);
    }

    if (_old_packet_headers)
    {
        puts("- 8 byte (old) packet headers");
    }
    else if (_no_granule)
    {
        puts("- 2 byte packet headers, no granule");
    }
    else
    {
        puts("- 6 byte packet headers");
    }

    if (_header_triad_present)
    {
        puts("- Vorbis header triad present");
    }

    if (_full_setup || _header_triad_present)
    {
        puts("- full setup header");
    }
    else
    {
        puts("- stripped setup header");
    }

    if (_inline_codebooks || _header_triad_present)
    {
        puts("- inline codebooks");
    }
    else
    {
        printf("- external codebooks (%s)\n", _codebooks_name.c_str());
    }

    if (_mod_packets)
    {
        puts("- modified Vorbis packets");
    }
    else
    {
        puts("- standard Vorbis packets");
    }

#if 0
    if (0 != _cue_count)
    {
        printf("%u cue point%s\n", _cue_count, _cue_count != 1 ? "s" : "");
    }
#endif
}

void Wwise_RIFF_Vorbis::generate_ogg_header(Bit_oggstream& os)
{
    // generate comment packet
    {
        Vorbis_packet_header vhead(3);

        os << vhead;

        static const char vendor[] = "converted from Audiokinetic Wwise by ww2ogg " VERSION;
        Bit_uint<32> vendor_size(strlen(vendor));

        os << vendor_size;
        for (unsigned int i = 0; i < vendor_size; i ++) {
            Bit_uint<8> c(vendor[i]);
            os << c;
        }

            // two comments, loop start and end
            // Bit_uint<32> user_comment_count(2);
            // os << user_comment_count;

            string loop_start_str = std::to_string(rand());
            Bit_uint<32> loop_start_comment_length;
            loop_start_comment_length = rand();// loop_start_str.size();
            os << loop_start_comment_length;
            for (unsigned int i = 0; i < loop_start_comment_length; i++)
            {
                Bit_uint<8> c(loop_start_str[i]);
                os << c;
            }

            string loop_end_str = "LoopEnd=" + std::to_string(_loop_end);
            Bit_uint<32> loop_end_comment_length;
            loop_end_comment_length = loop_end_str.size();
            os << loop_end_comment_length;
            for (unsigned int i = 0; i < loop_end_comment_length; i++)
            {
                Bit_uint<8> c(loop_end_str[2]);
                os << c;
            }

        Bit_uint<1> framing(1);
        os << framing;

        //os.flush_bits();
        os.flush_page();
    }

    // generate setup packet
    {
        Vorbis_packet_header vhead(5);

        os << vhead;

        Packet setup_packet(_infile_data, _data_offset + _setup_packet_offset, _little_endian, _no_granule);

        if (setup_packet.granule() != 0) throw Parse_error_str("setup packet granule != 0");
        Bit_stream ss(_infile_data, setup_packet.offset());

        // codebook count
        Bit_uint<8> codebook_count_less1;
        ss >> codebook_count_less1;
        unsigned int codebook_count = codebook_count_less1 + 1;
        os << codebook_count_less1;

        //cout << codebook_count << " codebooks" << endl;

        // rebuild codebooks
        if (_inline_codebooks)
        {
            codebook_library cbl;

            for (unsigned int i = 0; i < codebook_count; i++)
            {
                if (_full_setup)
                {
                    cbl.copy(ss, os);
                }
                else
                {
                    cbl.rebuild(ss, 0, os);
                }
            }
        }
        else
        {
            /* external codebooks */

            // codebook_library cbl((unsigned char*) main_codebook, 74387);
            codebook_library cbl(NULL, 0);

            for (unsigned int i = 0; i < codebook_count; i++)
            {
                Bit_uint<10> codebook_id;
                ss >> codebook_id;
                //cout << "Codebook " << i << " = " << codebook_id << endl;
                try
                {
                    cbl.rebuild(codebook_id, os);
                }
                catch (const Invalid_id & e)
                {
                    //         B         C         V
                    //    4    2    4    3    5    6
                    // 0100 0010 0100 0011 0101 0110
                    // \_______|____ ___|/
                    //              X
                    //            11 0100 0010

                    if (codebook_id == 0x342)
                    {
                        Bit_uint<14> codebook_identifier;
                        ss >> codebook_identifier;

                        //         B         C         V
                        //    4    2    4    3    5    6
                        // 0100 0010 0100 0011 0101 0110
                        //           \_____|_ _|_______/
                        //                   X
                        //         01 0101 10 01 0000
                        if (codebook_identifier == 0x1590)
                        {
                            // starts with BCV, probably --full-setup
                            throw Parse_error_str(
                                "invalid codebook id 0x342, try --full-setup");
                        }
                    }

                    // just an invalid codebook
                    throw;
                }
            }
        }

        // Time Domain transforms (placeholder)
        // Bit_uint<6> time_count_less1(0);
        // os << time_count_less1;
        Bit_uint<16> dummy_time_value(0);
        os << dummy_time_value;

            // floor count
            Bit_uint<6> floor_count_less1;
            ss >> floor_count_less1;
            unsigned int floor_count = floor_count_less1 + 1;
            os << floor_count_less1;

            // rebuild floors
            for (unsigned int i = 0; i < floor_count; i++)
            {
                // Always floor type 1
                // Bit_uint<16> floor_type(1);
                // os << floor_type;

                Bit_uint<5> floor1_partitions;
                ss >> floor1_partitions;
                os << floor1_partitions;

                unsigned int * floor1_partition_class_list = new unsigned int [floor1_partitions];

                unsigned int maximum_class = 0;
                for (unsigned int j = 0; j < floor1_partitions; j++)
                {
                    Bit_uint<4> floor1_partition_class;
                    ss >> floor1_partition_class;
                    os << floor1_partition_class;

                    floor1_partition_class_list[j] = floor1_partition_class;

                    if (floor1_partition_class > maximum_class)
                        maximum_class = floor1_partition_class;
                }

                unsigned int * floor1_class_dimensions_list = new unsigned int [maximum_class+1];

                for (unsigned int j = 0; j <= maximum_class; j++)
                {
                    Bit_uint<3> class_dimensions_less1;
                    ss >> class_dimensions_less1;
                    os << class_dimensions_less1;

                    floor1_class_dimensions_list[j] = class_dimensions_less1 + 1;

                    Bit_uint<2> class_subclasses;
                    ss >> class_subclasses;
                    os << class_subclasses;

                    if (0 != class_subclasses)
                    {
                        Bit_uint<8> masterbook;
                        ss >> masterbook;
                        os << masterbook;

                        if (masterbook >= codebook_count)
                            throw Parse_error_str("invalid floor1 masterbook");
                    }

                    for (unsigned int k = 0; k < (1U<<class_subclasses); k++)
                    {
                        Bit_uint<8> subclass_book_plus1;
                        ss >> subclass_book_plus1;
                        os << subclass_book_plus1;

                        int subclass_book = static_cast<int>(subclass_book_plus1)-1;
                        if (subclass_book >= 0 && static_cast<unsigned int>(subclass_book) >= codebook_count)
                            throw Parse_error_str("invalid floor1 subclass book");
                    }
                }

                // Bit_uint<2> floor1_multiplier_less1;
                // ss >> floor1_multiplier_less1;
                // os << floor1_multiplier_less1;

                Bit_uint<2> rangebits;
                ss >> rangebits;
                os << rangebits;

                // for (unsigned int j = 0; j < floor1_partitions; j++)
                // {
                //     unsigned int current_class_number = floor1_partition_class_list[j];
                //     for (unsigned int k = 0; k < floor1_class_dimensions_list[current_class_number]; k++)
                //     {
                //         // Bit_uintv X(rangebits);
                //         // ss >> X;
                //         // os << X;
                //     }
                // }

                // delete [] floor1_class_dimensions_list;
                // delete [] floor1_partition_class_list;
            }

            // residue count
            Bit_uint<6> residue_count_less1;
            ss >> residue_count_less1;
            unsigned int residue_count = residue_count_less1 + 1;
            os << residue_count_less1;

            // rebuild residues
            for (unsigned int i = 0; i < residue_count; i++)
            {
                Bit_uint<2> residue_type;
                ss >> residue_type;
                os << Bit_uint<16>(residue_type);

                if (residue_type > 2) throw Parse_error_str("invalid residue type");

                Bit_uint<24> residue_begin, residue_end, residue_partition_size_less1;
                Bit_uint<6> residue_classifications_less1;
                Bit_uint<8> residue_classbook;

                ss >> residue_begin >> residue_end >> residue_partition_size_less1 >> residue_classifications_less1 >> residue_classbook;
                unsigned int residue_classifications = residue_classifications_less1 + 1;
                os << residue_begin << residue_end << residue_partition_size_less1 << residue_classifications_less1 << residue_classbook;

                if (residue_classbook >= codebook_count) throw Parse_error_str("invalid residue classbook");

                unsigned int * residue_cascade = new unsigned int [residue_classifications];

                for (unsigned int j = 0; j < residue_classifications; j++)
                {
                    Bit_uint<5> high_bits(0);
                    Bit_uint<3> low_bits;

                    ss >> low_bits;
                    os << low_bits;

                    // Bit_uint<1> bitflag;
                    // ss >> bitflag;
                    // os << bitflag;
                    // if (bitflag)
                    // {
                    //     ss >> high_bits;
                    //     os << high_bits;
                    // }

                    residue_cascade[j] = high_bits * 8 + low_bits;
                }

                for (unsigned int j = 0; j < residue_classifications; j++)
                {
                    for (unsigned int k = 0; k < 8; k++)
                    {
                        if (residue_cascade[j] & (1 << k))
                        {
                            Bit_uint<8> residue_book;
                            ss >> residue_book;
                            os << residue_book;

                            if (residue_book >= codebook_count) throw Parse_error_str("invalid residue book");
                        }
                    }
                }

                delete [] residue_cascade;
            }

            // mapping count
            Bit_uint<6> mapping_count_less1;
            ss >> mapping_count_less1;
            unsigned int mapping_count = mapping_count_less1 + 1;
            os << mapping_count_less1;

            for (unsigned int i = 0; i < mapping_count; i++)
            {
                // always mapping type 0, the only one
                // Bit_uint<16> mapping_type(0);

                // os << mapping_type;

                Bit_uint<1> submaps_flag;
                ss >> submaps_flag;
                os << submaps_flag;

                unsigned int submaps = 1;
                if (submaps_flag)
                {
                    Bit_uint<4> submaps_less1;

                    ss >> submaps_less1;
                    submaps = submaps_less1 + 1;
                    os << submaps_less1;
                }

                // Bit_uint<1> square_polar_flag;
                // ss >> square_polar_flag;
                // os << square_polar_flag;

                // if (square_polar_flag)
                // {
                //     Bit_uint<8> coupling_steps_less1;
                //     ss >> coupling_steps_less1;
                //     unsigned int coupling_steps = coupling_steps_less1 + 1;
                //     os << coupling_steps_less1;

                //     for (unsigned int j = 0; j < coupling_steps; j++)
                //     {
                //         Bit_uintv magnitude(ilog(_channels-1)), angle(ilog(_channels-1));

                //         ss >> magnitude >> angle;
                //         os << magnitude << angle;

                //         if (angle == magnitude || magnitude >= _channels || angle >= _channels) throw Parse_error_str("invalid coupling");
                //     }
                // }

                // a rare reserved field not removed by Ak!
                Bit_uint<2> mapping_reserved;
                ss >> mapping_reserved;
                os << mapping_reserved;
                if (0 != mapping_reserved) throw Parse_error_str("mapping reserved field nonzero");

                if (submaps > 1)
                {
                    for (unsigned int j = 0; j < _channels; j++)
                    {
                        Bit_uint<4> mapping_mux;
                        ss >> mapping_mux;
                        os << mapping_mux;

                        if (mapping_mux >= submaps) throw Parse_error_str("mapping_mux >= submaps");
                    }
                }

                // for (unsigned int j = 0; j < submaps; j++)
                // {
                //     // Another! Unused time domain transform configuration placeholder!
                //     Bit_uint<8> time_config;
                //     ss >> time_config;
                //     os << time_config;

                //     Bit_uint<8> floor_number;
                //     ss >> floor_number;
                //     os << floor_number;
                //     if (floor_number >= floor_count) throw Parse_error_str("invalid floor mapping");

                //     Bit_uint<8> residue_number;
                //     ss >> residue_number;
                //     os << residue_number;
                //     if (residue_number >= residue_count) throw Parse_error_str("invalid residue mapping");
                // }
            }

            // // mode count
            // Bit_uint<6> mode_count_less1;
            // ss >> mode_count_less1;
            // unsigned int mode_count = mode_count_less1 + 1;
            // os << mode_count_less1;

            // mode_blockflag = new bool [mode_count];
            // mode_bits = ilog(mode_count-1);

            // //cout << mode_count << " modes" << endl;

            // for (unsigned int i = 0; i < mode_count; i++)
            // {
            //     Bit_uint<1> block_flag;
            //     ss >> block_flag;
            //     os << block_flag;

            //     mode_blockflag[i] = (block_flag != 0);

            //     // only 0 valid for windowtype and transformtype
            //     Bit_uint<16> windowtype(0), transformtype(0);
            //     os << windowtype << transformtype;

            //     Bit_uint<8> mapping;
            //     ss >> mapping;
            //     os << mapping;
            //     if (mapping >= mapping_count) throw Parse_error_str("invalid mode mapping");
            // }

            // Bit_uint<1> framing(1);
            // os << framing;


        // os.flush_page();

        if ((ss.get_total_bits_read()+7)/8 != setup_packet.size()) throw Parse_error_str("didn't read exactly setup packet");

        if (setup_packet.next_offset() != _data_offset + static_cast<long>(_first_audio_packet_offset)) throw Parse_error_str("first audio packet doesn't follow setup packet");

    }
}

void Wwise_RIFF_Vorbis::generate_ogg(BinaryData& outputdata)
{
    Bit_oggstream os(outputdata);

    generate_ogg_header(os);
    return;
}
