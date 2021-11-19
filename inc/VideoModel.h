//
// Created by lixiaoqing on 2021/6/3.
//

#ifndef VIDEOINTERCEPT_VIDEOMODEL_H
#define VIDEOINTERCEPT_VIDEOMODEL_H

#include <string>
#include <vector>
#include "Define.h"

class VideoModel final {

public:

    VideoModel() : videoPath(), start(0), end(0) {}

    ~VideoModel() = default;

public:

    /**
     * 视频文件路径
     */
    std::string videoPath;

    /**
     * 视频开始时间（Unix时间戳：单位：毫秒）
     */
    Timestamp start;

    /**
     * 视频结束时间（Unix时间戳，单位：毫秒）
     */
    Timestamp end;

    /**
     * 当前视频匹配上的截图点
     */
    std::vector<Timestamp> points;
};


#endif //VIDEOINTERCEPT_VIDEOMODEL_H
