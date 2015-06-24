/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef FILE_H__
#define FILE_H__

#include "intern.h"

struct File_impl;

File_impl *FileImpl_create();
File_impl *FileImpl_create(uint32_t offset, uint32_t size);

struct File {
	File();
	File(File_impl *impl);
	~File();

	bool open(const char *path, const char *mode = "rb");
	void close();
	bool ioErr() const;
	uint32_t size();
	uint32_t tell();
	void seek(int offs, int origin = SEEK_SET);
	uint32_t read(void *ptr, uint32_t len);
	uint8_t readByte();
	uint16_t readUint16LE();
	uint32_t readUint32LE();
	void write(void *ptr, uint32_t size);
	void writeByte(uint8_t b);
	void writeUint16LE(uint16_t n);
	void writeUint32LE(uint32_t n);

	char *_path;
	File_impl *_impl;
};

struct MemoryMappedFile_impl;

struct MemoryMappedFile {
	MemoryMappedFile();
	~MemoryMappedFile();

	bool open(const char *path, const char *mode = "rb");
	void close();
	void *getPtr();

	MemoryMappedFile_impl *_impl;
};

#endif // FILE_H__
