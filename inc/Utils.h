//
// Created by lixiaoqing on 2021/11/18.
//

#ifndef VIDEOINTERCEPT_UTILS_H
#define VIDEOINTERCEPT_UTILS_H

namespace FileUtil {
    /**
     * 打印文件信息，包括：
     *   1. 最后修改时间
     *   2. 大小
     * @param filePath
     */
    void printFileInfo(const char *const filePath);

    /**
     * 获取文件最后修改时间。单位：秒
     * @param filePath
     */
    long long int getFileLastModifyTime(const char *const filePath);

    /**
     * 获取文件大小
     * @param filePath
     */
    int getFileSize(const char *const filePath);

    /**
     * 判断文件是否存在
     * @param filePath
     * @return
     */
    bool isFileExist(const char *const filePath);
}

#endif //VIDEOINTERCEPT_UTILS_H
