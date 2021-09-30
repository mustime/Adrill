/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include <dirent.h>
#include <unistd.h>

#include "macros.h"
#include "file_utils.h"

FileSearcher::FileSearcher(const std::string& rootPath) {
    this->_defaultRootPath = rootPath;
    if (!rootPath.empty()) {
        this->_searchPaths.push_back(rootPath);
    }
}

const std::vector<std::string>& FileSearcher::searchPaths() const {
    return this->_searchPaths;
}

void FileSearcher::addSearchPath(const std::string& searchPath, bool addToFront) {
    std::string prefix;
    if (!this->isAbsolutePath(searchPath)) {
        prefix = this->_defaultRootPath;
    }
    std::string path = prefix + searchPath;
    if (path.length() > 0 && path[path.length() - 1] != '/') {
        path += "/";
    }
    if (addToFront) {
        this->_searchPaths.insert(this->_searchPaths.begin(), path);
    } else {
        this->_searchPaths.push_back(path);
    }
}

void FileSearcher::setSearchPaths(const std::vector<std::string>& searchPaths) {
    this->_searchPaths.clear();

    bool hasDefaultRootPath = false;
    for (const std::string& searchPath : searchPaths) {
        std::string prefix;
        if (!this->isAbsolutePath(searchPath)) {
            prefix = this->_defaultRootPath;
        }
        std::string path = prefix + searchPath;
        if (path.length() > 0 && path[path.length() - 1] != '/') {
            path += "/";
        }
        if (!hasDefaultRootPath && path == this->_defaultRootPath) {
            hasDefaultRootPath = true;
        }
        this->_searchPaths.push_back(path);
    }
    if (!hasDefaultRootPath && !this->_defaultRootPath.empty()) {
        this->_searchPaths.push_back(this->_defaultRootPath);
    }
}

std::string FileSearcher::resolveFullPath(const std::string& path) const {
    // empty or already absolution path ?
    if (path.empty()) return "";
    if (this->isAbsolutePath(path)) return path;

    // do search
    for (const std::string& searchPath : this->_searchPaths) {
        std::string fullPath = this->_searchFromPath(path, searchPath);
        if (fullPath.length() > 0) {
            return fullPath;
        }
    }
    
    LOGGER_LOGE("no file found for '%s'\n", path.c_str());
    return path;
}

std::string FileSearcher::resolveCanonicalPath(const std::string& path) const {
    std::string resolved;
    char* ret = ::realpath(path.c_str(), nullptr);
    if (ret) {
        resolved = ret;
        ::free(ret);
    }
    return resolved;
}

bool FileSearcher::isAbsolutePath(const std::string& path) const {
    return (path[0] == '/');
}

std::string FileSearcher::toAbsolutePath(const std::string& path) const {
    std::string abs_path = path;
    if (!this->isAbsolutePath(abs_path)) {
        char tmp[PATH_MAX] = { 0 };
        if (!::getcwd(tmp, PATH_MAX)) {
            LOGGER_LOGE("unable to get cwd: %s\n", ::strerror(errno));
        }
        size_t tmplen = ::strlen(tmp);
        if (tmp[tmplen - 1] != '/') tmp[tmplen] = '/';
        abs_path.assign(tmp);
        abs_path += path;
    }
    return abs_path;
}

bool FileSearcher::isFileExist(const std::string& filename) const {
    bool existed = false;
    if (!filename.empty()) {
        if (!this->isAbsolutePath(filename)) {
            // do search
            for (const std::string& searchPath : this->_searchPaths) {
                std::string fullPath = this->_searchFromPath(filename, searchPath);
                if (!fullPath.empty()) {
                    existed = true;
                    break;
                }
            }
        } else {
            existed = (::access(filename.c_str(), F_OK) == 0);
        }
    }
    return existed;
}

// TODO: considering search path
bool FileSearcher::isDirExist(const std::string& dirpath) const {
    bool found = false;
    DIR* dp = ::opendir(dirpath.c_str());
    if (dp) {
        found = true;
        ::closedir(dp);
    }
    return found;
}

std::string FileSearcher::_searchFromPath(const std::string& filename, const std::string& searchPath) const {
    std::string prefix_path;
    std::string postfix_name = filename;
    size_t pos = filename.find_last_of('/');
    if (pos != std::string::npos) {
        prefix_path = filename.substr(0, pos + 1);
        postfix_name = filename.substr(pos + 1);
    }

    std::string path = searchPath + prefix_path;
    if (!path.empty() && path[path.size() - 1] != '/') {
        path += '/';
    }
    path += postfix_name;
    return this->isFileExist(path) ? path : "";
}