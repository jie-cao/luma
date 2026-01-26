// File Watcher - Cross-platform file change detection
// Used for shader hot-reload
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace luma {

// Callback when file changes: (path) -> void
using FileChangeCallback = std::function<void(const std::string& path)>;

class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();
    
    // Watch a file for changes
    void watchFile(const std::string& path, FileChangeCallback callback);
    
    // Watch multiple files
    void watchFiles(const std::vector<std::string>& paths, FileChangeCallback callback);
    
    // Stop watching a file
    void unwatchFile(const std::string& path);
    
    // Stop watching all files
    void unwatchAll();
    
    // Check for changes and invoke callbacks (call once per frame)
    // Returns true if any file changed
    bool checkChanges();
    
    // Get list of watched files
    std::vector<std::string> getWatchedFiles() const;
    
private:
    struct WatchedFile {
        std::string path;
        FileChangeCallback callback;
        uint64_t lastModTime = 0;
    };
    
    std::unordered_map<std::string, WatchedFile> watchedFiles_;
    
    // Get file modification time (returns 0 if file doesn't exist)
    static uint64_t getFileModTime(const std::string& path);
};

}  // namespace luma
