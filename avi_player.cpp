/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "avi_player.h"
#include "file.h"
#include "mixer.h"
#include "systemstub.h"

bool AVI_Demuxer::open(File *f) {
	_f = f;
	_recordsListSize = 0;
	_chunkData = 0;
	_chunkDataSize = 0;
	return _f != 0 && readHeader();
}

void AVI_Demuxer::close() {
	_f = 0;
	free(_chunkData);
	_chunkData = 0;
}

bool AVI_Demuxer::readHeader_avih() {
	uint8_t hdr[kSizeOfChunk_avih];
	_f->read(hdr, kSizeOfChunk_avih);
	_frameRate = 1000000 / READ_LE_UINT32(hdr);
	_frames = READ_LE_UINT32(hdr + 16);
	_streams = READ_LE_UINT32(hdr + 24);
	_width = READ_LE_UINT32(hdr + 32);
	_height = READ_LE_UINT32(hdr + 36);
	return _streams == 2 && _width == AVI_Player::kDefaultFrameWidth && _height == AVI_Player::kDefaultFrameHeight;
}

bool AVI_Demuxer::readHeader_strh() {
	uint8_t hdr[kSizeOfChunk_strh];
	_f->read(hdr, kSizeOfChunk_strh);
	if (memcmp(hdr, "auds", 4) == 0 && READ_LE_UINT32(hdr + 4) == 0) {
		_audioBufferSize = READ_LE_UINT32(hdr + 36);
		return true;
	}
	if (memcmp(hdr, "vids", 4) == 0 && memcmp(hdr + 4, "cvid", 4) == 0) {
		_videoBufferSize = READ_LE_UINT32(hdr + 36);
		return true;
	}
	return false;
}

bool AVI_Demuxer::readHeader_strf_auds() {
	uint8_t hdr[kSizeOfChunk_waveformat];
	_f->read(hdr, kSizeOfChunk_waveformat);
	int formatTag = READ_LE_UINT16(hdr);
	int channels = READ_LE_UINT16(hdr + 2);
	int sampleRate  = READ_LE_UINT32(hdr + 4);
	int bitsPerSample = READ_LE_UINT16(hdr + 14);
	return formatTag == 1 && channels == 1 && sampleRate == 44100 && bitsPerSample == 8;
}

bool AVI_Demuxer::readHeader_strf_vids() {
	uint8_t hdr[kSizeOfChunk_bitmapinfo];
	_f->read(hdr, kSizeOfChunk_bitmapinfo);
	int width = READ_LE_UINT32(hdr + 4);
	int height = READ_LE_UINT32(hdr + 8);
	int planes = READ_LE_UINT16(hdr + 12);
	int bitDepth = READ_LE_UINT16(hdr + 14);
	return width == _width && height == _height && planes == 1 && bitDepth == 24;
}

bool AVI_Demuxer::readHeader() {
	_frames = 0;
	_width = _height = 0;
	_streams = 0;
	_frameRate = 0;

	char tag[4];
	_f->seek(8); // skip RIFF header
	_f->read(tag, 4);
	if (memcmp(tag, "AVI ", 4) == 0) {
		bool readHdrLoop = true;
		while (readHdrLoop) {
			_f->read(tag, 4);
			int len = _f->readUint32LE();
			assert((len & 1) == 0);
			if (memcmp(tag, "LIST", 4) == 0) {
				_f->read(tag, 4);
				if (memcmp(tag, "movi", 4) == 0) {
					_chunkDataSize = MAX(_videoBufferSize, _audioBufferSize);
					_chunkData = (uint8_t *)malloc(_chunkDataSize);
					if (!_chunkData) {
						warning("Unable to allocate %d bytes", _chunkDataSize);
						return false;
					}
					return true;
				}
			} else if (memcmp(tag, "avih", 4) == 0 && len == kSizeOfChunk_avih) {
				readHdrLoop = readHeader_avih();
			} else if (memcmp(tag, "strh", 4) == 0 && len == kSizeOfChunk_strh) {
				readHdrLoop = readHeader_strh();
			} else if (memcmp(tag, "strf", 4) == 0 && len == kSizeOfChunk_waveformat) {
				readHdrLoop = readHeader_strf_auds();
			} else if (memcmp(tag, "strf", 4) == 0 && len == kSizeOfChunk_bitmapinfo) {
				readHdrLoop = readHeader_strf_vids();
			} else {
				_f->seek(len, SEEK_CUR);
			}
		}
	}
	return false;
}

bool AVI_Demuxer::readNextChunk(AVI_Chunk &chunk) {
	char tag[12];
	assert(_recordsListSize >= 0);
	if (_recordsListSize == 0) {
		_f->read(tag, 12); // 'LIST', size, 'rec '
		_recordsListSize = READ_LE_UINT32(tag + 4) - 4;
	}
	_f->read(tag, 8);
	int len = READ_LE_UINT32(tag + 4);
	len = (len + 1) & ~1;
	_recordsListSize -= len + 8;
	if (tag[2] == 'w' && tag[3] == 'b') {
		chunk.type = kChunkAudioType;
	} else if (tag[2] == 'd' && tag[3] == 'c') {
		chunk.type = kChunkVideoType;
	} else {
		_f->seek(len, SEEK_CUR);
		return false;
	}
	assert(len <= _chunkDataSize);
	_f->read(_chunkData, len);
	chunk.data = _chunkData;
	chunk.dataSize = len;
	return true;
}

static void SET_YUV_V4(uint8_t *dst, uint8_t y1, uint8_t y2, uint8_t u, uint8_t v) {
	dst[0] = u;
	dst[1] = y1;
	dst[2] = v;
	dst[3] = y2;
}

void Cinepak_Decoder::decodeFrameV4(Cinepak_YUV_Vector *v0, Cinepak_YUV_Vector *v1, Cinepak_YUV_Vector *v2, Cinepak_YUV_Vector *v3) {
	uint8_t *p = _yuvFrame + _yPos * _yuvPitch + _xPos * 2;

	SET_YUV_V4(&p[0], v0->y[0], v0->y[1], v0->u, v0->v);
	SET_YUV_V4(&p[4], v1->y[0], v1->y[1], v1->u, v1->v);
	p += _yuvPitch;
	SET_YUV_V4(&p[0], v0->y[2], v0->y[3], v0->u, v0->v);
	SET_YUV_V4(&p[4], v1->y[2], v1->y[3], v1->u, v1->v);
	p += _yuvPitch;
	SET_YUV_V4(&p[0], v2->y[0], v2->y[1], v2->u, v2->v);
	SET_YUV_V4(&p[4], v3->y[0], v3->y[1], v3->u, v3->v);
	p += _yuvPitch;
	SET_YUV_V4(&p[0], v2->y[2], v2->y[3], v2->u, v2->v);
	SET_YUV_V4(&p[4], v3->y[2], v3->y[3], v3->u, v3->v);
}

static void SET_YUV_V1(uint8_t *dst, uint8_t y, uint8_t u, uint8_t v) {
	dst[0] = u;
	dst[1] = y;
	dst[2] = v;
	dst[3] = y;
}

void Cinepak_Decoder::decodeFrameV1(Cinepak_YUV_Vector *v) {
	uint8_t *p = _yuvFrame + _yPos * _yuvPitch + _xPos * 2;

	SET_YUV_V1(&p[0], v->y[0], v->u, v->v);
	SET_YUV_V1(&p[4], v->y[1], v->u, v->v);
	p += _yuvPitch;
	SET_YUV_V1(&p[0], v->y[0], v->u, v->v);
	SET_YUV_V1(&p[4], v->y[1], v->u, v->v);
	p += _yuvPitch;
	SET_YUV_V1(&p[0], v->y[2], v->u, v->v);
	SET_YUV_V1(&p[4], v->y[3], v->u, v->v);
	p += _yuvPitch;
	SET_YUV_V1(&p[0], v->y[2], v->u, v->v);
	SET_YUV_V1(&p[4], v->y[3], v->u, v->v);
}

void Cinepak_Decoder::decodeVector(Cinepak_YUV_Vector *v) {
	for (int i = 0; i < 4; ++i) {
		v->y[i] = readByte();
	}
	v->u = 128 + readByte();
	v->v = 128 + readByte();
}

void Cinepak_Decoder::decode(const uint8_t *data, int dataSize) {
	_data = data;

	const uint8_t flags = readByte();
	_data += 3;
	_w = readWord();
	_h = readWord();
	const int strips = readWord();
	assert(_w == AVI_Player::kDefaultFrameWidth && _h == AVI_Player::kDefaultFrameHeight && strips == MAX_STRIPS);

	_xPos = _yPos = 0;
	int yMax = 0;

	for (int strip = 0; strip < strips; ++strip) {
		if (strip != 0 && (flags & 1) == 0) {
			memcpy(&_vectors[kCinepakV1][strip][0], &_vectors[kCinepakV1][strip - 1][0], sizeof(Cinepak_YUV_Vector) * MAX_VECTORS);
			memcpy(&_vectors[kCinepakV4][strip][0], &_vectors[kCinepakV4][strip - 1][0], sizeof(Cinepak_YUV_Vector) * MAX_VECTORS);
		}

		readWord();
		int size = readWord();
		readWord();
		readWord();
		const int stripHeight = readWord();
		readWord();

		size -= 12;
		_xPos = 0;
		yMax += stripHeight;
		int v, i;
		while (size > 0) {
			int chunkType = readWord();
			int chunkSize = readWord();
			size -= chunkSize;
			chunkSize -= 4;

			switch (chunkType) {
			case 0x2000:
			case 0x2200:
				v = (chunkType == 0x2200) ? kCinepakV1 : kCinepakV4;
				for (i = 0; i < chunkSize / 6; ++i) {
					decodeVector(&_vectors[v][strip][i]);
				}
				chunkSize = 0;
				break;
			case 0x2100:
			case 0x2300:
				v = (chunkType == 0x2300) ? kCinepakV1 : kCinepakV4;
				i = 0;
				while (chunkSize > 0) {
					const uint32_t mask = readLong();
					chunkSize -= 4;
					for (int bit = 0; bit < 32; ++bit) {
						if (mask & (1 << (31 - bit))) {
							decodeVector(&_vectors[v][strip][i]);
							chunkSize -= 6;
						}
						++i;
					}
				}
				break;
			case 0x3000:
				while (chunkSize > 0 && _yPos < yMax) {
					uint32_t mask = readLong();
					chunkSize -= 4;
					for (int bit = 0; bit < 32 && _yPos < yMax; ++bit) {
						if (mask & (1 << (31 - bit))) {
							Cinepak_YUV_Vector *v0 = &_vectors[kCinepakV4][strip][readByte()];
							Cinepak_YUV_Vector *v1 = &_vectors[kCinepakV4][strip][readByte()];
							Cinepak_YUV_Vector *v2 = &_vectors[kCinepakV4][strip][readByte()];
							Cinepak_YUV_Vector *v3 = &_vectors[kCinepakV4][strip][readByte()];
							chunkSize -= 4;
							decodeFrameV4(v0, v1, v2, v3);
						} else {
							Cinepak_YUV_Vector *v0 = &_vectors[kCinepakV1][strip][readByte()];
							--chunkSize;
							decodeFrameV1(v0);
						}
						_xPos += 4;
						if (_xPos >= _w) {
							_xPos = 0;
							_yPos += 4;
						}
					}
				}
				break;
			case 0x3100:
				while (chunkSize > 0 && _yPos < yMax) {
					uint32_t mask = readLong();
					chunkSize -= 4;
					for (int bit = 0; bit < 32 && chunkSize >= 0 && _yPos < yMax; ) {
						if (mask & (1 << (31 - bit))) {
							++bit;
							if (bit == 32) {
								assert(chunkSize >= 4);
								mask = readLong();
								chunkSize -= 4;
								bit = 0;
							}
							if (mask & (1 << (31 - bit))) {
								Cinepak_YUV_Vector *v0 = &_vectors[kCinepakV4][strip][readByte()];
								Cinepak_YUV_Vector *v1 = &_vectors[kCinepakV4][strip][readByte()];
								Cinepak_YUV_Vector *v2 = &_vectors[kCinepakV4][strip][readByte()];
								Cinepak_YUV_Vector *v3 = &_vectors[kCinepakV4][strip][readByte()];
								chunkSize -= 4;
								decodeFrameV4(v0, v1, v2, v3);
							} else {
								Cinepak_YUV_Vector *v0 = &_vectors[kCinepakV1][strip][readByte()];
								--chunkSize;
								decodeFrameV1(v0);
							}
						}
						++bit;
						_xPos += 4;
						if (_xPos >= _w) {
							_xPos = 0;
							_yPos += 4;
						}
					}
				}
				break;
			case 0x3200:
				while (chunkSize > 0 && _yPos < yMax) {
					Cinepak_YUV_Vector *v0 = &_vectors[kCinepakV1][strip][readByte()];
					--chunkSize;
					decodeFrameV1(v0);
					_xPos += 4;
					if (_xPos >= _w) {
						_xPos = 0;
						_yPos += 4;
					}
				}
				break;
			}
			_data += chunkSize;
		}
	}
}

AVI_Player::AVI_Player(Mixer *mixer, SystemStub *stub)
	: _soundQueue(0), _soundTailQueue(0), _mixer(mixer), _stub(stub) {
}

AVI_Player::~AVI_Player() {
	while (_soundQueue) {
		AVI_SoundBufferQueue *next = _soundQueue->next;
		free(_soundQueue->buffer);
		free(_soundQueue);
		_soundQueue = next;
	}
}

void AVI_Player::play(File *f) {
	_soundQueue = 0;
	_soundQueuePreloadSize = 0;
	if (_demux.open(f)) {
		_stub->setYUV(true, _demux._width, _demux._height);
		_mixer->setMusicMix(this, AVI_Player::mixCallback);
		for (int i = 0; i < _demux._frames; ++i) {
			uint32_t nextFrameTimeStamp = _stub->getTimeStamp() + 1000 / _demux._frameRate;
			_stub->processEvents();
			if (_stub->_quit || _stub->_pi.enter) {
				_stub->_pi.enter = false;
				break;
			}
			AVI_Chunk chunk;
			while (_demux.readNextChunk(chunk)) {
				switch (chunk.type) {
				case kChunkAudioType:
					decodeAudioChunk(chunk);
					break;
				case kChunkVideoType:
					decodeVideoChunk(chunk);
					break;
				}
			}
			int diff = nextFrameTimeStamp - _stub->getTimeStamp();
			if (diff > 0) {
				_stub->sleep(diff);
			}
		}
		_mixer->setMusicMix(0, 0);
		_stub->setYUV(false, 0, 0);
		_demux.close();
	}
}

void AVI_Player::decodeAudioChunk(AVI_Chunk &c) {
	AVI_SoundBufferQueue *sbq = (AVI_SoundBufferQueue *)malloc(sizeof(AVI_SoundBufferQueue));
	if (sbq) {
		sbq->buffer = (uint8_t *)malloc(c.dataSize);
		if (sbq->buffer) {
			memcpy(sbq->buffer, c.data, c.dataSize);
			sbq->size = c.dataSize;
			sbq->offset = 0;
			sbq->next = 0;
		} else {
			free(sbq);
			sbq = 0;
		}
	}
	_stub->lockAudio();
	if (sbq) {
		if (!_soundQueue) {
			_soundQueue = sbq;
		} else {
			AVI_SoundBufferQueue *p = _soundTailQueue;
			assert(!p->next);
			p->next = sbq;
		}
		_soundTailQueue = sbq;
		if (_soundQueuePreloadSize < kSoundPreloadSize) {
			++_soundQueuePreloadSize;
		}
	}
	_stub->unlockAudio();
}

void AVI_Player::decodeVideoChunk(AVI_Chunk &c) {
	_cinepak._yuvFrame = _stub->lockYUV(&_cinepak._yuvPitch);
	if (_cinepak._yuvFrame) {
		_cinepak.decode(c.data, c.dataSize);
	}
	_stub->unlockYUV();
}

void AVI_Player::mix(int16_t *buf, int samples) {
	if (_soundQueuePreloadSize < kSoundPreloadSize) {
		return;
	}
	while (_soundQueue && samples > 0) {
		int sample = (_soundQueue->buffer[_soundQueue->offset] << 8) ^ 0x8000;
		*buf++ = (int16_t)sample;
		*buf++ = (int16_t)sample;
		_soundQueue->offset += 2; // skip every second sample (44Khz stream vs 22Khz mixer)
		if (_soundQueue->offset >= _soundQueue->size) {
			AVI_SoundBufferQueue *next = _soundQueue->next;
			free(_soundQueue->buffer);
			free(_soundQueue);
			_soundQueue = next;
		}
		--samples;
	}
	if (!_soundQueue) {
		_soundTailQueue = 0;
	}
	if (samples > 0) {
		warning("AVI_Player::mix() soundQueue underrun %d", samples);
	}
}

void AVI_Player::mixCallback(void *param, uint8_t *buf, int len) {
	memset(buf, 0, len);
	assert((len & 3) == 0);
	((AVI_Player *)param)->mix((int16_t *)buf, len / 4);
}
