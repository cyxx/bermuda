
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
	bool _carry;
	uint16_t _bits;
	int _len;

	void reset(const uint8_t *src) {
		_src = src;
		_carry = false;
		_bits = READ_LE_UINT16(_src); _src += 2;
		_len = 16;
	}

	bool getNextBit() {
		_carry = (_bits & 1) == 1;
		_bits >>= 1;
		--_len;
		if (_len == 0) {
			_bits = READ_LE_UINT16(_src); _src += 2;
			_len = 16;
		}
		return _carry;
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

static void writeBMP(const char *path, uint8_t *buf, int size) {
	fprintf(stdout, "write '%s' size %d\n", path, size);
	char filename[16];
	const char *sep = strrchr(path, '/');
	strcpy(filename, sep ? sep + 1 : path);
	char *ext = strrchr(filename, '.');
	if (ext) {
		strcpy(ext + 1, "BMP");
	}
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		// bmp_file_header_t
		fwriteUint16LE(fp, 0x4D42); // type
		fwriteUint32LE(fp, 14 + size - 4); // size
		fwriteUint16LE(fp, 0); // reserved1
		fwriteUint16LE(fp, 0); // reserved2
		fwriteUint32LE(fp, 14 + 40 + 4 * 256); // off_bits
		// bmp_info_header_t and palette
		static const int len = 40 + 4 * 256;
		fwrite(buf, 1, len, fp);
		// bitmap_bits
		fwrite(buf + len + 4, 1, size - len - 4, fp);
		fclose(fp);
	}
}

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		FILE *fp = fopen(argv[i], "rb");
		if (fp) {
			const int bitmapSize = decodeWGP(fp);
			fclose(fp);
			writeBMP(argv[1], _bitmapBuffer, bitmapSize);
		}
	}
	return 0;
}
