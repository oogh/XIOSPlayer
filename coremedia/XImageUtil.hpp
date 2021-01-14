//
// Created by Oogh on 2020/3/19.
//

#ifndef ANDROIDDEMO_XIMAGEUTIL_H
#define ANDROIDDEMO_XIMAGEUTIL_H

#include <cstdio>

struct AVFrame;

/*
     使用示例如下：
     /////////////////////////////////////////////////////

     char filename[256];
     // iOS路径写法
     snprintf(filename, sizeof(filename), "%s/out_%dx%d_%d.rgba",
     XImage::getIOSDebugDirectory().data(), image->width, image->height, ++count);

     // Android路径写法
     snprintf(filename, sizeof(filename), "/sdcard/out_%dx%d_%d.rgba", image->width, image->height, ++count);
     GDImageUitl::rgba2File(pixels, width, height, filename);

     ////////////////////////////////////////////////////////////
*/

struct XImageUtil {
    /**
     * @brief 保存RGBA数据到指定文件中
     * @details 保存到文件中的是rgba裸数据，播放使用ffplay播放，命令是：
     * ffplay -f rawvideo -pix_fmt rgba -video_size 540x960 out_540x960.rgba
     * @param pixels rgba像素数据
     * @param width 宽度
     * @param height 高度
     * @param filename 文件名，比如 "/sdcard/out_540x960.rgba"
     */
    static void rgba2File(void* pixels, int width, int height, const char* filename) {
        FILE* fp = fopen(filename, "w+");
        if (fp) {
            fwrite(pixels, width * height * 4, 1, fp);
            fclose(fp);
        }
    }

    /**
     * @brief 保存YUV420P数据到指定文件中
     * @details 保存到文件中的是yuv420p裸数据，播放使用ffplay播放，命令是：
     * ffplay -f rawvideo -pix_fmt yuv -video_size 700x1240 out_700x1240.yuv
     * @param AVFrame 带有yuv420p数据的AVFrame
     * @param filename 文件名，比如 "/sdcard/out_700x1240.yuv"
     */
    static void yuv420p2File(AVFrame* frame, const char* filename) {
//        FILE *fp = fopen(filename, "ab");
//        if (fp) {
//            int linesizeY = frame->linesize[0];
//            int linesizeU = frame->linesize[1];
//            int linesizeV = frame->linesize[2];
//
//            uint8_t *Y = frame->data[0];
//            uint8_t *U = frame->data[1];
//            uint8_t *V = frame->data[2];
//
//            for (int i = 0; i < frame->height; i++) {
//                fwrite(Y, static_cast<size_t>(frame->width), 1, fp);
//                Y += linesizeY;
//            }
//
//            for (int i = 0; i < frame->height / 2; i++) {
//                fwrite(U, static_cast<size_t>(frame->width / 2), 1, fp);
//                U += linesizeU;
//            }
//
//            for (int i = 0; i < frame->height / 2; i++) {
//                fwrite(V, static_cast<size_t>(frame->width / 2), 1, fp);
//                V += linesizeV;
//            }
//        }
//        fclose(fp);
    }

    static void yuv2File(uint8_t* pixels[], int width, int height, const char* filename) {
        FILE *fp = fopen(filename, "ab");
        if (fp) {
            int linesizeY = width;
            int linesizeU = width / 2;
            int linesizeV = width / 2;

            uint8_t *Y = pixels[0];
            uint8_t *U = pixels[1];
            uint8_t *V = pixels[2];

            for (int i = 0; i < height; i++) {
                fwrite(Y, static_cast<size_t>(width), 1, fp);
                Y += linesizeY;
            }

            for (int i = 0; i < height / 2; i++) {
                fwrite(U, static_cast<size_t>(width / 2), 1, fp);
                U += linesizeU;
            }

            for (int i = 0; i < height / 2; i++) {
                fwrite(V, static_cast<size_t>(width / 2), 1, fp);
                V += linesizeV;
            }
        }
        fclose(fp);
    }
};

#endif //ANDROIDDEMO_XIMAGEUTIL_H
