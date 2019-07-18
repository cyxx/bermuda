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
	File(uint32_t offset, uint32_t size);
	File(const uint8_t *ptr, uint32_t len);
	File(uint8_t *ptr, uint32_t len);
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
	uint32_t readUint32BE();
	void write(void *ptr, uint32_t size);
	void writeByte(uint8_t b);
	void writeUint16LE(uint16_t n);
	void writeUint32LE(uint32_t n);

	char *_path;
	File_impl *_impl;
};

#endif // FILE_H__
