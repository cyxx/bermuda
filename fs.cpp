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
#include <sys/param.h>
#include "file.h"
#include "fs.h"
#include "str.h"

struct FileSystem_impl {
	FileSystem_impl() :
		_rootDir(0), _fileList(0), _fileCount(0), _filePathSkipLen(0) {
	}

	virtual ~FileSystem_impl() {
		free(_rootDir);
		for (int i = 0; i < _fileCount; ++i) {
			free(_fileList[i]);
		}
		free(_fileList);
	}

	void setDataDirectory(const char *dir) {
		_rootDir = strdup(dir);
		_filePathSkipLen = strlen(dir) + 1;
		buildFileListFromDirectory(dir);
	}

	const char *findFilePath(const char *file) {
		if (_fileCount == 0) {
			return file;
		}
		for (int i = 0; i < _fileCount; ++i) {
			if (strcasecmp(_fileList[i], file) == 0) {
				return _fileList[i];
			}
		}
		return 0;
	}

	void addFileToList(const char *filePath) {
		_fileList = (char **)realloc(_fileList, (_fileCount + 1) * sizeof(char *));
		if (_fileList) {
			_fileList[_fileCount] = strdup(filePath + _filePathSkipLen);
			++_fileCount;
		}
	}

	virtual void buildFileListFromDirectory(const char *dir) = 0;

	static FileSystem_impl *create();

	char *_rootDir;
	char **_fileList;
	int _fileCount;
	int _filePathSkipLen;

};

#ifdef BERMUDA_WIN32
struct FileSystem_Win32 : FileSystem_impl {
	void buildFileListFromDirectory(const char *dir) {
		WIN32_FIND_DATA findData;
		char searchPath[MAX_PATH];
		snprintf(searchPath, sizeof(searchPath), "%s/*", dir);
		HANDLE h = FindFirstFile(searchPath, &findData);
		if (h) {
			do {
				char filePath[MAX_PATH];
				snprintf(filePath, sizeof(filePath), "%s/%s", dir, findData.cFileName);
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
				char filePath[MAXPATHLEN];
				snprintf(filePath, sizeof(filePath), "%s/%s", dir, de->d_name);
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

struct FileSystem_romfs {
	uint32_t _startOffset;
	const uint8_t *_ptr;
	uint32_t _pos;
	MemoryMappedFile _f;

	FileSystem_romfs()
		: _startOffset(0), _ptr(0), _pos(0) {
	}

	void open(const char *filePath) {
		if (_f.open(filePath)) {
			_ptr = (const uint8_t *)_f.getPtr();
			if (_ptr && memcmp(_ptr, "-rom1fs-", 8) == 0) {
				_pos = 16;
				readString(0);
				_startOffset = _pos;
			}
		}
	}

	bool isOpen() const {
		return _startOffset != 0;
	}

	uint32_t readLong() {
		uint32_t l = 0;
		if (_ptr) {
			l = READ_BE_UINT32(_ptr + _pos);
			_pos += 4; 
		}
		return l;
	}

	void readString(char *s) {
		if (_ptr) {
			const char *src = (const char *)_ptr + _pos;
			if (s) {
				strcpy(s, src);
			}
			const int len = (strlen(src) + 15) & ~15;
			_pos += len;
		}
	}

	File *openFile(const char *path, int level = 0) {
		if (level == 0) {
			_pos = _startOffset;
		}
		const char *sep = strchr(path, '/');
		do {
			const uint32_t nextOffset = readLong();
			const uint32_t specInfo = readLong();
			const uint32_t dataSize = readLong();
			_pos += 4;
			char name[32];
			readString(name);
			switch (nextOffset & 7) {
			case 1:
				if (sep && strncasecmp(name, path, sep - path) == 0) {
					_pos = specInfo;
					return openFile(sep + 1, level + 1);
				}
				break;
			case 2:
				if (strcasecmp(name, path) == 0) {
					File_impl *fi = FileImpl_create(_pos, dataSize);
					return new File(fi);
				}
				break;
			}
			_pos = nextOffset & ~15;
		} while (_pos != 0);
		return 0;
	}
};

static const char *kBsDat = "bs.dat";

static FileSystem_romfs g_romfs;

FileSystem::FileSystem(const char *rootDir)
	: _impl(0) {
	g_romfs.open(kBsDat);
	if (g_romfs.isOpen()) {
		return;
	}
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
#ifdef __EMSCRIPTEN__
		stringToUpperCase(path);
#endif
	}
	return path;
}

File *FileSystem::openFile(const char *path, bool errorIfNotFound) {
	File *f = 0;
	char *fixedPath = fixPath(path);
	if (fixedPath) {
		if (g_romfs.isOpen()) {
			f = g_romfs.openFile(fixedPath);
			if (!f->open(kBsDat, "rb")) {
				delete f;
				f = 0;
			}
		} else {
			const char *filePath = _impl->findFilePath(fixedPath);
			if (filePath) {
				f = new File;
				char fileSystemPath[MAXPATHLEN];
				snprintf(fileSystemPath, sizeof(fileSystemPath), "%s/%s", _impl->_rootDir, filePath);
				if (!f->open(fileSystemPath, "rb")) {
					delete f;
					f = 0;
				}
			}
		}
		free(fixedPath);
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
	char *fixedPath = fixPath(path);
	if (fixedPath) {
		if (g_romfs.isOpen()) {
			File *f = g_romfs.openFile(fixedPath);
			exists = f != 0;
			delete f;
		} else {
			exists = _impl->findFilePath(fixedPath) != 0;
		}
		free(fixedPath);
	}
	return exists;
}
