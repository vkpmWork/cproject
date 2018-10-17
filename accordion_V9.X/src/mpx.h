/*
 * mpx.h
 *
 *  Created on: 15.03.2016
 *      Author: irina
 */

#ifndef MPX_H_
#define MPX_H_

#include <stdio.h>
#include "common.h"

class tmpx
{
public:
	tmpx  *pMpx;

	tmpx(size_t b_size) : buf_size(b_size), c_header_size(4) {  pInfo = (TInfo *)calloc(1, sizeof(TInfo));
																fs = NULL;
																sizeof_TInfo = sizeof(TInfo);
															 }
	virtual ~tmpx();

	int  	mpx_GetFileInfo (char* filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time);
	int  	mpx_SeekTo1Frame(); /* возвращает начало заголовка */

	bool  	mpx_StartPlay();
	void  	mpx_StopPlay () {if (fs) { fclose(fs); fs = NULL; } }
	unsigned int  mpx_GetFrames (char * &buf);
	std::string   mpx_GetMetadata(char*);

private:
    size_t 		buf_size;
    const char  c_header_size;
	FILE*		fs;
	int 		send_frame_number;
    int 		frames_to_read;
    int			total_frames;
    int			sizeof_TInfo;
	struct TInfo
	{
		char 		  ID[3]; // сигнатура тэга, должна быть равна "ID3 или TAG"
		unsigned char Version; // основной байт версии (major)
		unsigned char Revision; // дополнительный байт версии (minor)
		unsigned char Flags; // флаги
		unsigned char Size [4]; // размер тэга без заголовков
	} *pInfo;

	inline int  get_frame_length(unsigned char, unsigned char, unsigned char);
	int		    mpx_GetFrameSize(char*);
	int  		mpx_IsValidFrameHeader(char*);
	inline void close_fs();
	int         GetStartPos(FILE*);
	std::string Metadata_V1(char*);
	std::string Metadata_V2(char*, int);
};

extern tmpx 	*ptr_mpx;
extern bool 	 mpx_create(size_t);
extern void 	 mpx_free();
extern int 		 mpx_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time);
extern unsigned int mpx_GetFrames(char * &);
extern bool  	 mpx_StartPlay();
extern void 	 mpx_StopPlay();
extern std::string mpx_Metadata(char*);
#endif /* MPX_H_ */
