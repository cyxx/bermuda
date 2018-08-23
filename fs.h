/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef FS_H__
#define FS_H__

#include "intern.h"

struct File;
struct FileSystem_impl;

struct FileSystem {
	FileSystem(const char *dataPath);
	~FileSystem();

	File *openFile(const char *path, bool errorIfNotFound = true);
	void closeFile(File *f);

	bool existFile(const char *path);

	FileSystem_impl *_impl;
	bool _romfs;
};

struct FileHolder {
	FileHolder(FileSystem &fs, const char *path, bool errorIfNotFound = true)
		: _fs(fs) {
		_fp = _fs.openFile(path, errorIfNotFound);
	}

	~FileHolder() {
		_fs.closeFile(_fp);
		_fp = 0;
	}

	File *operator->() {
		return _fp;
	}

	FileSystem &_fs;
	File *_fp;
};

#endif // FS_H__
