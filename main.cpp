#include <iostream>
#include <string>
#include "Interception.h"

int main() {

    const std::string videoPath = "/Users/lixiaoqing/Desktop/videos/4k/20211113214919_0060.mp4";
//    const std::string videoPath = "/Users/lixiaoqing/Desktop/videos/socol/2021-11-13_21-08-46_front.ts";
    const std::string imageDir = "/Users/lixiaoqing/Desktop/videos/4k_images/";

    Interception interception;
    interception.interceptFromVideo(videoPath.c_str(), imageDir.c_str());

    return 0;
}
