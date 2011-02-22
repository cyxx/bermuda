/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifdef BERMUDA_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef BERMUDA_POSIX
#include <dirent.h>
#include <sys/stat.h>
#endif
#include "file.h"
#include "fs.h"

struct FileSystem_impl {
	FileSystem_impl() :
		_fileList(0), _fileCount(0), _filePathSkipLen(0) {
	}

	virtual ~FileSystem_impl() {
		for (int i = 0; i < _fileCount; ++i) {
			free(_fileList[i]);
		}
		free(_fileList);
	}

	void setDataDirectory(const char *dir) {
		_filePathSkipLen = strlen(dir) + 1;
		buildFileListFromDirectory(dir);
	}

	const char *findFilePath(const char *file) {
		for (int i = 0; i < _fileCount; ++i) {
			if (strcasecmp(_fileList[i] + _filePathSkipLen, file) == 0) {
				return _fileList[i];
			}
		}
		return 0;
	}

	void addFileToList(const char *filePath) {
		_fileList = (char **)realloc(_fileList, (_fileCount + 1) * sizeof(char *));
		if (_fileList) {
			_fileList[_fileCount] = strdup(filePath);
			++_fileCount;
		}
	}

	virtual void buildFileListFromDirectory(const char *dir) = 0;

	static FileSystem_impl *create();

	char **_fileList;
	int _fileCount;
	int _filePathSkipLen;

};

#ifdef BERMUDA_WIN32
struct FileSystem_Win32 : FileSystem_impl {
	void buildFileListFromDirectory(const char *dir) {
		WIN32_FIND_DATA findData;
		char searchPath[MAX_PATH];
		sprintf(searchPath, "%s/*", dir);
		HANDLE h = FindFirstFile(searchPath, &findData);
		if (h) {
			do {
				char filePath[MAX_PATH];
				sprintf(filePath, "%s/%s", dir, findData.cFileName);
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (strcmp(findData.cFileName, "..") == 0 || strcmp(findData.cFileName, ".") == 0) {
						continue;
					}
					buildFileListFromDirectory(filePath);
				} else {
					addFileToList(filePath);
				}
			} while (FindNextFile(h, &findData));
			FindClose(h);
		}
	}
};
FileSystem_impl *FileSystem_impl::create() { return new FileSystem_Win32; }
#endif

#ifdef BERMUDA_POSIX
struct FileSystem_POSIX : FileSystem_impl {
	void buildFileListFromDirectory(const char *dir) {
		DIR *d = opendir(dir);
		if (d) {
			dirent *de;
			while ((de = readdir(d)) != NULL) {
				if (de->d_name[0] == '.') {
					continue;
				}
				char filePath[512];
				sprintf(filePath, "%s/%s", dir, de->d_name);
				struct stat st;
				if (stat(filePath, &st) == 0) {
					if (S_ISDIR(st.st_mode)) {
						buildFileListFromDirectory(filePath);
					} else {
						addFileToList(filePath);
					}
				}
			}
			closedir(d);
		}
	}
};
FileSystem_impl *FileSystem_impl::create() { return new FileSystem_POSIX; }
#endif

FileSystem::FileSystem(const char *rootDir) {
	_impl = FileSystem_impl::create();
	_impl->setDataDirectory(rootDir);
}

FileSystem::~FileSystem() {
	delete _impl;
}

static char *fixPath(const char *src) {
	char *path = (char *)malloc(strlen(src) + 4 + 1);
	if (path) {
		char *dst = path;
		if (strncmp(src, "..", 2) == 0) {
			src += 3;
		} else {
			strcpy(dst, "SCN/");
			dst += 4;
		}
		do {
			if (*src == '\\') {
				*dst = '/';
			} else {
				*dst = *src;
			}
			++dst;
		} while (*src++);
	}
	return path;
}

File *FileSystem::openFile(const char *path, bool errorIfNotFound) {
	File *f = 0;
	char *filePath = fixPath(path);
	if (filePath) {
		const char *fileSystemPath = _impl->findFilePath(filePath);
		if (fileSystemPath) {
			f = new File;
			if (!f->open(fileSystemPath, "rb")) {
				delete f;
				f = 0;
			}
		}
		free(filePath);
	}
	if (errorIfNotFound && !f) {
		error("Unable to open '%s'", path);
	}
	return f;
}

void FileSystem::closeFile(File *f) {
	if (f) {
		f->close();
		delete f;
	}
}

bool FileSystem::existFile(const char *path) {
	bool exists = false;
	char *filePath = fixPath(path);
	if (filePath) {
		exists = _impl->findFilePath(filePath) != 0;
		free(filePath);
	}
	return exists;
}
