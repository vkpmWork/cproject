/*
 * mp3bitrate.h
 *
 *  Created on: 25.05.2018
 *      Author: irina
 */

#ifndef MP3BITRATE_H_
#define MP3BITRATE_H_

namespace mp3
{

  class Tmp3_bitrate
  {
  private:
    int  mf;
    int  bitrate;

    bool mp3_IsValidFrameHeader(char*, int &m_ver, int &m_padding, int &m_sr, int &m_brindex, int &m_layer);
//    int  get_mp3_info();
    int  get_frame_size(char*);
    int  calc_bitrate(int m_mpegver, int m_brindex, int m_layer);

  public:
    Tmp3_bitrate(int m_mf /*file's descriptor */);
    int  get_bitrate();
    bool mp3_SeekForFirstFrame(int&); /* возвращает начало заголовка */
    int  get_start_frame(int m_start, int m_start_pos);
    int  GetStartPos();
  };

  extern bool get_file_segment(int mf, int& m_start, int& m_stop);
} /* namespace mp3_bitrate */
#endif /* MP3BITRATE_H_ */
