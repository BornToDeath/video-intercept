# 视频截图

本工程可以提供视频截图的功能。

## 依赖的三方库

本工程依赖如下三方库：
1. ffmpeg
2. turbojpeg
3. webp

## 使用方法

```c++
#include "Interception.h"

const std::string videoPath = "/Users/lxq/Desktop/videos/4k/20211113214919_0060.mp4";
const std::string imageDir = "/Users/lxq/Desktop/videos/4k_images/";

Interception interception;
interception.interceptFromVideo(videoPath.c_str(), imageDir.c_str());
```