/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "decoder.h"
#ifdef BERMUDA_ZLIB
#define ZLIB_CONST
#include <zlib.h>
#endif

struct BitStream {
	const uint8_t *_src;
	uint16_t _bits;
	int _len;

	void reset(const uint8_t *src) {
		_src = src;
		_bits = READ_LE_UINT16(_src); _src += 2;
		_len = 16;
	}

	bool getNextBit() {
		const bool bit = (_bits & (1 << (16 - _len))) != 0;
		--_len;
		if (_len == 0) {
			_bits = READ_LE_UINT16(_src); _src += 2;
			_len = 16;
		}
		return bit;
	}

	uint8_t getNextByte() {
		uint8_t b = *_src++;
		return b;
	}

	uint16_t getNextWord() {
		uint16_t w = READ_LE_UINT16(_src); _src += 2;
		return w;
	}
};

static const bool kCheckCompressedData = true;

int decodeLzss(const uint8_t *src, uint8_t *dst) {
	BitStream stream;
	int outputSize = READ_LE_UINT32(src); src += 4;
	int inputSize = READ_LE_UINT32(src); src += 4;
	for (const uint8_t *compressedData = src; inputSize != 0; compressedData += 0x1000) {
		int decodeSize = inputSize;
		if (decodeSize > 256) {
			decodeSize = 256;
		}
		inputSize -= decodeSize;
		if (kCheckCompressedData) {
			src = compressedData;
			const uint16_t crc = READ_LE_UINT16(src); src += 2;
			uint16_t sum = 0;
			for (int i = 0; i < decodeSize * 8 - 1; ++i) {
				sum = ((sum & 1) << 15) | (sum >> 1);
				sum ^= READ_LE_UINT16(src); src += 2;
			}
			if (sum != crc) {
				error("Invalid checksum, expected 0x%X got 0x%X", crc, sum);
			}
		}
		src = compressedData + 2;
		stream.reset(src);
		while (1) {
			if (stream.getNextBit()) {
				*dst++ = stream.getNextByte();
				continue;
			}
			uint16_t size = 0;
			uint16_t offset = 0;
			if (stream.getNextBit()) {
				uint16_t code = stream.getNextWord();
				offset = 0xE000 | ((code >> 3) & 0x1F00) | (code & 0xFF);
				if (code & 0x700) {
					size = ((code >> 8) & 7) + 2;
				} else {
					code = stream.getNextByte();
					if (code == 0) {
						return outputSize;
					} else if (code == 1) {
						continue;
					} else if (code == 2) {
						break;
					} else {
						size = code + 1;
					}
				}
			} else {
				for (int i = 0; i < 2; ++i) {
					size <<= 1;
					if (stream.getNextBit()) {
						size |= 1;
					}
				}
				size += 2;
				offset = 0xFF00 | stream.getNextByte();
			}
			while (size--) {
				*dst = *(dst + (int16_t)offset);
				++dst;
			}
		}
	}
	return outputSize;
}

int decodeZlib(const uint8_t *src, uint8_t *dst) {
#ifdef BERMUDA_ZLIB
	z_stream s;
	memset(&s, 0, sizeof(s));
	int ret = inflateInit(&s);
	if (ret == Z_OK) {
		s.next_in = src + 8;
		s.avail_in = READ_LE_UINT32(src);
		s.next_out = dst;
		s.avail_out = READ_LE_UINT32(src + 4);
		if (inflate(&s, Z_NO_FLUSH) == Z_STREAM_END) {
			ret = s.total_out;
		} else {
			ret = 0;
		}
		inflateEnd(&s);
		return ret;
	}
#else
	warning("No decoder for zlib data");
#endif
	return 0;
}
