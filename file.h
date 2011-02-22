/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef FILE_H__
#define FILE_H__

#include "intern.h"

struct File_impl;

struct File {
	File();
	~File();

	bool open(const char *path, const char *mode = "rb");
	void close();
	bool ioErr() const;
	uint32 size();
	uint32 tell();
	void seek(int offs, int origin = SEEK_SET);
	uint32 read(void *ptr, uint32 len);
	uint8 readByte();
	uint16 readUint16LE();
	uint32 readUint32LE();
	void write(void *ptr, uint32 size);
	void writeByte(uint8 b);
	void writeUint16LE(uint16 n);
	void writeUint32LE(uint32 n);

	File_impl *_impl;
};

#endif // FILE_H__
