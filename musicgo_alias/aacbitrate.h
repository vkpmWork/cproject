/*
 * aacbitrate.h
 *
 *  Created on: 30.05.2018
 *      Author: irina
 */

#ifndef AACBITRATE_H_
#define AACBITRATE_H_

#include <iostream>
#include <fcntl.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

namespace aac
{

  class Taacbitrate
  {
  private:
      int  mf;
      int  bitrate;
      int  get_frame_length(unsigned char h3, unsigned char h4, unsigned char h5);
      bool aac_IsValidFrameHeader(char* header, int &m_length ,int &m_sampling_frequency);
public:
    Taacbitrate(int m_mf);
    int         get_start_pos();
    int         get_bitrate();
    bool        aac_SeekForFirstFrame(int&);
    int         get_start_frame(int m_start, int m_start_pos);
  };

  extern int get_aac_bitrate(int m_mf /*дескриптор aac-файла */, int &start_pos /*начало аудиоданных без заголовка */);
  extern bool get_file_segment(int mf, int& m_start, int& m_stop);
} /* namespace aac */
#endif /* AACBITRATE_H_ */
