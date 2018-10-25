/*
 * mp3bitrate.cpp
 *
 *  Created on: 25.05.2018
 *      Author: irina
 */

#include "mp3bitrate.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#define   METADATA_V1  -128
#define   HEADER_SIZE   4

        const int srtable[] = /* Sampling Rate */
        {   44100, 48000, 32000, 0,  // mpeg1
            22050, 24000, 16000, 0,  // mpeg2
            11025, 12000, 8000,  0}; // mpeg2.5

        const int brtable[] = /* Bitrate */
        {           0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0,
                    0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 0,
                    0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 0,
                    0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256, 0,
                    0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160, 0};

namespace mp3
{
  bool get_file_segment(int mf, int &m_start, int &m_stop)
  {
    Tmp3_bitrate br(mf);

    int start_pos = br.GetStartPos();
    lseek(mf, start_pos, SEEK_SET);

    if (!br.mp3_SeekForFirstFrame(start_pos)) return false;

    m_start = br.get_start_frame(m_start, start_pos);

    m_stop  *= br.get_bitrate();
    return true;
  }


  Tmp3_bitrate::Tmp3_bitrate(int m_mf) : mf(m_mf), bitrate(0) {}

  int Tmp3_bitrate::get_bitrate()
  {
    return bitrate/8;
  }

  bool  Tmp3_bitrate::mp3_SeekForFirstFrame(int &start_pos) /* возвращает начало заголовка */
  {
          /* https://habrahabr.ru/post/103635/ */

          bool is_found   = false;
          if (mf == -1) return is_found;

          size_t b_size      =  50000;

          unsigned char *buf               = (unsigned char*)calloc(b_size, sizeof(char));
          if (!buf) return is_found;

          char *mpx_header = (char*)calloc(HEADER_SIZE, sizeof(char));
          if (!mpx_header)
          {
                  free(buf);
                  return is_found;
          }

          b_size = read(mf, buf, b_size);

          for (size_t pos = 0; pos < b_size; pos++)
          {
                  if ((buf[pos] == 0xFF) && ((buf[pos+1] & 0xE0) == 0xE0 ))
                  {
                                  if (b_size - pos < 10) break;

                                  memcpy(mpx_header, buf+pos, HEADER_SIZE);
                                  int  m_len = get_frame_size(mpx_header);
                                  if (m_len)
                                  {
                                      if ((pos + m_len + 4) > b_size) break;
                                      memcpy(mpx_header, buf+pos+m_len, HEADER_SIZE);
                                      m_len = get_frame_size(mpx_header);
                                      if (m_len)
                                      {
                                          start_pos += pos;
                                          is_found   = true;
                                          break;
                                      }
                                  }
                  }
          }

          if (is_found)
          {
              int mpegver, padding, sr, brindex, layer;

              mp3_IsValidFrameHeader(mpx_header, mpegver = 0, padding = 0, sr = 0, brindex = 0, layer = 0);
              bitrate = calc_bitrate(mpegver, brindex, layer);
          }

          free(mpx_header); mpx_header = NULL;
          free(buf);        buf        = NULL;

          return is_found;
  }

  int Tmp3_bitrate::GetStartPos()
  {
          struct
          {
                  char          ID[3]; // сигнатура тэга, должна быть равна "ID3 или TAG"
                  unsigned char Version; // основной байт версии (major)
                  unsigned char Revision; // дополнительный байт версии (minor)
                  unsigned char Flags; // флаги
                  unsigned char Size [4]; // размер тэга без заголовков
          } pInfo;

          int sz = sizeof(pInfo);
          memset(&pInfo, 0, sz);

          int m_start_position = 0;
          lseek(mf, 0, SEEK_SET);

          if (read(mf, &pInfo, (size_t)sz) < 10) return -1;

      if (!memcmp(pInfo.ID, "ID3", 3)) /* ID3v2.2, ID3v2.3 и ID3v2.4 */
      {
          /* размер ID3 заголовка */
          m_start_position = pInfo.Size[3] | ((int)pInfo.Size[2] <<  7) | ((int)pInfo.Size[1] << 14) | ((int)pInfo.Size[0] << 21);
      }
      else
      {
          lseek(mf, METADATA_V1, SEEK_END);
          read(mf, &pInfo, (size_t)sz);
          if (!memcmp(pInfo.ID, "TAG", 3)) pInfo.Version = 1;
      }

      return m_start_position;
  }

  bool  Tmp3_bitrate::mp3_IsValidFrameHeader(char* mpx_header, int &m_ver, int &m_padding, int &m_sr, int &m_brindex, int &m_layer)
  {
          /* chack the syncword */
          if ( (mpx_header[0] != 0xFF) && ((mpx_header[1]& 0xE0) != 0xE0) ) return false;

          /* get & check mpeg version */
          m_ver = (mpx_header[1] & 0x18) >> 3;
          if (m_ver == 1 || m_ver > 3)  return false;

          /* get & check mpeg layer */
          m_layer = (mpx_header[1] & 0x06) >> 1;
          if (m_layer > 3)      return false;

          /* get & check bitrate index */
          m_brindex = (mpx_header[2] & 0xF0) >> 4;
         if (/*brindex < 0x01 ||*/ m_brindex > 0x0E)   return false;

          /* get & check sampling rate index */
          int srindex = (mpx_header[2] & 0x0C) >> 2;
          if (srindex >= 0x03)  return false;

          m_padding = (mpx_header[2] & 0x02) >> 1;

          int index    = m_ver == 0 ? 8 : m_ver == 2 ? 4 : 0;
          m_sr         = srtable[srindex + index];

          return true;
  }

  int  Tmp3_bitrate::calc_bitrate(int m_mpegver, int m_brindex, int m_layer)
  {
    int m_bitrate = 0;
    if (m_mpegver == 3 && m_layer == 3) m_bitrate = brtable[m_brindex];      /* mpeg1, layer1 */
    if (m_mpegver == 3 && m_layer == 2) m_bitrate = brtable[m_brindex + 16]; /* mpeg1, layer2 */
    if (m_mpegver == 3 && m_layer == 1) m_bitrate = brtable[m_brindex + 32]; /* mpeg1, layer3 */

    if ((m_mpegver == 2 || m_mpegver == 0) && (m_layer == 3))                 m_bitrate = brtable[m_brindex + 48];               /* mpeg2, 2.5, layer1 */
    if ((m_mpegver == 2 || m_mpegver == 0) && (m_layer == 2 || m_layer == 1)) m_bitrate = brtable[m_brindex + 64]; /* mpeg2, layer2 or layer3 */

    return m_bitrate*1000;
  }

  int  Tmp3_bitrate::get_frame_size(char* mpx_header)
  {
    int m_frame_size = 0;
    int mpegver, padding, sr, brindex, layer;

    if (!mp3_IsValidFrameHeader(mpx_header, mpegver = 0, padding = 0, sr = 0, brindex = 0, layer = 0)) return 0;
    int m_bitrate = calc_bitrate(mpegver, brindex, layer);

    switch(mpegver)
    {
            case 3 : if (layer == 3) m_frame_size = floor(12*m_bitrate/sr)*4 + padding*4;
                             else if (layer == 2 || layer == 1) m_frame_size = floor(144 * m_bitrate/sr + padding);
                             break;  /* mpeg1   */
            case 2 :
            case 0 : if (layer == 3) m_frame_size = floor(12*m_bitrate/sr)*4 + padding*4;
                             else if (layer == 2) m_frame_size = floor(144*m_bitrate/sr + padding);
                                      else if (layer == 1) m_frame_size = floor(72*m_bitrate/sr + padding);
                             break;  /* mpeg2.5 */
    }

    return m_frame_size;
  }

  int Tmp3_bitrate::get_start_frame(int m_start, int m_start_pos)
  {

    m_start *= get_bitrate();
    m_start += m_start_pos;
    lseek(mf, m_start, SEEK_SET);

    if (!mp3_SeekForFirstFrame(m_start)) return 0;

    return m_start;
  }

} /* namespace mp3_bitrate */
