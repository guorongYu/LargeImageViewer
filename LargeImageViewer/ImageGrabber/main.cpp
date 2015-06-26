#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "Delegate.h"
#include "ExportFunctions.h"

//using namespace cv;

void MakePyramidImage(unsigned char** img, int width, int height, int numLayers)
{
    int i, j, k;

    cv::Mat src(height, width, CV_8UC1);
    cv::Mat dst, tmp;

    for (i = 0; i < height; i++) for (j = 0; j < width; j++)
        src.at<unsigned char>(i, j) = (unsigned char)img[0][i*width + j];

    tmp = src;
    dst = tmp;

    for (k = 1; k < numLayers; k++)
    {
        cv::pyrDown(tmp, dst, cv::Size(tmp.cols / 2, tmp.rows / 2));

        width = dst.cols;
        height = dst.rows;

        for (i = 0; i < height; i++) for (j = 0; j < width; j++)
            img[k][i*width + j] = dst.at<unsigned char>(i, j);

        tmp = dst;
    }
}


unsigned char** AllocateLayers(int initial_width, int initial_height, int layers)
{
    // Allocate a memory for contain image data before call MakePyramidImage().
    unsigned char** addr = new unsigned char*[layers];
    int width = initial_width;
    int height = initial_height;
    for (int i = 0; i < layers; ++i)
    {
        addr[i] = new unsigned char[width * height];
        memset(addr[i], 0, width * height);
        width = width / 2;
        height = height / 2;
    }
    return addr;
}

void FreeLayers(unsigned char** addr, int numLayers)
{
    for (int i = 0; i < numLayers; ++i)
    {
        delete[]addr[i];
    }
    delete[]addr;
}

void CVMatToByteArray(unsigned char* dst, cv::Mat& src)
{
    for (int i = 0; i < src.rows; i++) 
        for (int j = 0; j < src.cols; j++)
            dst[i*src.cols + j] = src.at<unsigned char>(i, j);
}

typedef vee::Delegate < vee::NOTHREADING, void, int > callback_type;
struct global
{
    static int numLayers;
    static int layer0_width;
    static int layer0_height;
    static unsigned char** layers;
    static callback_type update;
};

unsigned char** global::layers = nullptr;
int global::layer0_width = 0;
int global::layer0_height = 0;
int global::numLayers = 0;
callback_type global::update;

__declspec(dllexport) void __stdcall AddUpdateCallback(callback_type::funcptr_type f)
{
    global::update += f;
}

__declspec(dllexport) int __stdcall GetLayer0Width()
{
    return global::layer0_width;
}

__declspec(dllexport) int __stdcall GetLayer0Height()
{
    return global::layer0_height;
}

__declspec(dllexport) int __stdcall NumbefOfLayers()
{
    return global::numLayers;
}

__declspec(dllexport) int __stdcall GetLayerWidth(int index)
{
    int width = global::layer0_width;
    for (int i = 0; i < index; ++i)
    {
        width /= 2;
    }
    return width;
}

__declspec(dllexport) int __stdcall GetLayerHeight(int index)
{
    int height = global::layer0_height;
    for (int i = 0; i < index; ++i)
    {
        height /= 2;
    }
    return height;
}

__declspec(dllexport) int __stdcall GetLayerBufferPitch(int index)
{
    int ret = GetLayerWidth(index) * GetLayerHeight(index);
    return ret;
}

__declspec(dllexport) void* __stdcall GetLayerData(unsigned char* dst, int index)
{
    if (dst == nullptr)
        global::layers[index];
    memmove(dst, global::layers[index], GetLayerBufferPitch(index));
    return global::layers[index];
}

__declspec(dllexport) void* __stdcall GetLayerDataOfRegion(unsigned char* dst, int index, int left, int top, int right, int bottom)
{
    if (dst == nullptr)
        global::layers[index];
    int width = right - left;
    int height = bottom - top;
    printf("(%d, %d) (%d, %d) %d x %d\n", left, top, right, bottom, width, height);
    int layer_width = GetLayerWidth(index);
    int offset = (layer_width * top) + left;
    for (int i = 0; i < height; ++i)
    {
        memmove(dst + (i * width), &(global::layers[index][0]) + offset, width);
        offset += layer_width;
    }
    global::update(1);
    return global::layers[index];
}

__declspec(dllexport) int __stdcall OnLoad()
{
    cv::Mat src;
    src = cv::imread("input.bmp", CV_LOAD_IMAGE_GRAYSCALE);
    if (!src.data)
    {
        printf("Failed to open file! file not found.\n");
        return 0;
    }
    global::layer0_width  = src.cols;
    global::layer0_height = src.rows;
    global::numLayers = 7;
    global::layers = AllocateLayers(global::layer0_width, 
                                    global::layer0_height, 
                                    global::numLayers);
    CVMatToByteArray(global::layers[0], src);
    MakePyramidImage(global::layers, 
                     global::layer0_width, 
                     global::layer0_height, 
                     global::numLayers);
    printf("Success to load dll!\n");
    return 0;
}

int main()
{
    cv::Mat src;
    src = cv::imread("input.bmp", CV_LOAD_IMAGE_GRAYSCALE);
    if (!src.data)
    {
        printf("Failed to open file! file not found.\n");
        return 0;
    }

    unsigned char** layers = AllocateLayers(src.cols, src.rows, 7);
    CVMatToByteArray(layers[0], src);
    //////////////////////////////////////// img[0][]에 원본 데이터를 넣어주면, layer만큼 img[1][], ... img[layer-1][]의 영상을 만들어 줌
    MakePyramidImage(layers, src.cols, src.rows, 7);

    int width  = src.cols;
    int height = src.rows;
    for (int k = 0; k < 7; k++)
    {
        cv::Mat display(height, width, CV_8UC1);
        for (int i = 0; i < height; i++) 
            for (int j = 0; j < width; j++)
            display.at<unsigned char>(i, j) = layers[k][i*width + j];

        char name[100];
        sprintf_s(name, "prd%03d.bmp", k);
        imwrite(name, display);

        width /= 2;
        height /= 2;

        cv::waitKey(0);
    }

    FreeLayers(layers, 7);
}