// File Watcher Implementation
// Cross-platform file modification time detection

#include "file_watcher.h"
#include <iostream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace luma {

FileWatcher::FileWatcher() = default;
FileWatcher::~FileWatcher() = default;

uint64_t FileWatcher::getFileModTime(const std::string& path) {
#if defined(_WIN32)
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0;
    }
    ULARGE_INTEGER uli;
    uli.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    uli.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    return uli.QuadPart;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0;
    }
    // Use modification time in nanoseconds if available
#if defined(__APPLE__)
    return (uint64_t)st.st_mtimespec.tv_sec * 1000000000ULL + st.st_mtimespec.tv_nsec;
#else
    return (uint64_t)st.st_mtim.tv_sec * 1000000000ULL + st.st_mtim.tv_nsec;
#endif
#endif
}

void FileWatcher::watchFile(const std::string& path, FileChangeCallback callback) {
    WatchedFile wf;
    wf.path = path;
    wf.callback = callback;
    wf.lastModTime = getFileModTime(path);
    
    if (wf.lastModTime == 0) {
        std::cerr << "[filewatcher] Warning: file not found: " << path << std::endl;
    }
    
    watchedFiles_[path] = std::move(wf);
    std::cout << "[filewatcher] Watching: " << path << std::endl;
}

void FileWatcher::watchFiles(const std::vector<std::string>& paths, FileChangeCallback callback) {
    for (const auto& path : paths) {
        watchFile(path, callback);
    }
}

void FileWatcher::unwatchFile(const std::string& path) {
    watchedFiles_.erase(path);
}

void FileWatcher::unwatchAll() {
    watchedFiles_.clear();
}

bool FileWatcher::checkChanges() {
    bool anyChanged = false;
    
    for (auto& [path, wf] : watchedFiles_) {
        uint64_t currentModTime = getFileModTime(path);
        
        if (currentModTime != 0 && currentModTime != wf.lastModTime) {
            std::cout << "[filewatcher] File changed: " << path << std::endl;
            wf.lastModTime = currentModTime;
            anyChanged = true;
            
            if (wf.callback) {
                wf.callback(path);
            }
        }
    }
    
    return anyChanged;
}

std::vector<std::string> FileWatcher::getWatchedFiles() const {
    std::vector<std::string> result;
    result.reserve(watchedFiles_.size());
    for (const auto& [path, wf] : watchedFiles_) {
        result.push_back(path);
    }
    return result;
}

}  // namespace luma
