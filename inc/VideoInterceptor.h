//
// Created by lixiaoqing on 2021/4/29.
//

#ifndef VIDEOINTERCEPT_VIDEOINTERCEPTOR_H
#define VIDEOINTERCEPT_VIDEOINTERCEPTOR_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "VideoModel.h"


class VideoInterceptor final {

public:

    VideoInterceptor();

    ~VideoInterceptor();

    VideoInterceptor(const VideoInterceptor &obj) = delete;

    VideoInterceptor &operator=(const VideoInterceptor &obj) = delete;

public:

    /**
     * 解析视频信息
     * @param videoPath
     * @return
     */
    static std::shared_ptr<VideoModel> analyseVideoInfo(const char *const videoPath);

    static long long int getImageCount();

    /**
     * 从视频文件中截图
     * @param videoModel
     * @return
     */
    bool intercept(const std::shared_ptr<VideoModel> &videoModel);

    void setImageDir(const std::string &imageDir);

private:

    /**
     * 释放资源
     */
    void release();

    /**
     * 打开视频文件，进行一些初始化操作
     * @param videoFilePath
     * @return
     */
    bool openVideoFile(const std::string &videoFilePath);

    void getKeyPacketTimeInterval(bool &isValid, int64_t &avInterval);

    /**
     * 将 YUV 帧数据保存为 JPEG
     * @param frame
     * @param width
     * @param height
     * @param timestamp
     * @return
     */
    bool saveFrameJpeg(AVFrame *frame, int width, int height, Timestamp timestamp);

    /**
     * 将 RGB 帧数据保存为 WEBP
     * @param pFrame
     * @param width
     * @param height
     * @param photoFilePath
     * @return
     */
    static bool saveFrameWebp(AVFrame *pFrame, int width, int height, const char *photoFilePath);

    /**
     * JPEG 转 WEBP 并保存
     * @param jpegBuf
     * @param jpegSize
     * @param webpFilePath
     * @return
     */
    static bool jpeg2Webp(unsigned char *jpegBuf, int jpegSize, const char *const webpFilePath);


private:

//    uint8_t *bufferRgb;
    unsigned char *bufferYuv;

    AVFormatContext *formatCtx;
    AVFrame *frameOrigin;
    AVFrame *frameYuv;
//    AVFrame *frameRgb;

    AVCodec *codec;
    AVCodecContext *codecCtx;

//    struct SwsContext *swsCtxRgb;
    struct SwsContext *swsCtxYuv;

    int videoIndex;
    int videoRealWidth;
    int videoRealHeight;
    int videoDestWidth;
    int videoDestHeight;

    /**
     * 已经截取的图片数量
     */
    static long long int imageCount;

    /**
     * 图片保存目录
     */
    std::string imageDir;
};


#endif //VIDEOINTERCEPT_VIDEOINTERCEPTOR_H
