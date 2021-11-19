//
// Created by lixiaoqing on 2021/11/18.
//

#include <sys/stat.h>
#include <unistd.h>
#include "Utils.h"
#include "Log.h"

#define TAG "Utils"

void FileUtil::printFileInfo(const char *const filePath) {
    struct stat fileInfo{};
    if (stat(filePath, &fileInfo) != 0) {
        return;
    }

    char time[64] = {};

    // time of last access
    auto atime = fileInfo.st_atime;
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", std::localtime(&atime));
    Log::info(TAG, "atime: %s", time);

    // time of last data modification
    auto mtime = fileInfo.st_mtime;
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", std::localtime(&mtime));
    Log::info(TAG, "mtime: %s", time);

    // time of last status change
    auto ctime = fileInfo.st_ctime;
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", std::localtime(&ctime));
    Log::info(TAG, "ctime: %s", time);

    // time of file creation(birth)
    auto birthtime = fileInfo.st_birthtimespec.tv_sec;
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", std::localtime(&birthtime));
    Log::info(TAG, "birthtime: %s", time);

    // file size, in bytes
    auto size = fileInfo.st_size;
    Log::info(TAG, "size: %lld", size);
}

long long int FileUtil::getFileLastModifyTime(const char *const filePath) {
    struct stat fileInfo{};
    if (stat(filePath, &fileInfo) != 0) {
        return 0;
    }
    return fileInfo.st_mtime;
}

int FileUtil::getFileSize(const char *const filePath) {
    struct stat fileInfo{};
    if (stat(filePath, &fileInfo) != 0) {
        return 0;
    }
    return static_cast<int>(fileInfo.st_size);
}

bool FileUtil::isFileExist(const char *const filePath) {
    return 0 == access(filePath, F_OK);
}