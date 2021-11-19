//
// Created by lixiaoqing on 2021/11/19.
//

#ifndef VIDEOINTERCEPT_INTERCEPTION_H
#define VIDEOINTERCEPT_INTERCEPTION_H

#include <iostream>
#include <vector>
#include "Define.h"

class Interception final {

public:

    Interception() = default;

    ~Interception() = default;

public:

    /**
     * 从视频文件中截图
     * @param videoPath
     * @param imageDir
     * @return
     */
    bool interceptFromVideo(const char *const videoPath, const char* const imageDir);

private:

    /**
     * 获取介于 start 和 end 之间的截图点
     * @param start
     * @param end
     * @param points
     */
    void getInterceptPoints(Timestamp start, Timestamp end, std::vector<Timestamp> &points);
};

#endif //VIDEOINTERCEPT_INTERCEPTION_H
