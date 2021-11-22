//
// Created by lixiaoqing on 2021/4/29.
//

#include <sstream>
#include <sys/stat.h>
#include "Log.h"
#include "VideoInterceptor.h"
#include "Utils.h"

#define STACK_SIZE (1024)

// 最大帧率限制 一般为15-30帧/秒
#define Frame_Rate_Max_Limit (60)
#define Frame_Rate_Min_Limit (8)

// 截图位置最大偏差秒数  4秒
#define Intercept_Max_Deviation_Second (4)

// jpeg 图片压缩质量
#define Jpeg_Compression_Quality (100)

// webp 图片压缩质量
#define Webp_Compression_Quality (100)

// 缩放之后的图片宽高
#define Dest_Width  (1280)
#define Dest_Height (720)

#define TAG "VideoInterceptor"


long long int VideoInterceptor::imageCount = 0;

VideoInterceptor::VideoInterceptor() {
    formatCtx = nullptr;
    bufferYuv = nullptr;
    frameOrigin = nullptr;
    frameYuv = nullptr;
    codec = nullptr;
    codecCtx = nullptr;
    swsCtxYuv = nullptr;
//    bufferRgb = nullptr;
//    frameRgb = nullptr;
//    swsCtxRgb = nullptr;

    videoIndex = -1;
    videoRealWidth = 0;
    videoRealHeight = 0;
    videoDestWidth = 0;
    videoDestHeight = 0;
    imageDir = "";
}

VideoInterceptor::~VideoInterceptor() {
    release();
}

void VideoInterceptor::release() {
    Log::info(TAG, "release()");

    if (formatCtx) {
        //avformat_free_context(m_pFormatCtx);
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }

    if (frameOrigin) {
        av_free(frameOrigin);
        frameOrigin = nullptr;
    }

    if (frameYuv) {
        av_free(frameYuv);
        frameYuv = nullptr;
    }

    if (bufferYuv) {
        av_free(bufferYuv);
        bufferYuv = nullptr;
    }

    if (codecCtx) {
        avcodec_close(codecCtx);
//        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }

    if (swsCtxYuv) {
        sws_freeContext(swsCtxYuv);
        swsCtxYuv = nullptr;
    }

//    if (frameRgb) {
//        av_free(frameRgb);
//        frameRgb = nullptr;
//    }
//
//    if (bufferRgb) {
//        av_free(bufferRgb);
//        bufferRgb = nullptr;
//    }
//
//    if (swsCtxRgb) {
//        sws_freeContext(swsCtxRgb);
//        swsCtxRgb = nullptr;
//    }

    codec = nullptr;
    videoIndex = -1;
    videoRealWidth = 0;
    videoRealHeight = 0;
    videoDestWidth = 0;
    videoDestHeight = 0;
    imageDir = "";
}

std::shared_ptr<VideoModel> VideoInterceptor::analyseVideoInfo(const char *const videoPath) {

    if (videoPath == nullptr || !FileUtil::isFileExist(videoPath)) {
        Log::error(TAG, "videoPath == nullptr || video file not exist.");
        return nullptr;
    }

    // 获取文件最后修改时间，即结束时间。单位：秒
    Timestamp lastModifyTime = FileUtil::getFileLastModifyTime(videoPath);
    if (lastModifyTime == 0) {
        Log::error(TAG, "获取最后修改时间出错！");
        return nullptr;
    }

    // 转换成毫秒
    lastModifyTime *= 1000;

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (nullptr == pFormatCtx) {
        Log::error(TAG, "avformat_alloc_context failed, video=%s", videoPath);
        return nullptr;
    }

    char errorBuf[STACK_SIZE];
    int ret = avformat_open_input(&pFormatCtx, videoPath, nullptr, nullptr);
    if (ret < 0) {
        av_strerror(ret, errorBuf, STACK_SIZE);
        Log::error(TAG, "avformat_open_input() failed, code: %d, error: %s, video: %s", ret, errorBuf, videoPath);
        avformat_close_input(&pFormatCtx);
        return nullptr;
    }

    int videoIdx = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIdx = i;
            break;
        }
    }
    if (videoIdx < 0) {
        Log::error(TAG, "ffmpeg find video codec failed.");
        avformat_close_input(&pFormatCtx);
        return nullptr;
    }

    AVCodecContext *avCodecCtx = pFormatCtx->streams[videoIdx]->codec;
    if (avCodecCtx == nullptr) {
        avformat_close_input(&pFormatCtx);
        return nullptr;
    }

    // 视频宽高
    int width = avCodecCtx->width;
    int height = avCodecCtx->height;

    // 视频时长，单位：毫秒
    int duration = static_cast<int>(pFormatCtx->duration / 1000);

    // 如果时长或者宽高获取不到，则尝试需要打开视频流，补充视频信息
    if (width <= 0 || height <= 0 || width > 10000 || height > 10000 || duration <= 0 || duration >= 1 * 3600 * 1000) {
        ret = avformat_find_stream_info(pFormatCtx, nullptr);
        if (ret < 0) {
            av_strerror(ret, errorBuf, STACK_SIZE);
            Log::error(TAG, "avformat_find_stream_info failed, code=%d, error=%s, video=%s", ret, errorBuf, videoPath);
            avformat_close_input(&pFormatCtx);
            return nullptr;
        }

        if (avCodecCtx == nullptr) {
            avformat_close_input(&pFormatCtx);
            return nullptr;
        }

        width = avCodecCtx->width;
        height = avCodecCtx->height;
        duration = static_cast<int>(pFormatCtx->duration / 1000);
    }

    if (pFormatCtx) {
        avformat_close_input(&pFormatCtx);
    }

    // 视频开始时间。单位：毫秒
    auto createTime = lastModifyTime - duration;

    // 视频信息
    auto videoModel = std::make_shared<VideoModel>();
    videoModel->videoPath = videoPath;
    videoModel->start = createTime;
    videoModel->end = lastModifyTime;
    return videoModel;
}

bool VideoInterceptor::intercept(const std::shared_ptr<VideoModel> &videoModel) {
    if (videoModel->points.empty()) {
        return false;
    }

    // 打开视频文件，进行初始化
    bool ret = openVideoFile(videoModel->videoPath);
    if (!ret) {
        Log::error(TAG, "openVideoFile() failed.");
        return false;
    }

    Log::info(TAG, "正在从视频文件中截图, 视频:%s, 截图点个数:%d", videoModel->videoPath.c_str(), videoModel->points.size());

    /* Tips: 在下面的代码中，时间变量的前缀如果是av表示按标准时基为单位(微秒)，如果是stream表示按视频流的时基为单位 ------ */

    // 视频流的时基
    const AVRational steamTimeBase = formatCtx->streams[videoIndex]->time_base;

    // 获取视频流的帧率
    AVRational videoFrameRate = formatCtx->streams[videoIndex]->r_frame_rate;
    double videoFrameRateDouble = av_q2d(videoFrameRate);
    AVRational avg_frame_rate = formatCtx->streams[videoIndex]->avg_frame_rate;

    // 两帧之间的时间间隔的一半，单位微秒
    int avHalfTimeIntervalBetween2Frame = ((int) (AV_TIME_BASE / videoFrameRateDouble)) >> 1;
    int64_t streamHalfTimeIntervalBetween2Frame = av_rescale_q(avHalfTimeIntervalBetween2Frame, AV_TIME_BASE_Q,
                                                               steamTimeBase);
//    Log::error(LOGGER_WARNING, TAG, "视频帧率 = %f, 帧间距的一半 = %lld", videoFrameRateDouble, streamHalfTimeIntervalBetween2Frame);

    // 获取两个关键帧之间的平均间隔（参考值）
    bool isGopSizeValid = false;
    int64_t avKeyPacketTimeInterval = -1;
    getKeyPacketTimeInterval(isGopSizeValid, avKeyPacketTimeInterval);

    // 视频解码过滤器清空缓存
    avcodec_flush_buffers(formatCtx->streams[videoIndex]->codec);

    // 循环Seek，读取Packet，然后保存Frame
    unsigned long frameIndex = 0;
    int64_t lastPointKeyPacketIndex = 0;

    // 开始截图
    for (frameIndex = 0; frameIndex < videoModel->points.size(); frameIndex++) {
        // 截图点
        auto timestamp = videoModel->points.at(frameIndex);
        Log::info(TAG, ">>> timestamp=%llu", timestamp);

        // 时间戳合法性校验
        if (timestamp < videoModel->start || timestamp > videoModel->end) {
            Log::warn(TAG, "当前时间戳不在视频范围内！timestamp: %llu, video: %s", timestamp, videoModel->videoPath.c_str());
            continue;
        }

//        u8 startINTERCEPT = CommonUtils::GetSystemTime_mills();

        // 截图点的相对时间，以内部时基为时基，单位：微秒
        int64_t avExpectFrameTime = static_cast<int64_t>(timestamp - videoModel->start) * 1000;

        // 截图点的相对时间，以流的时基为时基
        int64_t streamExpectFrameTime = av_rescale_q(avExpectFrameTime, AV_TIME_BASE_Q, steamTimeBase);

        // 判断是否需要Seek
        bool isNeedSeek = false;
        if (frameIndex == 0) {
            isNeedSeek = true;
        } else if (!isGopSizeValid) {
            isNeedSeek = false;
        } else {
            int64_t currentPointKeyPacketIndex = avExpectFrameTime / avKeyPacketTimeInterval;
            isNeedSeek = (currentPointKeyPacketIndex > lastPointKeyPacketIndex);
            lastPointKeyPacketIndex = currentPointKeyPacketIndex;
        }

        // 保存FFmpeg相关方法的执行结果
        int resultCode = -1;
        // FFmpeg相关方法执行错误时的错误信息
        char errorBuffer[STACK_SIZE];
        // Seek到指定时间之前最近的一个关键帧
        bool soughtJustNow = false;

        if (isNeedSeek) {
            resultCode = av_seek_frame(formatCtx, videoIndex,
                                       streamExpectFrameTime + formatCtx->streams[videoIndex]->start_time,
                                       AVSEEK_FLAG_BACKWARD);
            // Seek 失败
            if (resultCode < 0) {
                av_strerror(resultCode, errorBuffer, STACK_SIZE);
                Log::error(TAG, "point = %lld, code = %d, error = %s", timestamp, resultCode, errorBuffer);
                return false;
            }
            soughtJustNow = true;
        }

        // 保存解码的数据包
        AVPacket packet;
        av_init_packet(&packet);

        // 标识是够截图成功
        bool interceptPointSuccess = false;

        // 从关键帧开始往后读取，直到找到一个距离截图点最近的一个帧，break
        while (true) {
            // 不用关心这个值, 并且不应该关心这个值。
            int frameFinished = 0;

            // 1. 读取Packet
            resultCode = av_read_frame(formatCtx, &packet);

            // 1.1 如果读取失败，continue;
            if (resultCode != 0) {
                av_strerror(resultCode, errorBuffer, STACK_SIZE);
                Log::error(TAG, "Read frame failed! code=%d, error=%s", resultCode, errorBuffer);
                av_free_packet(&packet);
                break;
            }

            // 1.2 如果读取的不是视频流的包
            if (packet.stream_index != videoIndex) {
                av_free_packet(&packet);
                continue;
            }

            // 检查是不是真的Seek到关键帧了
            if (soughtJustNow) {
                int isIFrame = (packet.flags & AV_PKT_FLAG_KEY);
                // 如果不是，Seek回去，以后也不再Seek了
                if (!isIFrame) {
                    // 往回Seek 3倍 gop大小(实际上1.5就可以，但保守起见多退一点)
                    int64_t streamKeyPacketTimeInterval = av_rescale_q(avKeyPacketTimeInterval, AV_TIME_BASE_Q,
                                                                       steamTimeBase);
                    int64_t seekTarget = packet.pts - streamKeyPacketTimeInterval * 3;
                    // 如果比流的起始时间还小，说明是视频刚开始，seek到视频流的起始位置即可
                    if (seekTarget < formatCtx->streams[videoIndex]->start_time) {
                        seekTarget = formatCtx->streams[videoIndex]->start_time;
                    }
                    resultCode = av_seek_frame(formatCtx, videoIndex, seekTarget, AVSEEK_FLAG_BACKWARD);

                    // Seek 失败
                    if (resultCode < 0) {
                        av_strerror(resultCode, errorBuffer, STACK_SIZE);
                        Log::error(TAG, "av_seek_frame failed,%d,%s", resultCode, errorBuffer);
                        av_free_packet(&packet);
                        return false;
                    }
                    isGopSizeValid = false;
                }
                soughtJustNow = false;
            }

            // 2. 解码
            resultCode = avcodec_decode_video2(codecCtx, frameOrigin, &frameFinished, &packet);

            // 2.1 解码失败
            if (resultCode < 0) {
                av_strerror(resultCode, errorBuffer, STACK_SIZE);
                Log::error(TAG, "decompress the packet failed! and the error code is %d. error:%s,time:%lld",
                           resultCode, errorBuffer, timestamp);
                av_free_packet(&packet);
                break;
            }
            // 2.2 没有解码到东西
            if (resultCode == 0) {
                Log::error(TAG, "No frame could be decompressed! resultCode = %d, frameFinish = %d", resultCode,
                           frameFinished);
                av_free_packet(&packet);
                break;
            }

            // 2.3 如果是黑屏
            if (frameFinished == 0) {
                av_free_packet(&packet);
                continue;
            }

            // 3. 判断是不是距离最近的包
            bool isNearestPacket = false;

            // 3.1 当前读取到的包的流时间
            int64_t streamCurrentPacketTime = frameOrigin->pts - formatCtx->streams[videoIndex]->start_time;
            if (streamCurrentPacketTime > 0) {
                // 3.2 当前包的流时间与期望的流时间之间的差
                int64_t streamDifference = streamExpectFrameTime - streamCurrentPacketTime;
                int64_t streamDifferenceAbs = abs(streamDifference);
                // 3.3 根据时间差判断是不是最近的
                isNearestPacket =
                        (streamDifferenceAbs <= streamHalfTimeIntervalBetween2Frame) || (streamDifference <= 0);
                // streamDifference  < 0 时 可能错误帧或者卡帧的情况下 会导致滞后过多（水印时间偏大） 应该按照截图失败处理
                // 暂定差值大于 2秒内的帧数量 * streamHalfTimeIntervalBetween2Frame * 2
                // 校验帧率合法性 帧率合法范围 8- 60帧/秒
                if (streamDifference < 0 && videoFrameRateDouble < Frame_Rate_Max_Limit &&
                    videoFrameRateDouble > Frame_Rate_Min_Limit) {
                    int64_t deviation = Intercept_Max_Deviation_Second * videoFrameRateDouble *
                                        streamHalfTimeIntervalBetween2Frame * 2;
                    if (streamDifferenceAbs > deviation) {
                        interceptPointSuccess = false;
                        break;
                    }
                }
            } else {
                // 3.2 如果pts=0，也就是第一帧，会是纯黑的。pts<0属于异常。
                isNearestPacket = false;
            }

            if (!isNearestPacket) {
                av_free_packet(&packet);
                continue;
            }

            Log::info(TAG, "point=%llu intercept succeed.", timestamp);
            interceptPointSuccess = true;
            break;
        }

        // 截图失败
        if (!interceptPointSuccess) {
            Log::error(TAG, "point=%lld intercept faild.", timestamp);
            av_free_packet(&packet);
            continue;
        }

//        // RGB 缩放
//        resultCode = sws_scale(swsCtxRgb, frameOrigin->data, frameOrigin->linesize, 0, codecCtx->height, frameRgb->data, frameRgb->linesize);
//        if (resultCode < 0) {
//            av_free_packet(&packet);
//            av_strerror(resultCode, errorBuffer, STACK_SIZE);
//            Log::error(LOGGER_WARNING, TAG, "sws_scale() failed, code=%d, error=%s",
//                       resultCode, errorBuffer);
//            continue;
//        }

        // YUV 缩放
        resultCode = sws_scale(swsCtxYuv, frameOrigin->data, frameOrigin->linesize, 0, codecCtx->height, frameYuv->data,
                               frameYuv->linesize);
        if (resultCode < 0) {
            av_free_packet(&packet);
            av_strerror(resultCode, errorBuffer, STACK_SIZE);
            Log::error(TAG, "sws_scale() failed, code=%d, error=%s", resultCode, errorBuffer);
            continue;
        }

//        u8 endINTERCEPT = CommonUtils::GetSystemTime_mills();
//        Log::error(LOGGER_INFO, TAG, "INTERCEPT 耗时= %llu 毫秒", endINTERCEPT - startINTERCEPT);

        // 将帧存为 jpeg
        bool saveFrameSuccess = saveFrameJpeg(frameYuv, videoDestWidth, videoDestHeight, timestamp);
        if (!saveFrameSuccess) {
            av_free_packet(&packet);
            continue;
        }

        av_free_packet(&packet);

        // 图片数+1
        VideoInterceptor::imageCount += 1;
    }

    // 释放资源
    release();
    return true;
}

bool VideoInterceptor::openVideoFile(const std::string &videoFilePath) {

    if (videoFilePath.empty()) {
        Log::error(TAG, "videoFilePath is empty.");
        return false;
    }

    // 初始化一个视频的Context
    formatCtx = avformat_alloc_context();
    if (nullptr == formatCtx) {
        Log::error(TAG, "avformat_alloc_context failed.");
        return false;
    }

    // 打开视频并读取视频的头信息
    char errorBuffer[STACK_SIZE] = {};
    int ret = avformat_open_input(&formatCtx, videoFilePath.c_str(), nullptr, nullptr);
    if (ret < 0) {  // 小于0是失败
        av_strerror(ret, errorBuffer, STACK_SIZE);
        Log::error(TAG, "avformat_open_input failed, code=%d, error=%s", ret, errorBuffer);
        release();
        return false;
    }

    // 获取这个视频包含的流信息
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret < 0) {
        av_strerror(ret, errorBuffer, STACK_SIZE);
        Log::error(TAG, "avformat_find_stream_info failed, code=%d, error=%s", ret, errorBuffer);
        release();
        return false;
    }

    // 找出文件中的视频流
    videoIndex = -1;  // 视频流的索引
    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        // 找出其中的视频流
        if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1) {
        Log::error(TAG, "unkown mp4 format.");
        release();
        return false;
    }

    // 找到视频流的解码Context、解码器
    codecCtx = formatCtx->streams[videoIndex]->codec;

    // 获取流对应的解码器参数
    //    AVCodecParameters * codecPars = formatCtx->streams[m_videoIndex]->codecpar;
    //    AVCodecID codecID = codecPars->codec_id;
//    AVCodecID codecID = codecCtx->codec_id;

    //        // 硬解码器
    //        if (codecID == AV_CODEC_ID_H264) {
    //            codec = avcodec_find_decoder_by_name("h264_mediacodec");
    //        } else if (codecID == AV_CODEC_ID_MPEG4) {
    //            codec = avcodec_find_decoder_by_name("mpeg4_mediacodec");
    //        }
    //        if (codec == nullptr) {
    //            LOG(TAG, "查找硬解码器失败，解码失败！");
    //            return false;
    //        }

    // 查找软解码器
    codec = avcodec_find_decoder(codecCtx->codec_id);

    //    codecCtx = avcodec_alloc_context3(codec);
    //
    //    // 替换解码器上下文参数。将视频流信息拷贝到 AVCodecContext 中
    //    avcodec_parameters_to_context(codecCtx, codecPars);

    // 打开解码器
    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        av_strerror(ret, errorBuffer, STACK_SIZE);
        Log::error(TAG, "avcodec_open2() failed, code=%d, error=%s", ret, errorBuffer);
        release();
        return false;
    }

    // 获取视频流的帧率
    AVRational videoFrameRate = formatCtx->streams[videoIndex]->r_frame_rate;
    double frameRate = av_q2d(videoFrameRate);

    // 码率
    int bitRate = codecCtx->bit_rate / 1000;

    // 视频实际的宽高
    videoRealWidth = codecCtx->width;
    videoRealHeight = codecCtx->height;

    // 缩放
    if (videoRealHeight > Dest_Height) {
        videoDestHeight = Dest_Height;
        videoDestWidth = videoRealWidth * videoDestHeight / videoRealHeight;
    } else {
        videoDestHeight = videoRealHeight;
        videoDestWidth = videoRealWidth;
    }

    Log::info(TAG, "图片实际宽高: %dx%d , 图片缩放宽高: %dx%d", videoRealWidth, videoRealHeight, videoDestWidth, videoDestHeight);

    // 创建空的帧
    frameOrigin = av_frame_alloc();
    if (frameOrigin == nullptr) {
        Log::error(TAG, "m_pFrame|m_pFrameRGB is null!");
        release();
        return false;
    }

    //    frameRgb = av_frame_alloc();
    //    if (frameRgb == nullptr) {
    //        Log::error(TAG, "m_pFrameRGB is null!");
    //        release();
    //        return false;
    //    }
    //
    //    //创建一个Buffer, 大小是图片目标尺寸两倍
    //    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, m_nDestWidth, m_nDestHeight);
    //    bufferRgb = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    //
    //    // 将空的帧填充到Buffer中？填空的干嘛？
    //    ret = avpicture_fill((AVPicture *) frameRgb, bufferRgb, AV_PIX_FMT_RGB24, m_nDestWidth,
    //                         m_nDestHeight);
    //    if (ret < 0) {
    //        av_strerror(ret, errorBuffer, STACK_SIZE);
    //        Log::error(TAG, "avpicture_fill failed, code=%d, error=%s", ret, errorBuffer);
    //        release();
    //        return false;
    //    }

    //    if (enableHardDecoding) {
    frameYuv = av_frame_alloc();
    if (frameYuv == nullptr) {
        Log::error(TAG, "frameYuv is null!");
        release();
        return false;
    }

    //创建一个Buffer, 大小是图片目标尺寸两倍
    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, videoDestWidth, videoDestHeight);
    bufferYuv = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    // 将空的帧填充到Buffer中？填空的干嘛？
    ret = avpicture_fill((AVPicture *) frameYuv, bufferYuv, AV_PIX_FMT_YUV420P, videoDestWidth,
                         videoDestHeight);
    if (ret < 0) {
        av_strerror(ret, errorBuffer, STACK_SIZE);
        Log::error(TAG, "avpicture_fill failed, code=%d, error=%s",
                   ret, errorBuffer);
        release();
        return false;
    }
    //    }

    // 获取Context前，判断pix_fmt是否是FFmpeg支持的
    if (sws_isSupportedInput(codecCtx->pix_fmt) == 0) {
        Log::error(TAG, "OpenVideoFile failed!!! %d is not supported as input pixel format", codecCtx->pix_fmt);
        return false;
    }

    //    // 获取缩放Context
    //    swsCtxRgb = sws_getContext(m_videoRealWidth, m_videoRealHeight, codecCtx->pix_fmt, m_nDestWidth, m_nDestHeight,
    //                               AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
    //    if (nullptr == swsCtxRgb) {
    //        Log::error(TAG, "sws_getContext return null!");
    //        release();
    //        return false;
    //    }

    swsCtxYuv = sws_getContext(videoRealWidth, videoRealHeight,
                               codecCtx->pix_fmt,
                               videoDestWidth, videoDestHeight, AV_PIX_FMT_YUV420P,
                               SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (nullptr == swsCtxYuv) {
        Log::error(TAG, "sws_getContext return null!");
        release();
        return false;
    }

    Log::info(TAG, ">>> 视频文件: %s, w=%d, h=%d", videoFilePath.c_str(), codecCtx->width, codecCtx->height);
    return true;
}

/**
 * 下面这一段是通过遍历获取真实的帧率和GOP间隔时间
 * 根据测试发现，gop_size会变化，帧率会变化，但是两个关键帧之间的时间间隔是保持大致稳定的
 * 如果存在两个关键帧之间的时间间隔相差很大的情况，那这种算法就可能会出错了。
 * @param[out] isValid 返回的时间间隔是否有效
 * @param[out] interval 时间间隔, 单位微秒
 */
void VideoInterceptor::getKeyPacketTimeInterval(bool &isValid, int64_t &avInterval) {

    AVPacket packet;
    av_init_packet(&packet);

    // 清空视频解码过滤器的缓存
    avcodec_flush_buffers(formatCtx->streams[videoIndex]->codec);

    // 第一个关键帧的pts
    int64_t firstIPacketPTS = -1;
    // 最后一个关键帧的pts
    int64_t lastIPacketPTS = -1;
    // 下一个要读取的帧的id
    int nextPacketIndex = 0;
    // 已经读取的I帧的数量
    int sumCount = 0;
    // 期望读取的I帧的数量，超过这个值不再读取
    const int I_Packet_Count = 50;
    // 期望读取的帧的数量，超过这个值不再读取
    const int Packet_Count = 3000;

    while (nextPacketIndex < Packet_Count && sumCount < I_Packet_Count) {
        int resultCode = av_read_frame(formatCtx, &packet);
        if (resultCode != 0) {
            break;
        }

        // 判断是否是视频帧
        if (packet.stream_index == videoIndex) {
            // 判断是否是是关键帧
            int isKeyPacket = (packet.flags & AV_PKT_FLAG_KEY);
            if (isKeyPacket) {
                // 第一个关键帧赋值
                if (firstIPacketPTS == -1) {
                    firstIPacketPTS = packet.pts;
                }
                // 最后一个关键帧赋值
                lastIPacketPTS = packet.pts;
                // 已读取的关键帧的数量
                sumCount++;
            }
        }
        nextPacketIndex++;
        av_free_packet(&packet);
    }

    // 计算出平均两个关键帧之间时间间隔
    bool isGopSizeValid = false;
    int64_t avKeyPacketTimeInterval = -1;
    if (sumCount > 1) {
        // 视频流的时基
        AVRational streamTimebase = formatCtx->streams[videoIndex]->time_base;
        int64_t streamAverageTimeInterval = (lastIPacketPTS - firstIPacketPTS) / (sumCount - 1);
        avKeyPacketTimeInterval = av_rescale_q(streamAverageTimeInterval, streamTimebase, AV_TIME_BASE_Q);
        isGopSizeValid = true;
    }

    isValid = isGopSizeValid;
    avInterval = avKeyPacketTimeInterval;
}

bool VideoInterceptor::saveFrameJpeg(AVFrame *frame, int width, int height, Timestamp timestamp) {

    /**
     * yuv 编码为 jpeg
     */

    int yuv_size = width * height * 3 / 2;
    Byte *yuv_buffer = (Byte *) malloc(sizeof(Byte) * yuv_size);
    if (yuv_buffer == nullptr) {
        Log::error(TAG, "Failed to malloc yuv buffer.");
        return false;
    }

    // 组合 YUV 到 yuv_buffer
    memcpy(yuv_buffer, frame->data[0], width * height);
    memcpy(yuv_buffer + width * height, frame->data[1], width * height / 4);
    memcpy(yuv_buffer + width * height + width * height / 4, frame->data[2], width * height / 4);

    int type = 0;    // YUV420P
    int padding = 1;
    int quality = Jpeg_Compression_Quality;   // 100表示不压缩
    int jpg_size = 0;    // jpeg 数据的大小
    Byte *jpg_buffer = nullptr;  // 存储编码后的 jpeg 数据

//    u8 t1 = CommonUtils::GetSystemTime_mills();

    JpegUtil::yuv2Jpeg(yuv_buffer, yuv_size, width, height, padding, quality, &jpg_buffer, jpg_size);
    if (jpg_buffer == nullptr) {
        Log::error(TAG, "Failed to encode YUV to JPEG.");
        return false;
    }

//	u8 t2 = CommonUtils::GetSystemTime_mills();
//    Log::error(LOGGER_INFO, TAG, "yuv2jpeg() 耗时 %llu 毫秒", (t2 - t1));

    free(yuv_buffer);

    /**
     * 保存 jpeg 到文件
     */

    const std::string jpegFilePath = imageDir + std::to_string(timestamp) + ".jpeg";
    Log::info(TAG, "保存硬解jpeg80图片:%s", jpegFilePath.c_str());

    FILE *fp_out = nullptr;
    if ((fp_out = fopen(jpegFilePath.c_str(), "wb+")) == nullptr) {
        Log::error(TAG, "can't open jpeg file:%s", jpegFilePath.c_str());
        return false;
    }
    fwrite(jpg_buffer, 1, jpg_size, fp_out);
    fclose(fp_out);

//    // 图片数+1
//    VideoInterceptor::imageCount += 1;

//    {
//        /**
//         * 将 jpeg80 转为 webp30
//         */
//
//        const char *const webpFilePath = point->getImagePathHardJpeg80ToWebp30().c_str();
//        bool isOk = jpeg2Webp(jpg_buffer, jpg_size, webpFilePath);
//        if (!isOk) {
//            LOG(TAG, "保存硬解jpeg80转webp30失败:%s", webpFilePath);
//        } else {
//            // 图片数+1
//            VideoInterceptor::imageCount += 1;
//        }
//    }

    if (jpg_buffer) {
        free(jpg_buffer);
    }

    return true;
}

long long int VideoInterceptor::getImageCount() {
    return VideoInterceptor::imageCount;
}

void VideoInterceptor::setImageDir(const std::string &imageDir) {
    this->imageDir = imageDir;
}

bool VideoInterceptor::saveFrameWebp(AVFrame *pFrame, int width, int height, const char *photoFilePath) {

    if (pFrame == nullptr) {
        Log::error(TAG, "pFrame == nullptr");
        return false;
    }

    uint8_t *output = nullptr;
    int size_webp = 0;
    int qualityForCompressWebp = Webp_Compression_Quality;  // 压缩质量
    int lossless = 0;  // 0有损  1无损

    // 自定义：去掉滤波
    WebpUtil::rgb2Webp(pFrame->data[0], width, height, width * 3, qualityForCompressWebp, lossless, &output, size_webp);

    // 校验
    if (size_webp <= 0 || output == nullptr) {
        Log::error(TAG, "size_webp <= 0 || output == nullptr !!!");
        return false;
    }

    // 保存文件
    FILE *fp_out = nullptr;
    if ((fp_out = fopen(photoFilePath, "wb+")) == nullptr) {
        Log::error(TAG, "can't open webp file:%s", photoFilePath);
        free(output);
        return false;
    }

    fwrite(output, 1, size_webp, fp_out);
    fclose(fp_out);

    free(output);
    return true;
}

bool VideoInterceptor::jpeg2Webp(unsigned char *jpegBuf, int jpegSize, const char *const webpFilePath) const {

    if (jpegBuf == nullptr || jpegSize == 0 || webpFilePath == nullptr) {
        Log::error(TAG, "jpegBuf == nullptr || jpegSize == 0 || webpFilePath == nullptr");
        return false;
    }

//    u8 t1 = CommonUtils::GetSystemTime_mills();

    /**
     * 将 jpeg 解码为 rgb
     */

    int jpegWidth = 0;
    int jpegHeight = 0;
    unsigned char *rgbBuf = nullptr;
    int rgbSize = 0;
    JpegUtil::jpeg2Rgb(jpegBuf, jpegSize, jpegWidth, jpegHeight, &rgbBuf, rgbSize);
    if (rgbBuf == nullptr || rgbSize <= 0) {
        Log::error(TAG, "jpeg2Rgb failed.");
        return false;
    }

    /**
     * 将 rgb 再编码为 webp
     */

    int webpSize = 0;
    unsigned char *webpBuf = nullptr;
    const int qualityForCompressWebp = Webp_Compression_Quality;  // 压缩质量
    const int lossless = 0;  // 0:有损, 1:无损
    WebpUtil::rgb2Webp(rgbBuf, jpegWidth, jpegHeight, jpegWidth * 3, qualityForCompressWebp, lossless, &webpBuf,
                       webpSize);

    // 对size_webp 进行校验
    if (webpSize <= 0 || webpBuf == nullptr) {
        Log::error(TAG, "webpSize <= 0 || webpBuf == nullptr !");
        free(rgbBuf);
        return false;
    }

    free(rgbBuf);
    rgbBuf = nullptr;

    // 保存文件
    FILE *fp_out = nullptr;
    if ((fp_out = fopen(webpFilePath, "wb+")) == nullptr) {
        Log::error(TAG, "can't open webp file:%s", webpFilePath);
        free(webpBuf);
        return false;
    }

    // 写入文件
    fwrite(webpBuf, 1, webpSize, fp_out);
    fclose(fp_out);

    free(webpBuf);
    return true;
}
