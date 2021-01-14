# 生成素材

## 1. 生成一段 yuv 图像序列

```shell
ffmpeg -i input.mp4 -pix_fmt yuv420p -ss 00:00:00 -t 00:00:10 output.yuv

# 播放
ffplay -f rawvideo -pix_fmt yuv420p -video_size 720x1280 output.yuv
```



## 2. 生成一张 yuv 图像

```shell
ffmpeg -i intput.mp4 -f rawvideo -pix_fmt yuv420p -vframes 1 output.yuv

# 播放
ffplay -f rawvideo -pix_fmt yuv420p -video_size 720x1280 output.yuv
```



## 3. 生成一张 rgba 图像

```shell
ffmpeg -i intput.mp4 -f rawvideo -pix_fmt rgba -vframes 1 output.rgba

# 播放
ffplay -f rawvideo -pix_fmt rgba -video_size 720x1280 output.rgba
```

