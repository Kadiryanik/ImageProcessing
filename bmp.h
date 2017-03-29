#ifndef BMP_H
#define BMP_H

#include <iostream>
#include <fstream>

using namespace std;

typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef unsigned short WORD;

#define HAAR_MODEL_1 1 // 1x2
#define HAAR_MODEL_2 2 // 2x1
#define HAAR_MODEL_3 3 // 1x3
#define HAAR_MODEL_4 4 // 3x1
#define HAAR_MODEL_5 5 // 2x2

/* TODO: intel dışında işlemciyle çalışılcaksa parser creater yazılcak */
typedef struct tagBITMAPFILEHEADER{
    WORD    bfType;             // 2 /* Magic identifier */
    DWORD   bfSize;             // 4 /* File size in bytes */
    WORD    bfReserved1;        // 2
    WORD    bfReserved2;        // 2
    DWORD   bfOffBits;          // 4 /* Offset to image data, bytes */
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
    DWORD    biSize;            // 4 /* Header size in bytes */
    int      biWidth;           // 4 /* Width of image */
    int      biHeight;          // 4 /* Height of image */
    WORD     biPlanes;          // 2 /* Number of colour planes */
    WORD     biBitCount;        // 2 /* Bits per pixel */
    DWORD    biCompression;     // 4 /* Compression type */
    DWORD    biSizeImage;       // 4 /* Image size in bytes */
    int      biXPelsPerMeter;   // 4 /* Pixels per meter */
    int      biYPelsPerMeter;   // 4 /* Pixels per meter */
    DWORD    biClrUsed;         // 4 /* Number of colours */
    DWORD    biClrImportant;    // 4 /* Important colours */
} __attribute__((packed)) BITMAPINFOHEADER;

typedef struct{
    BYTE    b;
    BYTE    g;
    BYTE    r;
} RGB_data;

typedef struct HAAR_MODELS{
    int height;
    int width;
} haarModels_t;

typedef struct MASK{
    int width;
    int height;
    int type;
    int* mask;

    MASK(int w, int h, int t){
        width = w;
        height = h;
        type = t;
        mask = new int[width * height];
        if(type){
            for(int i = 0; i < width*height; i++) mask[i] = 0;
        }
   }
    ~MASK(){
        delete[] mask;
    }
}mask_t;

/* load-save file */
BYTE* loadBMP(int* width, int* height, long* size, ifstream &file);
int saveBMP(const char *filename, int width, int height, BYTE *data);
/* BMP to Intensity, Intensity to BMP */
BYTE* convertBMPToIntensity(BYTE* Buffer, int width, int height);
BYTE* convertIntensityToBMP(BYTE* Buffer, int width, int height, long* newsize);
BYTE* convertIntensityToColoredBMP(BYTE* Buffer, int* allT, int k, int width, int height, long* newsize);
/* Draw */
int drawCircle(int x, int y, int r, BYTE* ramIntensity, int width, int height);
int drawRect(int x, int y, int rectX, int rectY, BYTE* ramIntensity, int width, int height);
int drawElips(int x, int y, float a, float b, BYTE* ramIntensity, int width, int height);
int drawPlus(int x, int y, int width, BYTE* ramIntensity);
/* Integral Image */
DWORD* getIntegralImage(BYTE* ramIntensity, int width, int height);
DWORD* getAllIntegralImages(BYTE* examples, int width, int height, int exampleNumber);
int getTotal(const DWORD* const integralImage, int x, int y, int lastX, int lastY, int width, int height);
int getTotalFeature(const DWORD* const integralImage, int x, int y, int lastX, int lastY, int width, int height);
/* Haar */
//int* haarCascade(int hModel, int size, const DWORD* const integralImage, int width, int height);
int* haarFeature(int hModel, int kHeight, int kWidth, const DWORD* const integralImage, int width, int height, int exampleNumber);
void calculate(int* haarVectorFace, int* haarVectorNonFace, int N, int M);
/* Blur */
BYTE* blurMean(const BYTE* const noise, int widthN, int heightN);
BYTE* blurGaussian(const BYTE* const noised, int widthN, int heightN);
BYTE* blurMedian(const BYTE* const noised, int widthN, int heightN);
BYTE getMiddle(const BYTE* const p1, const BYTE* const p2, const BYTE* const p3);
/* thresHold */
int thresHoldOtsu(const BYTE* const ramIntensity, int width, int height);
int thresHold(const BYTE* const ramIntensity, int width, int height);
BYTE* getDilation(BYTE* binaryImage, int width, int height, mask_t *dilation);
BYTE* getErosion(BYTE* binaryImage, int width, int height, mask_t *erosion);
BYTE* getFrame(BYTE* binaryImage, int width, int height);
BYTE* regionIdentification(BYTE* binaryImage, int width, int height);

int* thresHoldWithK(const BYTE* const ramIntensity, int k, int width, int height);
int* getAllT(int* tHold, int k, int width, int height);
/* For Debug */
void printStructers(BITMAPFILEHEADER h, BITMAPINFOHEADER i);

#endif