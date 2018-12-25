#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  		   char Riff[5];
  		   char WAVEfmt[9];
  		   char Cdata[5];
  		   unsigned long sizeOfFile;
  		   unsigned long nSamplesPerSec;
  		   unsigned long navgBytesPerSec;
  		   unsigned long sizeOfData;
		   unsigned short nBlockAlign;
		   unsigned long sizeOfFmt;
		   unsigned short wFormatTag;
		   unsigned short nChannels;
		   unsigned short nBitPerSample;
}WAVEHEADFMT;

int ReadWaveHead( WAVEHEADFMT *wav_in, FILE *file_in ) 
{
    int tmp;

	fseek( file_in, 0, SEEK_SET );
	if ( fread ( wav_in->Riff, 1, 4, file_in ) < 4 ) return -1;
	if ( fread ( &(wav_in->sizeOfFile), 1, 4, file_in) < 4 ) return -1;
	if ( fread ( wav_in->WAVEfmt, 1, 8, file_in ) < 8 ) return -1;
	if ( fread ( &(wav_in->sizeOfFmt), 1, 4, file_in ) < 4 ) return -1;
	if ( fread ( &(wav_in->wFormatTag), 1, 2, file_in ) < 2 ) return -1;
	if ( fread ( &(wav_in->nChannels), 1, 2, file_in ) < 2 ) return -1;
	if ( fread ( &(wav_in->nSamplesPerSec), 1, 4, file_in ) < 4 ) return -1;
	if ( fread ( &(wav_in->navgBytesPerSec), 1, 4, file_in ) < 4 ) return -1;
	if ( fread ( &(wav_in->nBlockAlign), 1, 2, file_in ) < 2 ) return -1;
	if ( fread ( &(wav_in->nBitPerSample), 1, 2, file_in ) < 2 ) return -1;
	if ( fread ( wav_in->Cdata, 1, 4, file_in) < 4 ) return -1;
	if ( fread ( &(wav_in->sizeOfData), 1, 4, file_in ) < 4 ) return -1;

	tmp = ftell( file_in );
	fseek( file_in, 0, SEEK_END );
	wav_in->sizeOfFile = ftell( file_in );
	wav_in->sizeOfData = wav_in->sizeOfFile - 44;

	fseek( file_in, tmp, SEEK_SET );

	return 0;
}

int WriteWaveHead( WAVEHEADFMT *wav_out, FILE *file_out ) 
{
	
	fseek( file_out, 0, SEEK_SET );
	if ( fwrite ( wav_out->Riff, 1, 4, file_out ) < 4 ) return -1;
	if ( fwrite ( &(wav_out->sizeOfFile), 1, 4, file_out) < 4 ) return -1;
	if ( fwrite ( wav_out->WAVEfmt, 1, 8, file_out ) < 8 ) return -1;
	if ( fwrite ( &(wav_out->sizeOfFmt), 1, 4, file_out ) < 4 ) return -1;
	if ( fwrite ( &(wav_out->wFormatTag), 1, 2, file_out ) < 2 ) return -1;
	if ( fwrite ( &(wav_out->nChannels), 1, 2, file_out ) < 2 ) return -1;
	if ( fwrite ( &(wav_out->nSamplesPerSec), 1, 4, file_out ) < 4 ) return -1;
	if ( fwrite ( &(wav_out->navgBytesPerSec), 1, 4, file_out ) < 4 ) return -1;
	if ( fwrite ( &(wav_out->nBlockAlign), 1, 2, file_out ) < 2 ) return -1;
	if ( fwrite ( &(wav_out->nBitPerSample), 1, 2, file_out ) < 2 ) return -1;
	if ( fwrite ( wav_out->Cdata, 1, 4, file_out) < 4 ) return -1;
	if ( fwrite ( &(wav_out->sizeOfData), 1, 4, file_out ) < 4 ) return -1;

	return 0;
}

int PrepareWaveHead( WAVEHEADFMT *wav_head, 
					 unsigned short nChannels,
					 unsigned long nSamplesPerSec,
					 unsigned short nBitPerSample,
					 unsigned long sizeOfData
				   )
{
	strcpy ( wav_head->Riff, "RIFF" );
	strcpy ( wav_head->WAVEfmt, "WAVEfmt " );
	strcpy ( wav_head->Cdata, "data" );

	wav_head->sizeOfFmt = 16;
	wav_head->wFormatTag = 1;
	wav_head->nChannels = nChannels;
	wav_head->nSamplesPerSec = nSamplesPerSec;
	wav_head->nBitPerSample = nBitPerSample;
	wav_head->sizeOfData = sizeOfData;
	wav_head->sizeOfFile = wav_head->sizeOfData + 44;
	wav_head->nBlockAlign = wav_head->nChannels * ( wav_head->nBitPerSample / 8 );
	wav_head->navgBytesPerSec= wav_head->nSamplesPerSec * wav_head->nBlockAlign;
	
	return 0;
}