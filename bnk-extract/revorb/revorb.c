/*
 * REVORB - Recomputes page granule positions in Ogg Vorbis files.
 *  version 0.3 (2015/09/03) - Jon Boydell <jonboydell@hotmail.com> - version for *NIX systems
 *  version 0.2 (2008/06/29)
 *
 * Copyright (c) 2008, Jiri Hruska <jiri.hruska@fud.cz>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*# INCLUDE=.\include #*/
/*# LIB=.\lib         #*/
/*# CFLAGS=/D_UNICODE #*/
/*# LFLAGS=/NODEFAULTLIB:MSVCRT /LTCG /OPT:REF /MANIFEST:NO #*/

#ifdef __APPLE__
    #include <sys/uio.h>
#else
    #include <stdio.h>
#endif
#include <stdbool.h>
#include "stdlib.h"
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include "../defs.h"
#include "../general_utils.h"
#include "../list.h"

bool g_failed;

uint32_t copy_headers(BinaryData* file_data, ogg_sync_state *si, ogg_stream_state *is,
                      uint8_list* lo, ogg_stream_state *os, vorbis_info *vi)
{
    char *buffer = ogg_sync_buffer(si, 4096);
    uint32_t numread = min(file_data->length, 4096u);
    memcpy(buffer, file_data->data, numread);
    uint32_t file_pos = numread;
    ogg_sync_wrote(si, numread);

    ogg_page page;
    if (ogg_sync_pageout(si, &page) != 1) {
        eprintf("Input is not an Ogg.\n");
        return false;
    }

    ogg_stream_init(is, ogg_page_serialno(&page));
    ogg_stream_init(os, ogg_page_serialno(&page));

    if (ogg_stream_pagein(is,&page) < 0) {
        eprintf("Error in the first page.\n");
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
    }

    ogg_packet packet;
    if (ogg_stream_packetout(is,&packet) != 1) {
        eprintf("Error in the first packet.\n");
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
    }

    vorbis_comment vc;
    vorbis_comment_init(&vc);
        if (vorbis_synthesis_headerin(vi, &vc, &packet) < 0) {
        eprintf("Error in header, probably not a Vorbis file.\n");
        vorbis_comment_clear(&vc);
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
    }

    ogg_stream_packetin(os, &packet);

    int i = 0;
    while(i < 2) {
        int res = ogg_sync_pageout(si, &page);

        if (res == 0) {
            buffer = ogg_sync_buffer(si, 4096);
            int numread = min(file_data->length - file_pos, 4096u);
            memcpy(buffer, &file_data->data[file_pos], numread);
            file_pos += numread;
            if (numread == 0 && i < 2) {
                eprintf("Headers are damaged, file is probably truncated.\n");
                ogg_stream_clear(is);
                ogg_stream_clear(os);
                return false;
            }
            ogg_sync_wrote(si, 4096);
            continue;
        }

        if (res == 1) {
            ogg_stream_pagein(is, &page);
            while(i < 2) {
                res = ogg_stream_packetout(is, &packet);
                if (res == 0)
                    break;
                if (res < 0) {
                    eprintf("Secondary header is corrupted.\n");
                    vorbis_comment_clear(&vc);
                    ogg_stream_clear(is);
                    ogg_stream_clear(os);
                    return false;
                }
                vorbis_synthesis_headerin(vi, &vc, &packet);
                ogg_stream_packetin(os, &packet);
                i++;
            }
        }
    }

    vorbis_comment_clear(&vc);

    while(ogg_stream_flush(os,&page)) {
        add_objects(lo, page.header, (uint32_t) page.header_len);
        add_objects(lo, page.body, (uint32_t) page.body_len);
        // if (fwrite(page.header, 1, page.header_len, fo) != (size_t) page.header_len || fwrite(page.body, 1, page.body_len, fo) != (size_t) page.body_len) {
            // fprintf(stderr,"Cannot write headers to output.\n");
            // ogg_stream_clear(is);
            // ogg_stream_clear(os);
            // return false;
        // }
    }

    return file_pos;
}

BinaryData* revorb(int argc, const char **argv)
{
    if (argc < 2) {
        eprintf("-= REVORB - <yirkha@fud.cz> 2008/06/29 =-\n");
        eprintf("Recomputes page granule positions in Ogg Vorbis files.\n");
        eprintf("Usage:\n");
        eprintf("  revorb <input.ogg> [output.ogg]\n");
        return NULL;
    }

    uint8_list output_buffer;
    initialize_list(&output_buffer);

  ogg_sync_state sync_in;
  ogg_sync_init(&sync_in);

  ogg_stream_state stream_in, stream_out;
  vorbis_info vi;
  vorbis_info_init(&vi);

  ogg_packet packet;
  ogg_page page;

  BinaryData* file_data;
  hex2bytes(argv[1], &file_data, 16);
  uint32_t file_pos;
  if ( (file_pos = copy_headers(file_data, &sync_in, &stream_in, &output_buffer, &stream_out, &vi)) ) {
      ogg_int64_t granpos = 0, packetnum = 0;
      int lastbs = 0;

    while(1) {

      int eos = 0;
      while(!eos) {
        int res = ogg_sync_pageout(&sync_in, &page);
        if (res == 0) {
          char *buffer = ogg_sync_buffer(&sync_in, 4096);
          int numread = min(file_data->length - file_pos, 4096u);
          memcpy(buffer, &file_data->data[file_pos], numread);
          file_pos += numread;
          if (numread > 0)
            ogg_sync_wrote(&sync_in, numread);
          else
            eos = 2;
          continue;
        }

        if (res < 0) {
          eprintf("Warning: Corrupted or missing data in bitstream.\n");
          g_failed = true;
        } else {
          if (ogg_page_eos(&page))
            eos = 1;
          ogg_stream_pagein(&stream_in,&page);

          while(1) {
            res = ogg_stream_packetout(&stream_in, &packet);
            if (res == 0)
              break;
            if (res < 0) {
              eprintf("Warning: Bitstream error.\n");
              g_failed = true;
              continue;
            }

            /*
            if (packet.granulepos >= 0) {
              granpos = packet.granulepos + logstream_startgran;
              packet.granulepos = granpos;
            }
            */
            int bs = vorbis_packet_blocksize(&vi, &packet);
            if (lastbs)
              granpos += (lastbs+bs) / 4;
            lastbs = bs;

            packet.granulepos = granpos;
            packet.packetno = packetnum++;
            if (!packet.e_o_s) {
              ogg_stream_packetin(&stream_out, &packet);

              ogg_page opage;
              while(ogg_stream_pageout(&stream_out, &opage)) {
                add_objects(&output_buffer, opage.header, (uint32_t) opage.header_len);
                add_objects(&output_buffer, opage.body, (uint32_t) opage.body_len);
                // if (fwrite(opage.header, 1, opage.header_len, fo) != (size_t) opage.header_len || fwrite(opage.body, 1, opage.body_len, fo) != (size_t) opage.body_len) {
                  // eprintf("Unable to write page to output.\n");
                  // eos = 2;
                  // g_failed = true;
                  // break;
                // }
              }
            }
          }
        }
      }

      if (eos == 2)
        break;

      {
        packet.e_o_s = 1;
        ogg_stream_packetin(&stream_out, &packet);
        ogg_page opage;
        while(ogg_stream_flush(&stream_out, &opage)) {
          add_objects(&output_buffer, opage.header, (uint32_t) opage.header_len);
          add_objects(&output_buffer, opage.body, (uint32_t) opage.body_len);
          // if (fwrite(opage.header, 1, opage.header_len, fo) != (size_t) opage.header_len || fwrite(opage.body, 1, opage.body_len, fo) != (size_t) opage.body_len) {
            // eprintf("Unable to write page to output.\n");
            // g_failed = true;
            // break;
          // }
        }
        ogg_stream_clear(&stream_in);
        break;
      }
    }

    ogg_stream_clear(&stream_out);
  } else {
    g_failed = true;
  }

  vorbis_info_clear(&vi);

  ogg_sync_clear(&sync_in);

  // fclose(fo);

  BinaryData* converted_ogg_data = malloc(sizeof(BinaryData));
  converted_ogg_data->data = output_buffer.objects;
  converted_ogg_data->length = output_buffer.length;

  return converted_ogg_data;
}
