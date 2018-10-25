/*
 * aac.h
 *
 *  Created on: 05.02.2016
 *      Author: irina
 *
Structure

AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)

AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)

Header consists of 7 or 9 bytes (without or with CRC).

Letter	Length (bits)	Description
A	12	syncword 0xFFF, all bits must be 1
B	1	MPEG Version: 0 for MPEG-4, 1 for MPEG-2
C	2	Layer: always 0
D	1	protection absent, Warning, set to 1 if there is no CRC and 0 if there is CRC

E	2	profile, the MPEG-4 Audio Object Type minus 1
F	4	MPEG-4 Sampling Frequency Index (15 is forbidden)
G	1	private bit, guaranteed never to be used by MPEG, set to 0 when encoding, ignore when decoding
H	3	MPEG-4 Channel Configuration (in the case of 0, the channel configuration is sent via an inband PCE)
I	1	originality, set to 0 when encoding, ignore when decoding
J	1	home, set to 0 when encoding, ignore when decoding
K	1	copyrighted id bit, the next bit of a centrally registered copyright identifier, set to 0 when encoding, ignore when decoding
L	1	copyright id start, signals that this frame's copyright id bit is the first bit of the copyright id, set to 0 when encoding, ignore when decoding
M	13	frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)
O	11	Buffer fullness
P	2	Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame
Q	16	CRC if protection absent is 0

 */

#ifndef AAC_H_
#define AAC_H_

#include <stdio.h>
#include <string>

class taac
{
public:
	taac  *pAAc;

	taac(size_t b_size) : buf_size(b_size), c_header_size(7), fs(NULL) {};
	virtual ~taac();
	int  aac_SeekTo1Frame(); /* возвращает начало заголовка */
	bool aac_IsValidFrameHeader(char*, int& , int&, int&, int&);
	int  aac_IsValidFrameHeader(char*);

	int  aac_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time);
	unsigned int  aac_GetFrames(char * &buf);
	bool  		  aac_StartPlay();
	void  		  aac_StopPlay() { close_fs(); }
	std::string	  aac_GetMetadata(char*);
private:
    size_t 		buf_size;
    const char  c_header_size;
	FILE*		fs;
	int 		send_frame_number;
    int 		frames_to_read;
    int			total_frames;
	inline int  get_frame_length(unsigned char, unsigned char, unsigned char);
	void close_fs();
};

extern taac 	*ptr_aac;
extern bool 	 aac_create(size_t);
extern void 	 aac_free();
extern int 		 aac_GetFileInfo(char* filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time);
extern unsigned int aac_GetFrames(char * &);
extern bool  	 aac_StartPlay();
extern void 	 aac_StopPlay();
extern std::string aac_Metadata(char*);

#endif /* AAC_H_ */
