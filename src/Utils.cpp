//
// Created by lixiaoqing on 2021/11/18.
//

#include <sys/stat.h>
#include <unistd.h>
#include "Utils.h"
#include "Log.h"
#include "encode.h"
#include "Define.h"

extern "C" {
#include "jpeglib.h"
#include "turbojpeg.h"
}

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

void JpegUtil::yuv2Jpeg(unsigned char *yuv_buffer, int yuv_size, int width, int height, int padding, int quality,
                        unsigned char **jpg_buffer, int &jpg_size) {
    TJSAMP subsample = TJSAMP_420;
    int need_size = tjBufSizeYUV2(width, padding, height, subsample);
    if (need_size != yuv_size) {
        Log::error(TAG,
                   "数据长度错误，根据宽高和padding计算的yuv数据长度=%d，给出的yuv数据长度=%d", need_size,
                   yuv_size);
        return;
    }

    int flags = 0;
    unsigned long retSize = 0;

    tjhandle handle = tjInitCompress();
    int ret = tjCompressFromYUV(handle, yuv_buffer, width, padding, height, subsample, jpg_buffer, &retSize, quality, flags);
    tjDestroy(handle);
    if (ret != 0) {
        Log::error("yu12jpeg", "压缩jpeg失败，错误信息:%s", tjGetErrorStr());
        return;
    }

    jpg_size = static_cast<int>(retSize);
}

void JpegUtil::jpeg2Rgb(unsigned char *jpegBuf, int jpegSize, int &jpegWidth, int &jpegHeight, unsigned char **rgbBuf, int &rgbSize) {

    // 创建一个 turbojpeg 句柄
    tjhandle handle = tjInitDecompress();
    if (nullptr == handle) {
        Log::error(TAG, "tjInitDecompress() failed.");
        return;
    }

    int jpeg_width = 0;
    int jpeg_height = 0;
    int jpegSubsamp = 0;
    int jpegColorspace = 0;

    // 解析 jpeg 图片的宽高
    int ret = tjDecompressHeader3(handle, jpegBuf, jpegSize, &jpeg_width, &jpeg_height, &jpegSubsamp, &jpegColorspace);
    if (ret != 0) {
        tjDestroy(handle);
        Log::error(TAG, "tjDecompressHeader3() failed.");
        return;
    }

    // 将 jpeg 解码为 rgb
    int rgb_size = jpeg_width * jpeg_height * 3;
    Byte *rgb_buffer = (Byte *) malloc(sizeof(Byte) * rgb_size);
    ret = tjDecompress2(handle, jpegBuf, jpegSize, rgb_buffer, jpeg_width, 0, jpeg_height, TJPF_RGB, 0);
    if (ret != 0) {
        free(rgb_buffer);
        tjDestroy(handle);
        return ;
    }
    tjDestroy(handle);

    jpegWidth = jpeg_width;
    jpegHeight = jpeg_height;
    *rgbBuf = rgb_buffer;
    rgbSize = rgb_size;
}

void WebpUtil::rgb2Webp(const unsigned char *rgb, int width, int height, int stride, float quality_factor, int lossless,
                        unsigned char **webpBuf, int &webpSize) {
    WebPConfig config;
    int ret = WebPConfigInitInternal(&config, WEBP_PRESET_ICON, quality_factor, WEBP_ENCODER_ABI_VERSION);
    if (ret != 0) {
        Log::error(TAG, "WebPConfigInitInternal failed.");
        return;
    }

    WebPPicture pic;
    ret = WebPPictureInit(&pic);
    if (ret != 0) {
        Log::error(TAG, "WebPPictureInit failed.");
        return;
    }

    WebPMemoryWriter wrt;
    config.lossless = !!lossless;
    pic.use_argb = !!lossless;
    pic.width = width;
    pic.height = height;
    pic.writer = WebPMemoryWrite;
    pic.custom_ptr = &wrt;
    WebPMemoryWriterInit(&wrt);

    ret = WebPPictureImportRGB(&pic, rgb, stride) && WebPEncode(&config, &pic);
    WebPPictureFree(&pic);
    if (!ret) {
        WebPMemoryWriterClear(&wrt);
        *webpBuf = nullptr;
        Log::error(TAG, "WebPPictureImportRGB failed.");
        return;
    }

    *webpBuf = wrt.mem;
    webpSize = static_cast<int>(wrt.size);
}