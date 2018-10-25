/*
 * mpx.cpp
 *
 *  Created on: 15.03.2016
 *      Author: irina
 */

#include "mpx.h"
#include "math.h"
#include "log.h"

/* for ID3v1.X */
#define   METADATA_V1  -128

/* for ID3v2.3-4 */
#define   TIT2			"TIT2"
#define   TPE1			"TPE1"

/* for ID3v2.3-4 */
#define   TT2			"TT2"
#define   TP1			"TP1"

	const int brtable[] = /* Bitrate */
	{	    0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0,
		    0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 0,
		    0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 0,
		    0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256, 0,
		    0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160, 0};

	const int srtable[] = /* Sampling Rate */
	{   44100, 48000, 32000, 0,  // mpeg1
	    22050, 24000, 16000, 0,  // mpeg2
	    11025, 12000, 8000,  0}; // mpeg2.5


tmpx *ptr_mpx = NULL;

/* ------------------------------------ */
bool    mpx_create(size_t sz)
{
	ptr_mpx = new tmpx(sz);
	return ptr_mpx != NULL;
}

void 	 mpx_StopPlay()
{
	if (ptr_mpx) ptr_mpx->mpx_StopPlay();
}

bool 	 mpx_StartPlay()
{
	//bool ret_value = false;
	//if (ptr_mpx) ret_value = ptr_mpx->mpx_StartPlay();

	return (ptr_mpx && ptr_mpx->mpx_StartPlay());
}

unsigned int mpx_GetFrames(char * &buf)
{
	unsigned  int ret_value = 0;
	if (ptr_mpx) ret_value = ptr_mpx->mpx_GetFrames(buf);
	return ret_value;
}

void 	mpx_free()
{
	if (ptr_mpx) { delete ptr_mpx; ptr_mpx = NULL; }
}

int mpx_SeekTo1Frame()
{
	int ret_value = -1;
	//if (ptr_aac) ret_value = ptr_aac->aac_SeekTo1Frame();

	return ret_value;
}

int mpx_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time)
{
	int ret_value = -1;
	if (ptr_mpx) ret_value = ptr_mpx->mpx_GetFileInfo(filename, br, spf, sr, frames , ch, m_frames_to_read, m_play_time);

	return ret_value;
}

std::string mpx_Metadata(char *f)
{
	std::string str;
	if (ptr_mpx) str = ptr_mpx->mpx_GetMetadata(f);
	return str;
}
/* ------------------------------------ */
/* ------------------------------------ */

tmpx::~tmpx()
{
	close_fs();
	free(pInfo);
}


int  tmpx::mpx_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &m_frame , int &ch, int &m_frames_to_read, int &m_play_time)
{
	size_t file_size 		 =  0;
	int ret_value 			 = -1;
	m_frame     	     	 =  0;
	int pos 				 = -1;

	logger::message.str("");

	if (!common::FileExists(filename)) return ret_value;

	fs = fopen(filename, "r+");  /* с этим файлом и будем работать */
	if (fs)
	{
		logger::message << std::endl << "New File to play :  " << filename << std::endl;

		fseek(fs, 0, SEEK_END);
		file_size = ftell(fs);
		fseek(fs, 0, SEEK_SET);

		pos = this->mpx_SeekTo1Frame();
		if (pos == -1)
		{
			logger::message << "Couldn't find MPEG frame in file  " << filename << std::endl;
			logger::write_log(logger::message);
			close_fs();
			return -1;
		}
		logger::message << "First frame found at offset :   " << pos << std::endl;
	}
	else
	{
		logger::message << "Couldn't open file " << filename << std::endl;
		logger::write_log(logger::message);
		return ret_value;
	}


	char* mpx_header = (char*)calloc(c_header_size, sizeof(char));
	if (!mpx_header)
	{
		close_fs();
		return ret_value;
	}

	if (fread(mpx_header, sizeof(char), c_header_size, fs) < (size_t)c_header_size)
	{
		close_fs();
		return ret_value;
	}

	/* chack the syncword */
	if ( (mpx_header[0] != 0xFF) && ((mpx_header[1]& 0xE0) != 0xE0) )
	{
		logger::message << "Bad syncword at first frame\n";
		logger::write_log(logger::message);
		close_fs();
		return ret_value;
	}

	/* get & check mpeg version */
	int mpegver = (mpx_header[1] & 0x18) >> 3;
	if (mpegver == 1 || mpegver > 3)
	{
		logger::message << "Bad (reserved) mpeg version at first frame\n";
		logger::write_log(logger::message);
		close_fs();
		return ret_value;
	}

	/* get & check mpeg layer */
	int layer = (mpx_header[1] & 0x06) >> 1;
	if (layer < 1 || layer > 3)
	{
		logger::message << "Bad (reserved) mpeg layer at first frame\n";
		logger::write_log(logger::message);
		close_fs();
		return ret_value;
	}

	/* get & check bitrate index */
	int brindex = (mpx_header[2] & 0xF0) >> 4;
	if (brindex < 0x01 || brindex > 0x0E)
	{
		logger::message << "Bad (reserved) bitrate index\n";
		logger::write_log(logger::message);
		close_fs();
		return ret_value;
	}

	/* get & check sampling rate index */
	int srindex = (mpx_header[2] & 0x0C) >> 2;
	if (srindex >= 0x03)
	{
		logger::message << "Bad sampling frequency index at first frame\n";
		logger::write_log(logger::message);
		close_fs();
		return ret_value;
	}

	sr = mpegver == 3 ? srtable[srindex] /* mpeg1 */: mpegver == 2 ? srtable[srindex + 4] /* mpeg2 */: srtable[srindex + 8] /* mpeg2.5 */;

	/* get & check channel configuration */
	ch = (mpx_header[3] & 0xC0) >> 6;

	int    frame_length		 =  0;
	int	   num_bytes_toread  =  0;
	while(!feof(fs))
	{
		frame_length = mpx_GetFrameSize(mpx_header);
		if (frame_length == 0 || frame_length > 50000) break;

		m_frame++;

		if (frame_length > c_header_size) num_bytes_toread = frame_length - c_header_size;
		else break;

		fseek(fs, num_bytes_toread, SEEK_CUR);
		if (fread(mpx_header, sizeof(char), c_header_size, fs) < (size_t)c_header_size) break;
	}
	free(mpx_header);

	fseek(fs, pos, SEEK_SET);

	switch (mpegver)
	{
		case 3 : spf = layer == 3 ? 384 : 1152;
				 break;
		case 2 :
		case 0 : spf = layer == 3 ? 384 : layer == 2 ? 1152 : 576;
				 break;
	}

	int playtime = (spf*m_frame)/sr;
	br	= file_size/playtime; /* in bytes per sec */
	br	= br*8/1000;		   /* in kbits per sec */
	ch  = ch <= 2 ? 2 : 1;

	std::string s = mpegver == 3 ? "MPEG 1" : mpegver == 2 ? "MPEG 2" : "MPEG 2.5";
	logger::message << "format		: " << s;

	s = layer == 3 ? "Layer I" : layer == 2 ? "Layer II" : "Layer III";
	logger::message << " " << s << std::endl;

	logger::message << "frames		: " << m_frame << std::endl;
	logger::message << "sample rate	: " << sr << " Hz" << std::endl;

	s = ch == 0 ? "Stereo" : layer == 1 ? "Joint Stereo" : layer == 2 ? "Dual channel" : "Mono";
	logger::message << "channels		: " << ch << " (" << s << ")" << std::endl;

	logger::message << "playtime		: " << ceil(playtime) << " sec" << std::endl;
	logger::message << "bitrate		: " << ceil(br) << " kbps (average)" << std::endl;

	m_frames_to_read = frames_to_read = round(sr/spf);
	total_frames     = m_frame;

	logger::write_log(logger::message);
	return ret_value = 0;
}

int  tmpx::mpx_SeekTo1Frame() /* возвращает начало заголовка */
{
	/* https://habrahabr.ru/post/103635/ */

	if (!fs) return -1;

	int    b_size 	   =  50000;
	int    ret_value   = -1;
	int    framelength = 0;
	int    pos		   = 0;

	unsigned char *buf 		 = (unsigned char*)calloc(b_size, sizeof(char));
	if (!buf) return ret_value;

	char *mpx_header = (char*)calloc(c_header_size, sizeof(char));
	if (!mpx_header)
	{
		free(buf);
		return ret_value;
	}

	int m_start_pos = GetStartPos(fs);
	fseek(fs, m_start_pos, SEEK_SET);

	b_size = fread(buf, sizeof(char), b_size, fs);
	std::cout << buf << std::endl;

	for (pos = 0; pos < b_size; pos++)
	{
		if ((buf[pos] == 0xFF) && ((buf[pos+1] & 0xE0) == 0xE0 ))
		{
				if (b_size - pos < 10) break;

				memcpy(mpx_header, buf+pos, c_header_size);
				if (mpx_IsValidFrameHeader(mpx_header))
				{
					framelength = mpx_GetFrameSize(mpx_header);

					if (framelength)
					{
						if ((pos + framelength + 4) > b_size) break;

						memcpy(mpx_header, buf+pos+framelength, c_header_size);
						if (mpx_IsValidFrameHeader(mpx_header))
						{
							ret_value = pos + m_start_pos;
							break;
						}
					}
				}
		}
	}

	fseek(fs, ret_value, SEEK_SET);

	free(mpx_header); mpx_header = NULL;
	free(buf); 		  buf 		 = NULL;

	return ret_value;
}

int  tmpx::mpx_IsValidFrameHeader(char* mpx_header)
{
	int ret_value = 0;
    int value     = 0;

	/* chack the syncword */
	if ( (mpx_header[0] != 0xFF) && ((mpx_header[1]& 0xE0) != 0xE0) ) return ret_value;

	/* get & check mpeg version */
	value = (mpx_header[1] & 0x18) >> 3;
	if (value == 1 || value > 3)		return ret_value;

	/* get & check mpeg layer */
	value = (mpx_header[1] & 0x06) >> 1;
	if (value > 3)			return ret_value;

	/* get & check bitrate index */
	value = (mpx_header[2] & 0xF0) >> 4;
	if (/*value < 0x01 ||*/ value > 0x0E) 	return ret_value;

	/* get & check sampling rate index */
	value = (mpx_header[2] & 0x0C) >> 2;
	if (value >= 0x03)					return ret_value;

	return (int) true;
}

int	 tmpx::mpx_GetFrameSize(char* mpx_header)
{

	int mpegver   = 0, layer = 0, brindex = 0, srindex = 0, bitrate = 0;
	int ret_value = 0;

	/* chack the syncword */
	if ( (mpx_header[0] != 0xFF) && ((mpx_header[1]& 0xE0) != 0xE0) ) return ret_value;

	mpegver = (mpx_header[1] & 0x18) >> 3;
	if (mpegver == 1 || mpegver > 3) return ret_value;

	layer = (mpx_header[1] & 0x06) >> 1;
	if (layer == 0 || layer > 3) return ret_value;

	srindex  = (mpx_header[2] & 0x0C) >> 2;
	if (srindex >= 3) return ret_value;

	brindex = (mpx_header[2] & 0xF0) >> 4;

	if (mpegver == 3 && layer == 3) bitrate = brtable[brindex];      /* mpeg1, layer1 */
	if (mpegver == 3 && layer == 2) bitrate = brtable[brindex + 16]; /* mpeg1, layer2 */
	if (mpegver == 3 && layer == 1) bitrate = brtable[brindex + 32]; /* mpeg1, layer3 */

	if ((mpegver == 2 || mpegver == 0) && (layer == 3)) bitrate = brtable[brindex + 48]; 			   /* mpeg2, 2.5, layer1 */
	if ((mpegver == 2 || mpegver == 0) && (layer == 2 || layer == 1)) bitrate = brtable[brindex + 64]; /* mpeg2, layer2 or layer3 */

	bitrate *= 1000;

	int padding  = (mpx_header[2] & 0x02) >> 1;
	int index    = mpegver == 0 ? 8 : mpegver == 2 ? 4 : 0;
	int sr  	 = srtable[srindex + index];


	switch(mpegver)
	{
		case 3 : if (layer == 3) ret_value = floor(12*bitrate/sr)*4 + padding*4;
				 else if (layer == 2 || layer == 1) ret_value = floor(144 * bitrate/sr + padding);
				 break;  /* mpeg1   */
		case 2 :
		case 0 : if (layer == 3) ret_value = floor(12*bitrate/sr)*4 + padding*4;
				 else if (layer == 2) ret_value = floor(144*bitrate/sr + padding);
				 	  else if (layer == 1) ret_value = floor(72*bitrate/sr + padding);
				 break;  /* mpeg2.5 */
		default: return ret_value;
	}

	return ret_value;
}

bool  tmpx::mpx_StartPlay()
{
	if (!fs) return false;

	send_frame_number  = 0;

	bool ret_value = this->mpx_SeekTo1Frame() != -1;
	if (!ret_value) this->mpx_StopPlay();

	return ret_value;
}

unsigned int  tmpx::mpx_GetFrames(char * &buf)
{
	unsigned int ret_value 		  = 0;
	int 		 frames_from_file = 0;

	char mpx_header[c_header_size+1];
	int  buf_length = 0;
	int	 pos 	= 0;
	int  bytes_to_read = 0;


	while (frames_from_file < frames_to_read)
	{
		if(send_frame_number >= total_frames) 	break;

		memset(mpx_header, 0, c_header_size);

		if (fread(mpx_header, sizeof(char), c_header_size, fs) < (size_t)c_header_size)
		{
			break;
		}

		//buf_length = this->mpx_IsValidFrameHeader(headers);
		if (!this->mpx_IsValidFrameHeader(mpx_header))
		{
			break;
		}

		/* copy frame header to output buffer */
		buf_length = mpx_GetFrameSize(mpx_header);
		if (buf_length == 0 || buf_length > 5000) break;

		memcpy(buf+pos, mpx_header, c_header_size);
		pos += c_header_size;

		bytes_to_read = buf_length - c_header_size;

		buf_length = fread(buf+pos, sizeof(char), bytes_to_read, fs);

		if (buf_length < bytes_to_read)
		{
			break;
		}

		pos += buf_length;
		frames_from_file ++;
		ret_value = pos;
	}

	send_frame_number += frames_from_file;

	return ret_value;
}

inline void tmpx::close_fs()
{
	if (fs)
	{
		try
		{
			fclose(fs);
			fs = NULL;
		}
		catch(...) {}
	}
}

int tmpx::GetStartPos(FILE* fs_)
{

	memset(pInfo, 0, sizeof_TInfo);

	int m_start_position = 0;
	fseek(fs_, 0, SEEK_SET);

	fread(pInfo, 1, sizeof_TInfo, fs_);

    if (!memcmp(pInfo->ID, "ID3", 3)) /* ID3v2.2, ID3v2.3 и ID3v2.4 */
    {
    	m_start_position = pInfo->Size[3] | ((int)pInfo->Size[2] <<  7) | ((int)pInfo->Size[1] << 14) | ((int)pInfo->Size[0] << 21);
    }
    else
    {
    	fseek(fs_, METADATA_V1, SEEK_END);
    	fread(pInfo, 1, sizeof_TInfo, fs_);
    	if (!memcmp(pInfo->ID, "TAG", 3)) pInfo->Version = 1;
    }

    fseek(fs_, 0, SEEK_SET);
	return m_start_position;
}

std::string tmpx::mpx_GetMetadata(char *filename)
{
	std::string str;

	if (pInfo == NULL) return str;

	switch(pInfo->Version)
	{
		case 1: str = Metadata_V1(filename);
				break;
		case 2:
		case 3:
		case 4:	str = Metadata_V2(filename, pInfo->Version);
				break;
	}

	return str;
}

std::string tmpx::Metadata_V1(char *filename)
{
	const int sz = 30;
	std::string str;

	FILE* f = fopen(filename, "r+");  /* с этим файлом и будем работать */
	if (!f) return str;

	char *s = (char*)calloc(sz+1, sizeof(char));
	if (!s) return str;

	fseek(f, METADATA_V1, SEEK_END);

	fread(s, 3, sizeof(char), f);

	if (!memcmp(s, "TAG", 3))
	{
		memset(s, 0, sz);
		if (fread(s, sz, sizeof(char), f)) str.append(s);
		if (fread(s, sz, sizeof(char), f))
		{
			str.append(", ");
			str.append(s);
		}
	}
	fclose(f);
	free(s);

	return str;
}

std::string tmpx::Metadata_V2(char *filename , int version)
{
	std::string str;

	FILE* f = fopen(filename, "r+");  /* с этим файлом и будем работать */
	if (!f) return str;

	char *data = NULL;

	char *s    = (char*)calloc(sizeof_TInfo+1, sizeof(char));
	if (!s) return str;

	fseek(f, 0, SEEK_SET);
	fread(s, sizeof_TInfo, sizeof(char), f);

	int   f_size = 0;
    if (!memcmp(s, "ID3", 3)) /* ID3v2.2, ID3v2.3 и ID3v2.4 */
    {
    	f_size = s[9] | ((int)s[8] <<  7) | ((int)s[7] << 14) | ((int)s[6] << 21);
    }
    else return str;

	int   len        = 0;
	int   stop_index = 0;

	char *tit2 = (char*) calloc(version == 2 ? 4: 5, sizeof(char));
	char *tpe1 = (char*) calloc(version == 2 ? 4: 5, sizeof(char));

	while (f_size > sizeof_TInfo)
	{
		memset(s, 0, sizeof_TInfo);

		fread(s, sizeof_TInfo, sizeof(char), f);
		if (!memcmp(s, tit2, 4))
		{
			len = s[7] | ((int)s[6] <<  8) | ((int)s[5] << 16) | ((int)s[4] << 24);
			if (len)
			{
				data = (char*)calloc(len+1, sizeof(char));
				fread(data, len, sizeof(char), f);
				str.append(data);
				free(data);
			}
			stop_index |= 0x01;
		}

		if (!memcmp(s, tpe1, 4))
		{
			len = s[7] | ((int)s[6] <<  8) | ((int)s[5] << 16) | ((int)s[4] << 24);
			if (len)
			{
				data = (char*)calloc(len+1, sizeof(char));
				fread(data, len, sizeof(char), f);
				str.append(data);
				free(data);
			}
			stop_index |= 0x02;
		}

		if (stop_index ==0x03 ) break;
		f_size-= (sizeof_TInfo + len);
	}

	fclose(f);
	free(s);
	free(tit2);
	free(tpe1);

	return str;

}
