
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define ZLIB_CONST
#include <zlib.h>

static uint16_t READ_LE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[1] << 8) | b[0];
}

static uint32_t READ_LE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

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

static int decodeLzss(const uint8_t *src, uint8_t *dst) {
	BitStream stream;
	int outputSize = READ_LE_UINT32(src); src += 4;
	int inputSize = READ_LE_UINT32(src); src += 4;
	for (const uint8_t *compressedData = src; inputSize != 0; compressedData += 0x1000) {
		int decodeSize = inputSize;
		if (decodeSize > 256) {
			decodeSize = 256;
		}
		inputSize -= decodeSize;
		if (1) {
			src = compressedData;
			const uint16_t crc = READ_LE_UINT16(src); src += 2;
			uint16_t sum = 0;
			for (int i = 0; i < decodeSize * 8 - 1; ++i) {
				sum = ((sum & 1) << 15) | (sum >> 1);
				sum ^= READ_LE_UINT16(src); src += 2;
			}
			if (sum != crc) {
				fprintf(stderr, "Invalid checksum, expected 0x%X got 0x%X\n", crc, sum);
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

static const int kBitmapSize = 40 + 256 * 4 + 640 * 480 + 256 * 4;

static uint8_t _bitmapBuffer[kBitmapSize];
static uint8_t _tempBuffer[kBitmapSize];

static uint16_t freadUint16LE(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return READ_LE_UINT16(buf);
}

static uint32_t freadUint32LE(FILE *fp) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return READ_LE_UINT32(buf);
}

static int decodeWGP(FILE *fp) {
	const uint16_t tag = freadUint16LE(fp);
	if (tag != 0x5057) {
		fprintf(stderr, "Invalid tag '0x%04x'\n", tag);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	int dataSize = ftell(fp);
	fseek(fp, 2, SEEK_SET);
	int len = 0;
	while (dataSize > 0) {
		const int sz = freadUint16LE(fp);
		dataSize -= 2;
		if (feof(fp)) {
			break;
		}
		if (sz != 0) {
			assert(sz < sizeof(_tempBuffer));
			fread(_tempBuffer, 1, sz, fp);
			const int decodedSize = decodeLzss(_tempBuffer, _bitmapBuffer + len);
			len += decodedSize;
			dataSize -= sz;
		}
	}
	return len;
}

static int decodeBMP(FILE *fp) {
	const uint16_t tag = freadUint16LE(fp);
	if (tag != 0x4D42) {
		fprintf(stderr, "Invalid tag '0x%04x'\n", tag);
		return 0;
	}
	const uint32_t size = freadUint32LE(fp);
	fseek(fp, 14, SEEK_SET);
	int count = fread(_bitmapBuffer, 1, 40 + 4 * 256, fp);
	if (count != 40 + 4 * 256) {
		fprintf(stdout, "fread() return %d\n", count);
		return 0;
	}
	const int x2 = READ_LE_UINT32(_bitmapBuffer + 4) - 1;
	const int y2 = READ_LE_UINT32(_bitmapBuffer + 8) - 1;
	fprintf(stdout, "BMP %d,%d size %d\n", x2, y2, size);
	_bitmapBuffer[count++] = x2 & 255;
	_bitmapBuffer[count++] = x2 >> 8;
	_bitmapBuffer[count++] = y2 & 255;
	_bitmapBuffer[count++] = y2 >> 8;
	int count2 = fread(_bitmapBuffer + count, 1, size - 14 - count, fp);
	return count + count2;
}

enum {
	kSprBufferRead,
	kSprBufferDecoded,
	kSprBufferDeflated,
};

static uint8_t _sprBuffer[3][0x10000];

static const int kMaxSprFrames = 1024;

static struct {
	uint8_t *data;
	uint16_t dataSize;
	uint8_t dim[10]; /* 5x uint16_t */
} _sprFrame[kMaxSprFrames];

static const int kMaxSpr = 256;

static int _sprCount[kMaxSpr];

static int decodeSPR(FILE *fp) {
	const uint16_t tag = freadUint16LE(fp);
	if (tag != 0x3553) {
		fprintf(stderr, "Invalid tag '0x%04x'\n", tag);
		return 0;
	}
	int spr = 0;
	int offset = 0;
	while (1) {
		const int count = freadUint16LE(fp);
		if (count == 0) {
			break;
		}
		assert(spr < kMaxSpr);
		_sprCount[spr] = count;
		++spr;
		assert(offset + count < kMaxSprFrames);
		for (int i = 0; i < count; ++i) {
			const int len  = freadUint16LE(fp);
			fread(_sprBuffer[kSprBufferRead], 1, len, fp);
			const int decodedSize = decodeLzss(_sprBuffer[kSprBufferRead], _sprBuffer[kSprBufferDecoded]);
			assert(decodedSize < 0x10000);
			uint8_t *p = (uint8_t *)malloc(decodedSize);
			if (p) {
				memcpy(p, _sprBuffer[kSprBufferDecoded], decodedSize);
				_sprFrame[offset + i].data = p;
				_sprFrame[offset + i].dataSize = decodedSize;
			}
			fread(_sprFrame[offset + i].dim, 1, 5 * 2, fp);
			const uint8_t *hdr = _sprFrame[offset + i].dim;
			const int w = READ_LE_UINT16(hdr + 2);
			const int h = READ_LE_UINT16(hdr + 4);
//			fprintf(stdout, "SPR %d/%d size %d,%d dim %d,%d\n", i, count, len, decodedSize, w, h);
		}
		offset += count;
	}
	return 0;
}

static int deflate(const uint8_t *in, int len, uint8_t *out, int outSize) {
	int ret;
	z_stream s;

	memset(&s, 0, sizeof(s));
	ret = deflateInit(&s, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK) {
		fprintf(stderr, "deflateInit() ret %d\n", ret);
		return -1;
	}
	s.avail_in = len;
	s.next_in = in;
	s.avail_out = outSize;
	s.next_out = out;
	ret = deflate(&s, Z_FINISH);
	if (ret != Z_STREAM_END) {
		fprintf(stderr, "deflate() ret %d\n", ret);
		return -1;
	}
	deflateEnd(&s);
	return s.total_out;
}

static void fwriteUint16LE(FILE *fp, uint16_t n) {
	fputc(n & 0xFF, fp);
	fputc(n >> 8, fp);
}

static void fwriteUint32LE(FILE *fp, uint32_t n) {
	fputc(n & 0xFF, fp);
	fputc(n >> 8, fp);
	fputc(n >> 16, fp);
	fputc(n >> 24, fp);
}

static void writeSPR(const char *path) {
	FILE *fp = fopen(path, "wb");
	if (fp) {
		fwriteUint16LE(fp, 0x355A); // '5Z'
		int offset = 0;
		for (int spr = 0; _sprCount[spr] != 0; ++spr) {
			fwriteUint16LE(fp, _sprCount[spr]);
			for (int i = 0; i < _sprCount[spr]; ++i) {
				if (0) { /* uncompressed */
					fwriteUint16LE(fp, _sprFrame[offset + i].dataSize);
					fwrite(_sprFrame[offset + i].data, 1, _sprFrame[offset + i].dataSize, fp);
				} else { /* zlib */
					const int size = deflate(_sprFrame[offset + i].data, _sprFrame[offset + i].dataSize, _sprBuffer[kSprBufferDeflated], 0x10000);
					assert(size + 8 < 0x10000);
					fwriteUint16LE(fp, size + 8);
					fwriteUint32LE(fp, size);
					fwriteUint32LE(fp, _sprFrame[offset + i].dataSize);
					fwrite(_sprBuffer[kSprBufferDeflated], 1, size, fp);
				}
				fwrite(_sprFrame[offset + i].dim, 1, 5 * 2, fp);
			}
			offset += _sprCount[spr];
		}
		fwriteUint16LE(fp, 0);
		fclose(fp);
	}
}

static void writeWGP(const char *path, uint8_t *buf, int size, bool saveAsBitmap = false) {
	FILE *fp = fopen(path, "wb");
	if (fp) {
		if (saveAsBitmap) {
			// bmp_file_header_t
			fwriteUint16LE(fp, 0x4D42); // type
			fwriteUint32LE(fp, 14 + size - 4); // size
			fwriteUint16LE(fp, 0); // reserved1
			fwriteUint16LE(fp, 0); // reserved2
			fwriteUint32LE(fp, 14 + 40 + 4 * 256); // off_bits
			// bmp_info_header_t and palette
			static const int len = 40 + 4 * 256;
			fwrite(buf, 1, len, fp);
			const int w = READ_LE_UINT16(buf + len);
			const int h = READ_LE_UINT16(buf + len + 2);
			fprintf(stdout, "write '%s' size %d dim %d,%d\n", path, size, w, h);
			// bitmap_bits
			fwrite(buf + len + 4, 1, size - len - 4, fp);
		} else {
			fwriteUint16LE(fp, 0x505A); // 'PZ'
			const int compressedSize = deflate(buf, size, _tempBuffer, sizeof(_tempBuffer));
			fwriteUint32LE(fp, compressedSize);
			fwriteUint32LE(fp, size);
			fwrite(_tempBuffer, 1, compressedSize, fp);
		}
		fclose(fp);
	}
}

int main(int argc, char *argv[]) {
	if (argc == 3) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			const char *ext = strrchr(argv[1], '.');
			if (ext) {
				if (strcasecmp(ext + 1, "WGP") == 0) {
					const int bitmapSize = decodeWGP(fp);
					writeWGP(argv[2], _bitmapBuffer, bitmapSize);
				} else if (strcasecmp(ext + 1, "BMP") == 0) {
					const int bitmapSize = decodeBMP(fp);
					writeWGP(argv[2], _bitmapBuffer, bitmapSize);
				} else if (strcasecmp(ext + 1, "SPR") == 0) {
					decodeSPR(fp);
					writeSPR(argv[2]);
				}
			}
			fclose(fp);
		}
	}
	return 0;
}
