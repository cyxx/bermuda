/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "file.h"

struct File_impl {
	bool _ioErr;
	File_impl() : _ioErr(false) {}
	virtual ~File_impl() {}
	virtual bool open(const char *path, const char *mode) { return false; }
	virtual void close() {}
	virtual uint32_t size() = 0;
	virtual uint32_t tell() = 0;
	virtual void seek(int offs, int origin) = 0;
	virtual uint32_t read(void *ptr, uint32_t len) = 0;
	virtual uint32_t write(void *ptr, uint32_t len) = 0;
};

struct StdioFile : File_impl {
	FILE *_fp;
	uint32_t _offset, _size;
	StdioFile() : _fp(0), _offset(0), _size(0) {}
	StdioFile(uint32_t offset, uint32_t size) : _fp(0), _offset(offset), _size(size) {}
	bool open(const char *path, const char *mode) {
		_ioErr = false;
		_fp = fopen(path, mode);
		if (_fp != 0) {
			if (_offset != 0) {
				fseek(_fp, _offset, SEEK_SET);
			}
			return true;
		}
		return false;
	}
	void close() {
		if (_fp) {
			fclose(_fp);
			_fp = 0;
		}
	}
	uint32_t size() {
		if (_size != 0) {
			return _size;
		}
		if (_fp) {
			int pos = ftell(_fp);
			fseek(_fp, 0, SEEK_END);
			_size = ftell(_fp);
			fseek(_fp, pos, SEEK_SET);
		}
		return _size;
	}
	uint32_t tell() {
		uint32_t pos = 0;
		if (_fp) {
			pos = ftell(_fp) - _offset;
		}
		return pos;
	}
	void seek(int offs, int origin) {
		if (_fp) {
			fseek(_fp, _offset + offs, origin);
		}
	}
	uint32_t read(void *ptr, uint32_t len) {
		uint32_t r = 0;
		if (_fp) {
			r = fread(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
		return r;
	}
	uint32_t write(void *ptr, uint32_t len) {
		uint32_t r = 0;
		if (_fp) {
			r = fwrite(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
		return r;
	}
};

struct ReadMemoryFile: File_impl {
	const uint8_t *_ptr;
	uint32_t _offset, _len;
	ReadMemoryFile(const uint8_t *ptr, uint32_t len)
		: _ptr(ptr), _offset(0), _len(len) {
	}
	uint32_t size() {
		return _len;
	}
	uint32_t tell() {
		return _offset;
	}
	void seek(int offs, int origin) {
		assert(origin == SEEK_SET);
		_offset = offs;
	}
	uint32_t read(void *ptr, uint32_t len) {
		int count = len;
		if (_offset + count > _len) {
			count = _len - _offset;
			_ioErr = true;
		}
		if (count != 0) {
			memcpy(ptr, _ptr + _offset, count);
			_offset += count;
		}
		return count;
	}
        uint32_t write(void *ptr, uint32_t len) {
		assert(0);
		return 0;
	}
};

struct WriteMemoryFile: File_impl {
	uint8_t *_ptr;
	uint32_t _offset, _len;
	WriteMemoryFile(uint8_t *ptr, uint32_t len)
		: _ptr(ptr), _offset(0), _len(len) {
	}
	uint32_t size() {
		return _len;
	}
	uint32_t tell() {
		return _offset;
	}
	void seek(int offs, int origin) {
		assert(origin == SEEK_SET);
		_offset = offs;
	}
	uint32_t read(void *ptr, uint32_t len) {
		assert(0);
		return 0;
	}
	uint32_t write(void *ptr, uint32_t len) {
		int count = len;
		if (_offset + count > _len) {
			count = _len - _offset;
			_ioErr = true;
		}
		if (count != 0) {
			memcpy(_ptr + _offset, ptr, count);
			_offset += count;
		}
		return count;
	}
};

File::File()
	: _path(0) {
	_impl = new StdioFile;
}

File::File(uint32_t offset, uint32_t size)
	: _path(0) {
	_impl = new StdioFile(offset, size);
}

File::File(const uint8_t *ptr, uint32_t len)
	: _path(0) {
	_impl = new ReadMemoryFile(ptr, len);
}

File::File(uint8_t *ptr, uint32_t len)
	: _path(0) {
	_impl = new WriteMemoryFile(ptr, len);
}

File::~File() {
	free(_path);
	_impl->close();
	delete _impl;
}

bool File::open(const char *path, const char *mode) {
	_impl->close();
	free(_path);
	_path = strdup(path);
	return _impl->open(path, mode);
}

void File::close() {
	_impl->close();
}

bool File::ioErr() const {
	return _impl->_ioErr;
}

uint32_t File::size() {
	return _impl->size();
}

uint32_t File::tell() {
	return _impl->tell();
}

void File::seek(int offs, int origin) {
	_impl->seek(offs, origin);
}

uint32_t File::read(void *ptr, uint32_t len) {
	return _impl->read(ptr, len);
}

uint8_t File::readByte() {
	uint8_t b;
	read(&b, 1);
	return b;
}

uint16_t File::readUint16LE() {
	uint8_t b[2];
	read(b, sizeof(b));
	return READ_LE_UINT16(b);
}

uint32_t File::readUint32LE() {
	uint8_t b[4];
	read(b, sizeof(b));
	return READ_LE_UINT32(b);
}

uint32_t File::readUint32BE() {
	uint8_t b[4];
	read(b, sizeof(b));
	return READ_BE_UINT32(b);
}

void File::write(void *ptr, uint32_t len) {
	_impl->write(ptr, len);
}

void File::writeByte(uint8_t b) {
	write(&b, 1);
}

void File::writeUint16LE(uint16_t n) {
	writeByte(n & 0xFF);
	writeByte(n >> 8);
}

void File::writeUint32LE(uint32_t n) {
	writeUint16LE(n & 0xFFFF);
	writeUint16LE(n >> 16);
}
