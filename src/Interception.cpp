//
// Created by lixiaoqing on 2021/11/19.
//

#include "Interception.h"
#include "VideoInterceptor.h"
#include "Utils.h"
#include "Log.h"

#define TAG "Interception"


void Interception::interceptFromVideo(const char *const videoPath, const char* const imageDir) {
    if (videoPath == nullptr || !FileUtil::isFileExist(videoPath)) {
        Log::error(TAG, "videoPath == nulltr || video file not exist.");
        return;
    }

    // 解析视频信息
    auto videoModel = VideoInterceptor::analyseVideoInfo(videoPath);

    // 获取待截图点
    getInterceptPoints(videoModel->start, videoModel->end, videoModel->points);
    if (videoModel->points.empty()) {
        Log::warn(TAG, "视频没有截图点! video:%s", videoPath);
        return;
    }

    // 截图
    VideoInterceptor interceptor;
    interceptor.setImageDir(imageDir);
    interceptor.intercept(videoModel);
}

void Interception::getInterceptPoints(Timestamp start, Timestamp end, std::vector<Timestamp> &points) {
    // TODO 获取待截图点
}
