#include <iostream>
#include <string>
#include "VideoInterceptor.h"
#include "Utils.h"

int main() {

//    std::string videoPath = "/Users/lixiaoqing/Desktop/videos/socol/2021-11-13_21-08-46_front.ts";
//    FileUtil::printFileInfo(videoPath.c_str());

    const std::string videoPath = "/Users/lixiaoqing/Desktop/videos/4k/20211113214919_0060.mp4";
//    const std::string videoPath = "/Users/lixiaoqing/Desktop/videos/socol/2021-11-13_21-08-46_front.ts";
    const std::string imageDir = "/Users/lixiaoqing/Desktop/videos/4k_images/";

    VideoInterceptor interceptor;
    interceptor.setImageDir(imageDir);
    interceptor.intercept(videoPath);

    return 0;
}
