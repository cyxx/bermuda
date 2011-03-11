#!/usr/bin/env python

VERSION = '0.1.4'

SDL_TARBALL = [ 'bs-%s-sdl-win32.zip',
	(
		'../README',
		'../README-SDL',
		'../libogg-0.dll',
		'../libvorbis-0.dll',
		'../libvorbisfile-3.dll',
		'../SDL.dll',
		'../bs.exe'
	)
]

SRC_TARBALL = [ 'bs-%s.tar.bz2',
	(
		'../Makefile',
		'../README',
		'../bag.cpp',
		'../avi_player.cpp',
		'../avi_player.h',
		'../decoder.cpp',
		'../decoder.h',
		'../dialogue.cpp',
		'../file.cpp',
		'../file.h',
		'../fs.cpp',
		'../fs.h',
		'../game.cpp',
		'../game.h',
		'../intern.h',
		'../main.cpp',
		'../mixer.cpp',
		'../mixer.h',
		'../opcodes.cpp',
		'../parser_dlg.cpp',
		'../parser_scn.cpp',
		'../random.cpp',
		'../random.h',
		'../resource.cpp',
		'../saveload.cpp',
		'../staticres.cpp',
		'../str.cpp',
		'../str.h',
		'../systemstub.h',
		'../systemstub_sdl.cpp',
		'../util.cpp',
		'../util.h',
		'../win16.cpp'
	)
]

DST_DIR = '.'

import os, tarfile, zipfile, md5

def print_md5(md5_file, filename):
	m = md5.new()
	fp = file(filename, 'rb')
	m.update(fp.read())
	line = m.hexdigest() + ' ' + os.path.split(filename)[1] + '\n'
	md5_file.write(line)

def build_zip_tarball(file_name, file_list):
	file_path = os.path.join(DST_DIR, file_name)
	zf = zipfile.ZipFile(file_path, 'w', zipfile.ZIP_DEFLATED)
	for entry_path in file_list:
		entry_name = os.path.split(entry_path)[1]
		zf.write(entry_path, entry_name)
	zf.close()

def build_bz2_tarball(file_name, file_list):
	file_path = os.path.join(DST_DIR, file_name)
	tf = tarfile.open(file_path, 'w:bz2')
	for entry_path in file_list:
		entry_name = file_name[0:-8] + '/' + os.path.split(entry_path)[1]
		tf.add(entry_path, entry_name)
	tf.close()

md5_file = file('CHECKSUMS-%s.MD5' % VERSION, 'w')
for tarball in (SDL_TARBALL, SRC_TARBALL):
	file_name = tarball[0] % VERSION
	file_list = tarball[1]
	print "Generating '" + file_name + "'...",
	if file_name.endswith('.zip'):
		build_zip_tarball(file_name, file_list)
	elif file_name.endswith('.tar.bz2'):
		build_bz2_tarball(file_name, file_list)
	else:
		raise "Unhandled extension for tarball"
	print "Ok"
	print_md5(md5_file, file_name)

