/*
 * aac.cpp
 *
 *  Created on: 05.02.2016
 *      Author: irina
 */

#include "log.h"
#include "aac.h"
#include "common.h"
#include "math.h"

const unsigned int sftable[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0 };
//const char c_header_size =  7;

taac 	*ptr_aac = NULL;

/* ------------------------------------ */
void 	 aac_StopPlay()
{
	if (ptr_aac) ptr_aac->aac_StopPlay();
}

bool 	 aac_StartPlay()
{
	bool ret_value = false;
	if (ptr_aac) ret_value = ptr_aac->aac_StartPlay();

	return ret_value;
}

unsigned int aac_GetFrames(char * &buf)
{
	unsigned  int ret_value = 0;
	if (ptr_aac) ret_value = ptr_aac->aac_GetFrames(buf);
	return ret_value;
}

/* ------------------------------------ */
bool     aac_create(size_t sz)
{
	ptr_aac = new taac(sz);
	return ptr_aac != NULL;
}

void 	aac_free()
{
	if (ptr_aac) { delete ptr_aac; ptr_aac = NULL; }
}

int aac_SeekTo1Frame()
{
	int ret_value = -1;
	if (ptr_aac) ret_value = ptr_aac->aac_SeekTo1Frame();

	return ret_value;
}

int aac_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time)
{
	int ret_value = -1;
	if (ptr_aac) ret_value = ptr_aac->aac_GetFileInfo(filename, br, spf, sr, frames , ch, m_frames_to_read, m_play_time);

	return ret_value;
}

std::string aac_Metadata(char* f)
{
	std::string str;
	if (ptr_aac) str =  ptr_aac->aac_GetMetadata(f);
	return str;
}

/* ------------------------------------ */

taac::~taac()
{
	close_fs();
}

int taac::get_frame_length(unsigned char h3, unsigned char h4, unsigned char h5)
{
	return ( (h3 & 0x03) << 11) | (h4 << 3) | ((h5 & 0x0E0) >> 5);
}

bool taac::aac_IsValidFrameHeader(char* header, int &m_protection_absent, int &m_length ,int &m_sampling_frequency, int &m_channel_config)
{
	if (!header) return false;

	logger::message.str("");
	logger::message.str("");
	m_protection_absent = m_length = m_channel_config = 0;

	unsigned short value  = ((header[0] << 4) & 0xFF0) | ((header[1] >> 4) & 0x0F);
	if (value != 0x0FFF)
	{
		logger::message << "Syncword isn't valid" << std::endl;
		logger::write_log(logger::message);
		return false;
	}

	value = (header[2] & 0xC0) >> 6;
	if (value == 3)
	{
		logger::message.str("Profile isn't valid\n");
		logger::write_log(logger::message);
		return false;
	}

	m_sampling_frequency = ((header[2] & 0x3C) >> 2) & 0x0F;
	if (!sftable[m_sampling_frequency])
	{
		logger::message << "MPEG-4 Sampling Frequency Index isn't valid.";
		logger::write_log(logger::message);
		m_sampling_frequency = -1;
		return false;
	}

	m_length = get_frame_length(header[3], header[4], header[5]);
	if (m_length < 0 || m_length > 5000)
	{
		logger::message << "Frame length isn't valid.";
		logger::write_log(logger::message);
		return false;
	}

	m_protection_absent = header[1] & 0x01; /* set to 1 if there is no CRC and 0 if there is CRC */
	m_channel_config    = ((header[2] & 0x01) << 2) || ((header[3] & 0xC0) >> 6);

	return true;
}

int  taac::aac_IsValidFrameHeader(char* header)
{
	if (!header) return 0;

	int m_length = 0;

	unsigned short value  = ((header[0] << 4) & 0xFF0) | ((header[1] >> 4) & 0x0F);
	if (value != 0x0FFF)
	{
		logger::message.str("Syncword isn't valid\n");
		logger::write_log(logger::message);
		return 0;
	}

	value = (header[2] & 0xC0) >> 6;
	if (value == 3)
	{
		logger::message.str("Profile isn't valid\n");
		logger::write_log(logger::message);
		return 0;
	}

	value = ((header[2] & 0x3C) >> 2) & 0x0F;
	if (!sftable[value])
	{
		logger::message.str("MPEG-4 Sampling Frequency Index isn't valid.\n");
		logger::write_log(logger::message);
		return 0;
	}

	m_length = get_frame_length(header[3], header[4], header[5]);
	if (m_length <= c_header_size || m_length > 5000)
	{
		logger::message.str("Frame length isn't valid.\n");
		logger::write_log(logger::message);
		m_length = 0;
	}

	return m_length;
}

int  taac::aac_SeekTo1Frame()
{
	if (!fs) return -1;

	int    ret_value = -1;
	size_t b_size    = 50000;

	char *buf        = (char*) malloc(b_size+1);
	unsigned short   syncword 	 = 0;

	if (buf)
	{
		fseek(fs, 0, SEEK_SET);
		memset(buf, 0, b_size);
		b_size = fread(buf, sizeof(char), b_size, fs);

		for (int i = 0; i < (int)b_size; i++)
		{
			syncword = ((buf[i] << 4) & 0xFF0) | ((buf[i+1] >> 4) & 0x0F);

			if(syncword == 0x0FFF)
				if (b_size - i >=  10)
				{
					ret_value = i;
					break;
				}
		}

		free(buf);
		buf = NULL;
		fseek(fs, 0, SEEK_SET);
	}

	return ret_value;
}

int  taac::aac_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &m_frame , int &ch, int &m_frames_to_read, int &m_play_time)
{
	int ret_value 			 = -1;
	m_frame     	     	 =  0;
	size_t file_size 		 =  0;

	if (!common::FileExists(filename)) return ret_value;

	logger::message.str("");

	fs = fopen(filename, "r");  /* с этим файлом и будем работать */
	if (fs)
	{
		//logger::message << "New File to play :  " << filename << std::endl;
		//logger::write_log(logger::message);

		fseek(fs, 0, SEEK_END);
		file_size = ftell(fs);
		fseek(fs, 0, SEEK_SET);

		int index = aac_SeekTo1Frame();
		if (index == -1)
		{
			logger::message << "AAC-header wasn't found in file " << filename << std::endl;
			logger::write_log(logger::message);
			close_fs();//
			return -1;
		}

		char headers[c_header_size+1];
		int m_protection_absent = 0;
		int m_frame_length      = 0;
		size_t m_next_file_pos 	= index;

		ret_value = 1;
		while (true)
		{
			memset(headers, 0, c_header_size);
			index = fread(headers, sizeof(char), c_header_size, fs);

			if (index < c_header_size)
			{
				logger::message << "AAC-header wasn't found in file (index < c_header_size) : " << filename << std::endl;
				logger::write_log(logger::message);

				ret_value = -1;
				break;
			}

			if (!aac_IsValidFrameHeader(headers, m_protection_absent, m_frame_length, sr, ch))
			{
				logger::message << "AAC-header wasn't found in file (aac_IsValidFrameHeader) : " << filename << std::endl;
				logger::write_log(logger::message);
				ret_value = -1;
				break;
			}

			/* переходим к следующему фрейму */
			m_frame++;
			m_next_file_pos += m_frame_length;


			if (m_next_file_pos < file_size) fseek(fs, m_frame_length - c_header_size, SEEK_CUR);
			else break;

		}

	}
	else logger::message << "Couldn't open file " << filename << std::endl;

	if (ret_value != -1)
	{
		spf = 1024;
		sr  = sftable[sr];

		float play_time = (float)(1024*m_frame)/sr;

		m_play_time = (int) play_time;
		/* in bytes/sec */
		if (play_time > 0)
		{
			br  = file_size/ play_time;

			/* in kbits/sec */
			br  = br*8/ 1000;

			logger::message << "Total frames: " << m_frame << "  Samplerate: " << sr << "Hz  Channels: " << ch << "  Playtime: " << play_time  << " sec";
			logger::message << "  Bitrate : " << (int)br    <<  "kbps (average)" << std::endl;

			m_frames_to_read = frames_to_read = round((float)sr/spf);
		}


		if (play_time <= 0 || br < 32 || br > 320 )
		{
			ret_value = -1;

			logger::message.str("");
			logger::message << "Something wrong whith file " << filename << std::endl;
			logger::message << "Play time = " << play_time << " br = " << br << std::endl;
		}

	}


	if (ret_value == -1) this->aac_StopPlay();

	total_frames = m_frame;
	logger::write_log(logger::message, common::levWarning);

	return ret_value;
}
bool  taac::aac_StartPlay()
{
	if (!fs) return false;

	send_frame_number  = 0;

	bool ret_value = aac_SeekTo1Frame() != -1;
	if (!ret_value)  this->aac_StopPlay();

	return ret_value;
}

unsigned int  taac::aac_GetFrames(char * &sbuf)
{
	unsigned int ret_value 		  = 0;
	int 		 frames_from_file = 0;

	char headers[c_header_size+1];
	int  buf_length = 0;
	int	 pos 		= 0;
	int  bytes_to_read = 0;


	while (frames_from_file < frames_to_read)
	{
		if(send_frame_number >= total_frames) 	break;

		memset(headers, 0, c_header_size);

		buf_length = fread(headers, sizeof(char), c_header_size, fs);
		if (buf_length < c_header_size)
		{
			break;
		}

		buf_length = aac_IsValidFrameHeader(headers);
		if (!buf_length)
		{
			break;
		}

		/* copy frame header to output buffer */
		memcpy(sbuf+pos, headers, c_header_size);
		pos += c_header_size;

		bytes_to_read = buf_length - c_header_size;

		buf_length = fread(sbuf+pos, sizeof(char), bytes_to_read, fs);

		if (buf_length < bytes_to_read)
		{
			break;
		}

		pos += buf_length;
		frames_from_file++;
		ret_value = pos;
	}

	send_frame_number += frames_from_file;

	return ret_value;
}

void taac::close_fs()
{
	if (fs)
	{
		try { fclose(fs); fs = NULL; }
		catch(...) {}
	}

}

std::string taac::aac_GetMetadata(char* aac_filename)
{
	 const int  b_size   = 2000;

	 char *f = (char*) malloc(strlen(aac_filename) + 3);
	 if (!f) return NULL;

	 strcpy(f, aac_filename);

	 char *ptr   = strrchr(f, '.'),
		  *ptr_e = NULL;

	 if (ptr) strcpy(ptr+1, "cue");
	 else strcat(f, "cue");

	 std::string meta = "";

	 if (common::FileExists(f))
	 {
		 char *str      = (char*) malloc(b_size);
      	 FILE *fd       = fopen(f, "r+");
      	 char *title    = (char*) malloc(10);

      	 bool  fl_title = false;

		 if (fd && str)
		 {
			 while(!feof(fd))
			 {

					if ( fgets(str, b_size, fd) )
					{
						str = common::xtrim(str);
						memcpy(title, str, 5);
						title = common::xstrupper(title);
						fl_title = memcmp(title, "TRACK", 5) == 0;
					}

					if (!fl_title) continue;

					while(!feof(fd))
					{

						if ( fgets(str, b_size, fd) )
						{
							str = common::xtrim(str);
							memcpy(title, str, 9);
							title = common::xstrupper(title);

							if (memcmp(title, "TRACK", 5) == 0) break;

							if (memcmp(title, "TITLE", 5) == 0 || memcmp(title, "PERFORMER", 9) == 0)
							{
								ptr   = strchr (str, '"');
								ptr_e = strrchr(str, '"') - 1;

								if (ptr)
								{
									if (meta.size()) meta.append(", ");
									meta.append(ptr+1, ptr_e-ptr);
								}
							}

						}
					}
			 }
		 }	//if (fd && str)

		 free(title);
		 if (fd) fclose(fd);
		 free(str);

		 free(f);
	 }

	return meta;
}
