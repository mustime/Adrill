/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_UTILS_FILE_UTILS_H__
#define __ADRILL_UTILS_FILE_UTILS_H__

#include <string>
#include <vector>

class FileSearcher {
public:
    FileSearcher(const std::string& root_path = "");

    /** search path allows auto searching full path from relative paths */
    const std::vector<std::string>& searchPaths() const;
    void addSearchPath(const std::string& path, bool addToFront = false);
    void setSearchPaths(const std::vector<std::string>& searchPaths);

    /** path resolving */
    bool isAbsolutePath(const std::string& path) const;
    std::string toAbsolutePath(const std::string& path) const;
    std::string resolveFullPath(const std::string& filename) const;
    std::string resolveCanonicalPath(const std::string& path) const;

    /** accessible */
    bool isFileExist(const std::string& path) const;
    bool isDirExist(const std::string& dirpath) const;

protected:
    std::string _searchFromPath(const std::string& filename, const std::string& searchPath) const;

    std::string _defaultRootPath;
    std::vector<std::string> _searchPaths;

};

#endif // __ADRILL_UTILS_FILE_UTILS_H__
