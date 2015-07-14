/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifdef BERMUDA_POSIX
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include "file.h"

struct File_impl {
	bool _ioErr;
	File_impl() : _ioErr(false) {}
	virtual ~File_impl() {}
	virtual bool open(const char *path, const char *mode) = 0;
	virtual void close() = 0;
	virtual uint32_t size() = 0;
	virtual uint32_t tell() = 0;
	virtual void seek(int offs, int origin) = 0;
	virtual uint32_t read(void *ptr, uint32_t len) = 0;
	virtual void write(void *ptr, uint32_t len) = 0;
};

struct File_stdio : File_impl {
	FILE *_fp;
	uint32_t _offset, _size;
	File_stdio() : _fp(0), _offset(0), _size(0) {}
	File_stdio(uint32_t offset, uint32_t size) : _fp(0), _offset(offset), _size(size) {}
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
	void write(void *ptr, uint32_t len) {
		if (_fp) {
			uint32_t r = fwrite(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
	}
};

File_impl *FileImpl_create() { return new File_stdio; }
File_impl *FileImpl_create(uint32_t offset, uint32_t size) { return new File_stdio(offset, size); }

File::File()
	: _path(0) {
	_impl = FileImpl_create();
}

File::File(File_impl *impl)
	: _path(0), _impl(impl) {
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

struct MemoryMappedFile_impl {
	MemoryMappedFile_impl(const char *path)
		: _ptr(0), _size(0) {
	}
	virtual ~MemoryMappedFile_impl() {
	}

	static MemoryMappedFile_impl *create(const char *path);

	void *_ptr;
	size_t _size;
};

#ifdef BERMUDA_POSIX
struct MemoryMappedFile_POSIX : MemoryMappedFile_impl {
	MemoryMappedFile_POSIX(const char *path)
		: MemoryMappedFile_impl(path), _fd(-1) {
		_fd = open(path, O_RDONLY);
		if (_fd != -1) {
			struct stat st;
			if (stat(path, &st) == 0) {
				void *addr = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, _fd, 0);
				if (addr != MAP_FAILED) {
					_ptr = addr;
					_size = st.st_size;
				}
			}
		}
	}
	virtual ~MemoryMappedFile_POSIX() {
		if (_ptr != 0) {
			munmap(_ptr, _size);
			_ptr = 0;
			_size = 0;
		}
		if (_fd != -1) {
			close(_fd);
			_fd = -1;
		}
	}

	int _fd;
};
MemoryMappedFile_impl *MemoryMappedFile_impl::create(const char *path) {
	return new MemoryMappedFile_POSIX(path);
}
#endif

#ifdef BERMUDA_WIN32
MemoryMappedFile_impl *MemoryMappedFile_impl::create(const char *path) {
	return 0;
}
#endif

MemoryMappedFile::MemoryMappedFile()
	: _impl(0) {
}

MemoryMappedFile::~MemoryMappedFile() {
	close();
}

bool MemoryMappedFile::open(const char *path, const char *mode) {
	close();
	_impl = MemoryMappedFile_impl::create(path);
	return _impl != 0;
}

void MemoryMappedFile::close() {
	if (_impl) {
		delete _impl;
		_impl = 0;
	}
}

void *MemoryMappedFile::getPtr() {
	return _impl ? _impl->_ptr : 0;
}

