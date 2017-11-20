/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef AVI_PLAYER_H__
#define AVI_PLAYER_H__

#include "intern.h"

enum AVI_ChunkType {
	kChunkAudioType,
	kChunkVideoType
};

struct AVI_Chunk {
	AVI_ChunkType type;
	uint8_t *data;
	int dataSize;
};

struct File;

struct AVI_Demuxer {
	enum {
		kSizeOfChunk_avih = 56,
		kSizeOfChunk_strh = 56,
		kSizeOfChunk_waveformat = 16,
		kSizeOfChunk_bitmapinfo = 40
	};

	bool open(File *f);
	void close();

	bool readHeader();
	bool readHeader_avih();
	bool readHeader_strh();
	bool readHeader_strf_auds();
	bool readHeader_strf_vids();

	bool readNextChunk(AVI_Chunk &chunk);

	int _frames;
	int _width, _height;
	int _streams;
	int _frameRate;

	File *_f;
	int _recordsListSize;
	uint8_t *_chunkData;
	int _chunkDataSize;
	int _audioBufferSize;
	int _videoBufferSize;
};

struct Mixer;
struct SystemStub;

struct Cinepak_YUV_Vector {
	uint8_t y[4];
	uint8_t u, v;
};

enum {
	kCinepakV1 = 0,
	kCinepakV4 = 1
};

struct Cinepak_Decoder {
	enum {
		MAX_STRIPS = 2,
		MAX_VECTORS = 256
	};

	uint8_t readByte() {
		return *_data++;
	}

	uint16_t readWord() {
		uint16_t value = READ_BE_UINT16(_data); _data += 2;
		return value;
	}

	uint32_t readLong() {
		uint32_t value = READ_BE_UINT32(_data); _data += 4;
		return value;
	}

	void decodeFrameV4(Cinepak_YUV_Vector *v0, Cinepak_YUV_Vector *v1, Cinepak_YUV_Vector *v2, Cinepak_YUV_Vector *v3);
	void decodeFrameV1(Cinepak_YUV_Vector *v);
	void decodeVector(Cinepak_YUV_Vector *v);
	void decode(const uint8_t *data, int dataSize);

	const uint8_t *_data;
	Cinepak_YUV_Vector _vectors[2][MAX_STRIPS][MAX_VECTORS];
	int _w, _h;
	int _xPos, _yPos, _yMax;

	uint8_t *_yuvFrame;
	int _yuvPitch;
};

struct AVI_SoundBufferQueue {
	uint8_t *buffer;
	int size;
	int offset;
	AVI_SoundBufferQueue *next;
};

struct AVI_Player {
	enum {
		kDefaultFrameWidth = 320,
		kDefaultFrameHeight = 200,
		kSoundPreloadSize = 4,
	};

	AVI_Player(Mixer *mixer, SystemStub *stub);
	~AVI_Player();

	void play(File *f);
	void readNextFrame();
	void decodeAudioChunk(AVI_Chunk &c);
	void decodeVideoChunk(AVI_Chunk &c);
	void mix(int16_t *buf, int samples);
	static void mixCallback(void *param, uint8_t *buf, int len);

	AVI_Demuxer _demux;
	AVI_SoundBufferQueue *_soundQueue, *_soundTailQueue;
	int _soundQueuePreloadSize;
	Cinepak_Decoder _cinepak;
	Mixer *_mixer;
	SystemStub *_stub;
};

#endif // AVI_PLAYER_H__
