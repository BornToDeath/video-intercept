//
// Created by lixiaoqing on 2021/11/19.
//

#include "Interception.h"
#include "VideoInterceptor.h"
#include "Utils.h"
#include "Log.h"

#define TAG "Interception"


bool Interception::interceptFromVideo(const char *const videoPath, const char* const imageDir) {
    if (videoPath == nullptr || !FileUtil::isFileExist(videoPath)) {
        Log::error(TAG, "videoPath == nulltr || video file not exist.");
        return false;
    }

    // 解析视频信息
    auto videoModel = VideoInterceptor::analyseVideoInfo(videoPath);

    // 获取待截图点
    getInterceptPoints(videoModel->start, videoModel->end, videoModel->points);

    // 截图
    VideoInterceptor interceptor;
    interceptor.setImageDir(imageDir);
    interceptor.intercept(videoModel);
}

void Interception::getInterceptPoints(Timestamp start, Timestamp end, std::vector<Timestamp> &points) {
    // TODO 获取待截图点
}
