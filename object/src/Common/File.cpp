
/////////////////////////////////////////////////////////////
/// Copyright(c) 2010 rivonet, All rights reserved.
/// @file:		FileUt.cpp
/// @brief:		base file utility
///
/// @author:	liudong.8284@gmail.com
/// @date:		2010-09-01
/////////////////////////////////////////////////////////////

#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>

#include "Common/Util.hpp"
#include "Common/Define.hpp"
#include "Common/File.hpp"

namespace common {

const int c_default_permit 	= (S_IRWXU | S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP);

std::string
module_path()
{
	char data[4096];
	int count = readlink("/proc/self/exe", data, 4096);
	if (count < 0 || count >= 4096) {
		return "";
	}
	data[count] = 0;
	return data;
}

std::string
curr_path()
{
	char data[4096];
	return getcwd(data, 4096);
}

bool
is_absolute_path(const std::string& path)
{
	return path.find_first_of("/\\") == 0;
}

    std::string
smart_path(const std::string& path)
{
	if (is_absolute_path(path)) {
		return path;
	}
	std::string curr = cut_end_slash(curr_path());
	return curr + (path.length() == 0 ?
		"" : "/") + path;
}

bool
is_path(const std::string& path)
{
	return path.find_first_of("/\\") != std::string::npos;
}

std::string
split_path(const std::string& path, bool slash)
{
	std::string dir;
	split_path_file(path, &dir, NULL, slash);
	return dir;
}

std::string
split_file(const std::string& path)
{
	std::string file;
	split_path_file(path, NULL, &file);
	return file;
}

void
split_path_file(const std::string& path, std::string* dir, std::string* file, bool slash)
{
	std::string::size_type pos = path.find_last_of("/\\");

	///< we hope that there should not have any dup '/' or '\'
	///< like /file
	if (pos == 0) {
		if (dir) {
			*dir = path.at(0);
		}
		if (file) {
			*file = path.substr(1);
		}
		return;
	}

	///< when path not including any '/' or '\', then
	///<	if you set slash true, dir is empty and path is also file
	///<	if you set slash false,dir and file both be path
	if (dir) {
		if (pos == std::string::npos) {
			*dir = !slash ? path : "";
		} else {
			*dir = path.substr(0, pos);
		}
	}

	if (file) {
		if (pos == std::string::npos) {
			*file = path;
		} else {
			*file = path.substr(pos + 1);
		}
	}
	return;
}

const char*
last_part(const char* name)
{
    const char* last = strrchr(name, '/');
    return last ? last + 1 : name;
}

std::string
split_app(const std::string& file)
{
	std::string::size_type pos = file.find_last_of(".");
	if (pos == std::string::npos) {
		return file;
	}
	return file.substr(0, pos);
}

std::string
ins_sub_dir(const std::string& path, const std::string& dir)
{
	std::string spath = split_path(path, true);
	if (spath.length() != 0 ||
		path.find_first_of("/\\") == 0)
	{
		spath += "/";
	}
	return spath + dir + "/" + split_file(path);
}

int
make_path(const std::string& path, int mode)
{
	if (file_exist(path)) {
		return 0;
	}

	for (std::string::size_type i = 0; i < path.size();) {
		std::string::size_type a = path.find_first_of("/\\", i);
		if (a == std::string::npos)	{
			a = path.size();
		
        } else if (a == 0) {
            if (path[0] == '/') {
                a += 1;
            }
        }
		if (mkdir(path.substr(0, a).c_str(), mode) != 0) {
			if (errno != EEXIST) {
				return errno;
			}
		}
		i = a + 1;
	}
	return 0;
}

int
make_file_path(const std::string& path, int mode)
{
	return make_path(split_path(path, true), mode);
}

std::string
cut_end_slash(const std::string& path)
{
	std::string::size_type pos = path.find_last_not_of("/\\");
	if (pos != path.length() - 1) {
		return path.substr(0, pos + 1);
	}
	return path;
}

std::string
prune_dup_slash(const std::string& path)
{
	///< no need prune
	if (path.find("//") == std::string::npos) {
		return path;
	}

	std::string s;
	for (std::string::size_type i = 0; i < path.size();) {
		std::string::size_type a = path.find_first_of("/", i);
		if (a == std::string::npos) {
			a = path.size() - 1;
		}
		s += path.substr(i, a - i + 1);
		std::string::size_type b = path.find_first_not_of("/", a + 1);
		if (b == std::string::npos) {
			break;
		}
		i = b;
	}
	return s;
}

std::string
fix_path_unix(const std::string& path)
{
	std::string s = path;
	for (std::string::size_type i = 0; i < s.size(); i++) {
		if (s[ i ] == '\\') {
			s[ i ] = '/';
		}
	}
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

file_handle_t
file_open(const std::string& path, int flag, int mode)
{
	if (flag == 0) {
		flag = O_CREAT | O_RDWR;
	}
	return ::open(path.c_str(), flag, mode);
}

int
file_close(file_handle_t fd)
{
	if (fd == c_invalid_handle) {
		return 0;
	}
	return close(fd);
}

uint64_t
file_len(file_handle_t fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) {
		return -1;
	}
	return st.st_size;
}

uint64_t
file_pos(file_handle_t fd)
{
	return lseek64(fd, 0, SEEK_CUR);
}

int64_t
file_seek(file_handle_t fd, uint64_t off)
{
	if (off == c_invalid_offset) {
		return lseek64(fd, 0, SEEK_END);
	} else {
		return lseek64(fd, off, SEEK_SET);
	}
}

int
file_trunc(file_handle_t fd, uint64_t len)
{
	return ftruncate(fd, len);
}

int
file_write(file_handle_t fd, const char* data, uint32_t len, uint64_t off)
{
	if (off != c_invalid_offset && !file_seek(fd, off)) {
		return -1;
	}
	return write(fd, data, len);
}

int
file_read(file_handle_t fd, char* data, uint32_t len, uint64_t off)
{
	if (off != c_invalid_offset && !file_seek(fd, off)) {
		return -1;
	}
	return read(fd, data, len);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t
file_load(const std::string& path, char* data, uint32_t len, uint64_t off)
{
	file_handle_t fd = file_open(path);
	if (fd == c_invalid_handle) {
		return -1;
	}
	len = file_read(fd, (char*)data, len, off);
	file_close(fd);
	return len;
}

int
file_rush(const std::string& path, const char* data, uint32_t len, int mode)
{
	file_handle_t fd = file_open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, mode);
	if (fd == c_invalid_handle) {
		return -1;
	}
	uint32_t wlen = file_write(fd, (char*)data, len);
	file_close(fd);
	return len == wlen ? 0 : -1;
}

uint64_t
file_len(const std::string& path)
{
	file_handle_t hd = file_open(path);
	if (hd == c_invalid_handle) {
		return -1;
	}
	uint64_t len = lseek64(hd, 0, SEEK_END);
	file_close(hd);
	return len;
}

bool
file_exist(const std::string& path)
{
	return access(path.c_str(), F_OK) == 0;
}

bool
file_dir(const std::string& path)
{
	struct stat filestat;
	return !(lstat(path.c_str(), &filestat) < 0 || S_ISDIR(filestat.st_mode) == 0);
}

bool
file_reg(const std::string& path)
{
	struct stat filestat;
	return !(lstat(path.c_str(), &filestat) < 0 || S_ISREG(filestat.st_mode) == 0);
}


int
file_rm(const std::string& path, bool dir)
{
	return dir ? rmdir(path.c_str()) : unlink(path.c_str());
}

int
traverse_rmdir(const std::string& path)
{
	return file_traverse(path, file_rm) > 0 ? 0 : -1;
}

uint64_t
file_trunc(const std::string& path, uint64_t newlen, bool create)
{
	file_handle_t hd = file_open(path, create);
	if (hd == c_invalid_handle) {
		return -1;
	}

	uint64_t len = lseek64(hd, 0, SEEK_END);
	if (len != newlen) {
		if (ftruncate(hd, newlen) != 0) {
			file_close(hd);
			return -1;
		}
	}
	file_close(hd);
	return len;
}

int
file_move(const std::string& pathold, const std::string& pathnew)
{
	file_rm(pathnew, false);
	file_rm(pathnew, true);
	return rename(pathold.c_str(), pathnew.c_str());
}

int
file_traverse(const std::string& path, handle_entry_t handle)
{
	int ret = 0;
	struct dirent storge, *entry;
	DIR *dir = opendir(path.c_str());
	if (dir == NULL) {
		return -1;
	}

	while (readdir_r(dir, &storge, &entry) == 0 && entry) {
		std::string subfile = path + "/" + entry->d_name;
		if (file_dir(subfile)) {
			if (strcmp(".", entry->d_name) == 0 ||
				strcmp("..", entry->d_name) == 0)
			{
				continue;
			}

			ret = file_traverse(subfile, handle);
			if (ret != 0) {
				break;
			}

		} else {
			ret = handle(subfile, false);
			if (ret != 0) {
				break;
			}
		}
	}
	if (ret == 0) {
		handle(path, true);
	}

	closedir(dir);
	return 0;
}

FileBase::FileBase()
{
	clear();
}

FileBase::~FileBase()
{
	close();
}

void
FileBase::clear()
{
	m_fd = c_invalid_handle;
}

int
FileBase::open_flag(int type)
{
	int flag = (get_bit(type, op_creat) ? O_CREAT : 0) |
		(get_bit(type, op_trunc) ? O_TRUNC : 0) |
		(get_bit(type, op_write) ? O_RDWR : O_RDONLY);

	flag |= (get_bit(type, op_direct) ? O_DIRECT : 0) |
			(get_bit(type, op_sync) ? O_SYNC : 0);
	return flag;
}

int
FileBase::open(const std::string& file_path, int flag, int mode)
{
	if (flag == 0) {
		flag = make_bit(op_creat, op_write);
	}
	/** if already opend, close first */
	if (is_open()) {
		close();
	}

	m_path = file_path;
	m_name = split_file(m_path);

	/** when create, try to make path */
	if (get_bit(flag, op_creat)) {
		if (make_file_path(path()) != 0) {
			return -1;
		}
	}

	m_fd = file_open(path(), open_flag(flag), mode);

	return is_open() ? 0 : -1;
}

int
FileBase::close()
{
	if (!is_open()) {
		return 0;
	}

	int ret = 0;
	if (m_fd != c_invalid_handle) {
		ret = file_close(m_fd);
		m_fd = c_invalid_handle;
	}
	return ret;
}

uint64_t
FileBase::file_len()
{
	if (!is_open()) {
		return -1;
	}
	return common::file_len(m_fd);
}

int
FileBase::pwrite(const void* buf, uint32_t len, uint64_t off)
{
	if (!is_open()) {
		return -1;
	} else {
		return ::pwrite(m_fd, buf, len, off);
	}
}

int
FileBase::pread(void* buf, uint32_t len, uint64_t off)
{
	if (!is_open()) {
		return -1;
	} else {
		len = ::pread(m_fd, buf, len, off);
		return len;
	}
}

int
FileBase::trunc(uint64_t len)
{
	if (!is_open()) {
		return -1;
	}
	return file_trunc(m_fd, len);
}

int
FileBase::flush()
{
	if (!is_open()) {
		return -1;
	}
	return fsync(m_fd);		 //fdatasync
}
}

