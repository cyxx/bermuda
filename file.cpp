/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "file.h"

struct File_impl {
	bool _ioErr;
	File_impl() : _ioErr(false) {}
	virtual ~File_impl() {}
	virtual bool open(const char *path, const char *mode) = 0;
	virtual void close() = 0;
	virtual uint32 size() = 0;
	virtual uint32 tell() = 0;
	virtual void seek(int offs, int origin) = 0;
	virtual uint32 read(void *ptr, uint32 len) = 0;
	virtual void write(void *ptr, uint32 len) = 0;
};

struct File_stdio : File_impl {
	FILE *_fp;
	File_stdio() : _fp(0) {}
	bool open(const char *path, const char *mode) {
		_ioErr = false;
		_fp = fopen(path, mode);
		return (_fp != 0);
	}
	void close() {
		if (_fp) {
			fclose(_fp);
			_fp = 0;
		}
	}
	uint32 size() {
		uint32 sz = 0;
		if (_fp) {
			int pos = ftell(_fp);
			fseek(_fp, 0, SEEK_END);
			sz = ftell(_fp);
			fseek(_fp, pos, SEEK_SET);
		}
		return sz;
	}
	uint32 tell() {
		uint32 pos = 0;
		if (_fp) {
			pos = ftell(_fp);
		}
		return pos;
	}
	void seek(int offs, int origin) {
		if (_fp) {
			fseek(_fp, offs, origin);
		}
	}
	uint32 read(void *ptr, uint32 len) {
		uint32 r = 0;
		if (_fp) {
			r = fread(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
		return r;
	}
	void write(void *ptr, uint32 len) {
		if (_fp) {
			uint32 r = fwrite(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
	}
};

File::File() {
	_impl = new File_stdio;
}

File::~File() {
	_impl->close();
	delete _impl;
}

bool File::open(const char *path, const char *mode) {
	_impl->close();
	return _impl->open(path, mode);
}

void File::close() {
	_impl->close();
}

bool File::ioErr() const {
	return _impl->_ioErr;
}

uint32 File::size() {
	return _impl->size();
}

uint32 File::tell() {
	return _impl->tell();
}

void File::seek(int offs, int origin) {
	_impl->seek(offs, origin);
}

uint32 File::read(void *ptr, uint32 len) {
	return _impl->read(ptr, len);
}

uint8 File::readByte() {
	uint8 b;
	read(&b, 1);
	return b;
}

uint16 File::readUint16LE() {
	uint8 b[2];
	read(b, sizeof(b));
	return READ_LE_UINT16(b);
}

uint32 File::readUint32LE() {
	uint8 b[4];
	read(b, sizeof(b));
	return READ_LE_UINT32(b);
}

void File::write(void *ptr, uint32 len) {
	_impl->write(ptr, len);
}

void File::writeByte(uint8 b) {
	write(&b, 1);
}

void File::writeUint16LE(uint16 n) {
	writeByte(n & 0xFF);
	writeByte(n >> 8);
}

void File::writeUint32LE(uint32 n) {
	writeUint16LE(n & 0xFFFF);
	writeUint16LE(n >> 16);
}
