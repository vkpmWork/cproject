/*
 * aacbitrate.cpp
 *
 *  Created on: 30.05.2018
 *      Author: irina
 */

#include "aacbitrate.h"

namespace aac
{
  int get_aac_bitrate(int m_mf /*дескриптор aac-файла */, int &start_pos /*начало аудиоданных без заголовка */)
  {
      Taacbitrate br(m_mf);
      start_pos = br.get_start_pos();

      return br.get_bitrate();
  }

  bool get_file_segment(int mf, int &m_start, int &m_stop)
  {

    int mm_start = m_start;
    Taacbitrate br(mf);

    int start_pos = br.get_start_pos();
    if (start_pos == -1) return false;


    int aa = start_pos;
    if (!br.aac_SeekForFirstFrame(aa)) return false;

    lseek(mf, start_pos, SEEK_SET);

    m_start = br.get_start_frame(m_start, start_pos);
    m_stop  *= br.get_bitrate();
    return true;
  }


  Taacbitrate::Taacbitrate(int m_mf)
              : mf(m_mf)
              , bitrate(0)
  {
    // TODO Auto-generated constructor stub

  }

  int Taacbitrate::get_bitrate()
  {
      return bitrate;
  }

  bool  Taacbitrate::aac_SeekForFirstFrame(int &start_pos)
  {
          if (mf == -1) return false;

          int          m_frame_length , m_sampling_frequency;
          int          m_frame = 0, m_next = 0;

          struct stat st;
          if ( fstat(mf, &st) == -1) return false;
          int m_file_size = st.st_size;

          lseek(mf, start_pos, SEEK_SET);

          const int     m_header_size = 7;
          char         *headers = (char*)malloc(m_header_size);
          if (!headers) return false;

          while (true)
          {
                memset(headers, 0, m_header_size);
                if (read(mf, headers, m_header_size) < m_header_size) return false;

                if (!aac_IsValidFrameHeader(headers, m_frame_length = 0,  m_sampling_frequency = 0))
                {
                    break; // AAC-header wasn't found in file
                }

                m_frame++;
                m_next += m_frame_length;

                if (m_next < m_file_size)
                {
                    int m_pos = m_frame_length - m_header_size;
                    lseek(mf, m_pos, SEEK_CUR);
                }
                else
                  break;

          }

          if (m_frame)
          {
                float play_time = (float)(1024*m_frame)/m_sampling_frequency; // play time in seconds

                bitrate   = m_file_size/ (int)play_time; // in bytes per second
          }

          return bitrate != 0;
  }

  int   Taacbitrate::get_start_frame(int m_start, int m_start_pos)
  {
    m_start *= get_bitrate();
    m_start += m_start_pos;
    lseek(mf, m_start, SEEK_SET);

    m_start += get_start_pos();
    return m_start;
  }

  int Taacbitrate::get_frame_length(unsigned char h3, unsigned char h4, unsigned char h5)
  {
          return ( (h3 & 0x03) << 11) | (h4 << 3) | ((h5 & 0x0E0) >> 5);
  }

  bool Taacbitrate::aac_IsValidFrameHeader(char* header, int &m_length ,int &m_sampling_frequency)
  {
          //AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)
          if (!header) return false;
          /*bytes per second */;
          const unsigned int sftable[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0 };

          unsigned short value  = ((header[0] << 4) & 0xFF0) | ((header[1] >> 4) & 0x0F);
          if (value != 0x0FFF)                    return false; // Syncword isn't valid

          value = (header[2] & 0xC0) >> 6;
          if (value == 3)                         return false; //Profile isn't valid

          m_sampling_frequency = ((header[2] & 0x3C) >> 2) & 0x0F;
          m_sampling_frequency = sftable[m_sampling_frequency];
          if (!m_sampling_frequency)              return false; // MPEG-4 Sampling Frequency Index isn't valid

          m_length = get_frame_length(header[3], header[4], header[5]);
          if (m_length < 0 || m_length > 5000)    return false; // Frame length isn't valid

          return true;
  }


  int Taacbitrate::get_start_pos()
  {
    int start_pos    = 0;
    size_t b_size    = 50000;

    char *buf        = (char*) malloc(b_size+1);
    unsigned short   syncword        = 0;
    int  m_frame_length , m_sampling_frequency;

    if (buf)
    {
            memset(buf, 0, b_size);
            b_size = read(mf, buf, b_size);

            for (int i = 0; i < (int)b_size; i++)
            {
                    syncword = ((buf[i] << 4) & 0xFF0) | ((buf[i+1] >> 4) & 0x0F);
                    if(syncword == 0x0FFF)
                            if (b_size - i >=  10)
                            {

                                    if (aac_IsValidFrameHeader(&buf[i], m_frame_length = 0,  m_sampling_frequency = 0))
                                    {
                                        start_pos = i;
                                        break;
                                    }
                            }
            }

            free(buf);
            buf = NULL;
    }
    return start_pos;
  }

} /* namespace aac */
