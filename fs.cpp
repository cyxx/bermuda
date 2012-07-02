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
		sprintf(searchPath, "%s/*", dir);
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
				char filePath[1024];
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

struct FileSystem_RomFS {

	struct Node {
		char name[16];
		uint32 offset;
		Node *child, *next;
	};
	uint32_t _startOffset;

	FileSystem_RomFS(const char *filePath) {
		File f;
		if (f.open(filePath)) {
			char buf[8];
			if (f.read(buf, 8) == 8 && memcmp(buf, "-rom1fs-", 8) == 0) {
				f.seek(8, SEEK_CUR);
				readString(f, 0);
				_startOffset = f.tell();
				_firstNode = readNode(f, 0);
			}
		}
	}
	virtual ~FileSystem_RomFS() {
		freeNode(_firstNode);
	}

	static uint32_t readLong(File &f) {
		uint8_t buf[4];
		if (f.read(buf, 4) == 4) {
			return READ_BE_UINT32(buf);
		}
		return 0;
	}

	static const char *readString(File &f, char *s) {
		static const int kPadSize = 16;
		uint8_t buf[kPadSize];
		int len = 0;
		while (f.read(buf, sizeof(buf)) == sizeof(buf)) {
			if (s) {
				memcpy(s + len, buf, sizeof(buf));
				len += sizeof(buf);
			}
			if (memchr(buf, 0, sizeof(buf))) {
				break;
			}
		}
		return s;
	}

	Node *readNode(File &f, uint32_t offset) {
		f.seek(_startOffset + offset);
while (1) {
		uint32 nextOffset = readLong(f);
		uint32 specData = readLong(f);
		f.seek(8, SEEK_CUR);
		char name[16];
		readString(f, name);
printf("'%s' type %d offset %x %x\n", name, nextOffset & 7, nextOffset & ~7, specData );
		if (nextOffset == 0) {
			return 0;
		}
		switch (nextOffset & 3) {
		case 1: // directory
			printf("dir '%s'\n", name);
			if (name[0] == '.') {
				continue;
			}
// TODO: specData as offset
//			readNode(f, specData);
			printf("enddir\n");
			break;
		case 2: // regular file
			printf("file '%s'\n", name);
			break;
		}
		f.seek((nextOffset & ~15));
}
return 0;
	}

	void freeNode(Node *node) {
		for (Node *n = node; n; n = n->next) {
			if (n->child) {
				freeNode(n->child);
			}
		}
		free(node);
	}

	Node *findNode(Node *node, const char *path) {
		const char *sep = strchr(path, '/');
		if (sep) {
			const int len = sep - path;
			char name[8];
			assert(len < int(sizeof(name) - 1));
			strncpy(name, path, len);
			name[len] = 0;
			for (Node *n = node; n; n = n->next) {
				if (strcasecmp(n->name, name) == 0) {
					return n->child ? findNode(n->child, sep + 1) : 0;
				}
			}
		} else {
			for (Node *n = node; n; n = n->next) {
				if (strcasecmp(n->name, path) == 0) {
					return n;
				}
			}
		}
		return 0;
	}

	Node *_firstNode;
};

FileSystem::FileSystem(const char *rootDir) {
	FileSystem_RomFS fs("bs.dat");
exit(1);
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
	char *fixedPath = fixPath(path);
	if (fixedPath) {
		const char *filePath = _impl->findFilePath(fixedPath);
		if (filePath) {
			f = new File;
			char fileSystemPath[1024];
			snprintf(fileSystemPath, sizeof(fileSystemPath), "%s/%s", _impl->_rootDir, filePath);
			if (!f->open(fileSystemPath, "rb")) {
				delete f;
				f = 0;
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
		exists = _impl->findFilePath(fixedPath) != 0;
		free(fixedPath);
	}
	return exists;
}
