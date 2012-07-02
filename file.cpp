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
	uint32 _offset, _size;
	File_stdio() : _fp(0), _offset(0), _size(0) {}
	File_stdio(uint32 offset, uint32 size) : _fp(0), _offset(offset), _size(size) {}
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
	uint32 size() {
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
	uint32 tell() {
		uint32 pos = 0;
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

File_impl *FileImpl_create() { return new File_stdio; }
File_impl *FileImpl_create(uint32 offset, uint32 size) { return new File_stdio(offset, size); }

File::File() {
	_impl = FileImpl_create();
}

File::File(File_impl *impl)
	: _impl(impl) {
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
