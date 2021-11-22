//
// Created by lixiaoqing on 2021/11/18.
//

#ifndef VIDEOINTERCEPT_UTILS_H
#define VIDEOINTERCEPT_UTILS_H

/**
 * 文件工具
 */
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

/**
 * Jpeg 工具
 */
namespace JpegUtil {
    /**
     * YUV 编码为 JPEG
     * @param yuv_buffer
     * @param yuv_size
     * @param width
     * @param height
     * @param padding
     * @param quality
     * @param jpg_buffer
     * @param jpg_size
     */
    void yuv2Jpeg(unsigned char *yuv_buffer, int yuv_size, int width, int height, int padding, int quality,
                  unsigned char **jpg_buffer, int &jpg_size);

    /**
     * JPEG 解码为 RGB
     * @param jpegBuf
     * @param jpegSize
     * @param jpegWidth
     * @param jpegHeight
     * @param rgbBuf
     * @param rgbSize
     */
    void jpeg2Rgb(unsigned char *jpegBuf, int jpegSize, int &jpegWidth, int &jpegHeight, unsigned char **rgbBuf,
                  int &rgbSize);
}

/**
 * Webp 工具
 */
namespace WebpUtil {
    /**
     * 将 RGB 编码为为 WEBP
     * @param rgb
     * @param width
     * @param height
     * @param stride
     * @param quality_factor
     * @param lossless
     * @param webpBuf
     * @param webpSize
     */
    void rgb2Webp(const unsigned char *rgb, int width, int height, int stride, float quality_factor, int lossless,
                  unsigned char **webpBuf, int &webpSize);
}

#endif //VIDEOINTERCEPT_UTILS_H
