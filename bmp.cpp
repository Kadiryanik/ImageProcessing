#include <iostream>
#include <fstream>
#include <math.h>       // for fabs
#include <cstring>      // for memset
#include <iomanip>      // for setw
#include <ctime>
#include <cstdlib>

#include "bmp.h"

using namespace std;

#define DEBUG 1
#define LOAD_ANYWAY 0
#define PRINTSIZE 6

#define WITH_MAHALONOBIS 0

#define PI 3.141592

#if DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif

static int labelHolder = 0;

/*--------------------------------------------------------------------------------------*/
BYTE* loadBMP(int* height, int* width, long* size, ifstream &file){
    // declare bitmap structures
    BITMAPFILEHEADER bmpHead;
    BITMAPINFOHEADER bmpInfo;

    // value to be used in ReadFile funcs
    DWORD bytesread;

    // open file to read from
    if (NULL == file)
        return NULL; // coudn't open file

    // read file header
    if (!file.read(reinterpret_cast<char*>(&bmpHead), sizeof(BITMAPFILEHEADER))){
        dout << "Error: file.read header" << endl;
        file.close();
        return NULL;
    }
    //read bitmap info
    if (!file.read(reinterpret_cast<char*>(&bmpInfo), sizeof(BITMAPINFOHEADER))){
        dout << "Error: file.read info" << endl;
        file.close();
        return NULL;
    }
#if !LOAD_ANYWAY
    // check if file is actually a bmp
    if (bmpHead.bfType != 'MB'){
        dout << "Error: file type" << endl;
        file.close();
        return NULL;
    }
#endif
    // get image measurements
    *width = bmpInfo.biWidth;
    *height = fabs(bmpInfo.biHeight);
#if !LOAD_ANYWAY
    // check if bmp is uncompressed
    if (bmpInfo.biCompression != 0){
        dout << "Error: biCompression != 0" << endl;
        file.close();
        return NULL;
    }
    // check if we have 24 bit bmp
    if (bmpInfo.biBitCount != 24) {
        dout << "Error: bitCount != 24 bit" << endl;
        file.close();
        return NULL;
    }
#endif
    // create buffer to hold the data
    *size = bmpHead.bfSize - bmpHead.bfOffBits;
    BYTE* Buffer = new BYTE[*size];
    // move file pointer to start of bitmap data
    file.seekg(bmpHead.bfOffBits, ios::beg);

    // read bmp data
    if (!file.read(reinterpret_cast<char*>(Buffer), *size))  {
        dout << "Error: file.read data" << endl;
        delete[] Buffer;
        file.close();
        return NULL;
    }
    // everything successful here: close file and return buffer
    file.close();

#if DEBUG > 9
    dout << "loadBMP headers" << endl;
    printStructers(bmpHead, bmpInfo);
#endif
    return Buffer;
}
/*--------------------------------------------------------------------------------------*/
int saveBMP(const char *filename, int height, int width, BYTE *data){
    BITMAPFILEHEADER bmpHead;
    BITMAPINFOHEADER bmpInfo;
    int size = width * height * 3;

    // andinitialize them to zero
    memset(&bmpHead, 0, sizeof (BITMAPFILEHEADER));
    memset(&bmpInfo, 0, sizeof (BITMAPINFOHEADER));

    // fill headers
    bmpHead.bfType = 0x4D42; // 'BM'
    bmpHead.bfSize= size + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // size + head + info no quad    
    bmpHead.bfReserved1 = bmpHead.bfReserved2 = 0;
    bmpHead.bfOffBits = bmpHead.bfSize - size; //equals 0x36 (54)
    // finish the initial of head

    bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.biWidth = width;
    bmpInfo.biHeight = height;
    bmpInfo.biPlanes = 1;
    bmpInfo.biBitCount = 24;            // bit(s) per pixel, 24 is true color
    bmpInfo.biCompression = 0;          //BI_RGB
    bmpInfo.biSizeImage = 0;
    bmpInfo.biXPelsPerMeter = 0x0ec4;   // paint and PSP use this values
    bmpInfo.biYPelsPerMeter = 0x0ec4;
    bmpInfo.biClrUsed = 0;
    bmpInfo.biClrImportant = 0;
    // finish the initial of infohead

    ofstream file;
    file.open(filename, ios::out | ios::binary | ios::app);
    
    if(!file.is_open()) return 0;

    if(!file.write(reinterpret_cast<char*>(&bmpHead), sizeof(BITMAPFILEHEADER))){
        file.close();
        return 0;
    }
    if(!file.write(reinterpret_cast<char*>(&bmpInfo), sizeof(BITMAPINFOHEADER))){
        file.close();
        return 0;
    }
    if(!file.write(reinterpret_cast<char*>(data), size)){
        file.close();
        return 0;
    }

    file.close();

#if DEBUG > 9
    dout << endl << "saveBMP headers" << endl;
    printStructers(bmpHead, bmpInfo);
#endif
    return 1;
}
/*--------------------------------------------------------------------------------------*/
BYTE* convertBMPToIntensity(BYTE* Buffer, int width, int height){
    // first make sure the parameters are valid
    if ((NULL == Buffer) || (width == 0) || (height == 0))
        return NULL;

    // find the number of padding bytes

    int padding = 0;
    int scanlinebytes = width * 3;
    while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
        padding++;
    // get the padded scanline width
    int psw = scanlinebytes + padding;

    // create new buffer
    BYTE* newbuf = new BYTE[width*height];

    // now we loop trough all bytes of the original buffer, 
    // swap the R and B bytes and the scanlines
    long bufpos = 0;
    long newpos = 0;
    for (int row = 0; row < height; row++)
    for (int column = 0; column < width; column++)  {
              newpos = row * width + column;
              bufpos = (height - row - 1) * psw + column * 3;
              newbuf[newpos] = BYTE(0.11*Buffer[bufpos + 2] + 0.59*Buffer[bufpos + 1] + 0.3*Buffer[bufpos]);
          }

    return newbuf;
}
/*--------------------------------------------------------------------------------------*/
BYTE* convertIntensityToBMP(BYTE* Buffer, int width, int height, long* newsize){
    // first make sure the parameters are valid
    if ((NULL == Buffer) || (width == 0) || (height == 0))
        return NULL;

    // now we have to find with how many bytes
    // we have to pad for the next DWORD boundary   

    int padding = 0;
    int scanlinebytes = width * 3;
    while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
        padding++;
    // get the padded scanline width
    int psw = scanlinebytes + padding;
    // we can already store the size of the new padded buffer
    *newsize = height * psw;

    // and create new buffer
    BYTE* newbuf = new BYTE[*newsize];

    // fill the buffer with zero bytes then we dont have to add
    // extra padding zero bytes later on
    memset(newbuf, 0, *newsize);

    // now we loop trough all bytes of the original buffer, 
    // swap the R and B bytes and the scanlines
    long bufpos = 0;
    long newpos = 0;
    for (int row = 0; row < height; row++)
    for (int column = 0; column < width; column++)      {
        bufpos = row * width + column;                  // position in original buffer
        newpos = (height - row - 1) * psw + column * 3; // position in padded buffer
        newbuf[newpos] = Buffer[bufpos];                //  blue
        newbuf[newpos + 1] = Buffer[bufpos];            //  green
        newbuf[newpos + 2] = Buffer[bufpos];            //  red
    }

    return newbuf;
}
/*--------------------------------------------------------------------------------------*/
BYTE* convertIntensityToColoredBMP(BYTE* Buffer, int* allT, int k, int width, int height, long* newsize){
    RGB_data colors[1 + k];
    int i;
    for(i = 0; i < k; i++){

        if(k <= 4){
            colors[0].b = 0;
            colors[0].g = 0;
            colors[0].r = 255;
            
            colors[1].b = 0;
            colors[1].g = 255;
            colors[1].r = 0;
            
            colors[2].b = 255;
            colors[2].g = 0;
            colors[2].r = 0;

            colors[3].b = 0;
            colors[3].g = 255;
            colors[3].r = 255;

            colors[4].b = 255;
            colors[4].g = 255;
            colors[4].r = 255;
        } else{
            dout << "Colors" << endl;
            for(i = 0; i < k; i++){
                colors[i].b = rand() % 255;
                colors[i].g = rand() % 255;
                colors[i].r = rand() % 255;
                dout << i + 1 << "- RGB - " << (int)colors[i].r << " " << (int)colors[i].g << " " << (int)colors[i].b << endl;
            }
        }
    }
    // first make sure the parameters are valid
    if ((NULL == Buffer) || (width == 0) || (height == 0))
        return NULL;

    // now we have to find with how many bytes
    // we have to pad for the next DWORD boundary   

    int padding = 0;
    int scanlinebytes = width * 3;
    while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
        padding++;
    // get the padded scanline width
    int psw = scanlinebytes + padding;
    // we can already store the size of the new padded buffer
    *newsize = height * psw;

    // and create new buffer
    BYTE* newbuf = new BYTE[*newsize];

    // fill the buffer with zero bytes then we dont have to add
    // extra padding zero bytes later on
    memset(newbuf, 0, *newsize);

    // now we loop trough all bytes of the original buffer, 
    // swap the R and B bytes and the scanlines
    long bufpos = 0;
    long newpos = 0;
    for (int row = 0; row < height; row++)
    for (int column = 0; column < width; column++)      {
        bufpos = row * width + column;                  // position in original buffer
        newpos = (height - row - 1) * psw + column * 3; // position in padded buffer
        for(i = 0; i < k; i++){
            if(Buffer[bufpos] == allT[i]){
                break;
            }
        }
        newbuf[newpos] = colors[i].b;                //  blue
        newbuf[newpos + 1] = colors[i].g;            //  green
        newbuf[newpos + 2] = colors[i].r;            //  red
    }

    return newbuf;
}
/*--------------------------------------------------------------------------------------*/
int drawCircle(int x, int y, int r, BYTE* ramIntensity, int width, int height){
    int i, newX, newY, isDrawed = 0;

    BYTE* temp;
    BYTE* ramIntensityEnd = ramIntensity + width * height;

    for(i = 0; i < 360; i++){
        newX = r * cos(PI*i/180) + x;
        newY = r * sin(PI*i/180) + y;

        temp = ramIntensity + (width * newX) + newY;
        
        if(temp < ramIntensityEnd && temp >= ramIntensity && newY > 0 && newY < width){
            *(temp) = 0xff;
            isDrawed = 1;
        }
    }

    if(!isDrawed){
        return 0;
    }
    return 1;
}
/*--------------------------------------------------------------------------------------*/
/*   A ---------------- D
*      |      |(rectY)|
*      |------.M(x, y)|
*      |(rectX)       |
*    B ---------------- C
*/
int drawRect(int x, int y, int rectX, int rectY, BYTE* ramIntensity, int width, int height){
    if(x < rectX || y < rectY || x > width || y > height){
        //cant draw this rect
        return 0;
    }

    int i, cordinateAx, cordinateAy;

    cordinateAx = x - rectX;
    cordinateAy = y - rectY;

    for(i = 0; i < 2 * rectX; i++){
        //Draw A to B
        *(ramIntensity + (width * (cordinateAx + i)) + cordinateAy) = 0xff;
        //Draw D to C
        *(ramIntensity + (width * (cordinateAx + i)) + (cordinateAy + 2 * rectY)) = 0xff;
    }
    for(i = 0; i < 2 * rectY; i++){
        //Draw A to D
        *(ramIntensity + (width * cordinateAx) + (cordinateAy + i)) = 0xff;
        //Draw B to C
        *(ramIntensity + (width * (cordinateAx + 2 * rectX)) + (cordinateAy + i)) = 0xff;
    }

    return 1;
}
/*--------------------------------------------------------------------------------------*/
/*      ==
*      /-|\
*    --  | --
*   /   b|   \
*  |     .----|
*   \   M  a /
*    --    --
*      \--/
*       ==
*/
int drawElips(int x, int y, float a, float b, BYTE* ramIntensity, int width, int height){
    int i, newX, newY, isDrawed = 0;

    BYTE* temp;
    BYTE* ramIntensityEnd = ramIntensity + width * height;
    
    for(i = 0; i < 360; i++){
        newX = x + b * cos(PI*i/180);
        newY = y + a * sin(PI*i/180);

        temp = ramIntensity + (width * newX) + newY;
        
        if(temp < ramIntensityEnd && temp >= ramIntensity && newY > 0 && newY < width){
            *(temp) = 0xff;
            isDrawed = 1;
        }
    }
    if(!isDrawed){
        return 0;
    }
    return 1;
}
/*--------------------------------------------------------------------------------------*/
int drawPlus(int x, int y, int width, BYTE* ramIntensity){
    *(ramIntensity + (width * x) + y) = 0xff;
    *(ramIntensity + (width * (x + 1)) + y) = 0xff;
    *(ramIntensity + (width * (x + 2)) + y) = 0xff;
    *(ramIntensity + (width * (x - 1)) + y) = 0xff;
    *(ramIntensity + (width * (x - 2)) + y) = 0xff;

    *(ramIntensity + (width * x) + y) = 0xff;
    *(ramIntensity + (width * x) + y + 1) = 0xff;
    *(ramIntensity + (width * x) + y + 2) = 0xff;
    *(ramIntensity + (width * x) + y - 1) = 0xff;
    *(ramIntensity + (width * x) + y - 2) = 0xff;
    return 1;
}
/*--------------------------------------------------------------------------------------*/
BYTE* blurMean(const BYTE* const noised, int widthN, int heightN){
    //int *mean = new int[3 * 3];
    //memset(mean, 1, 9 * sizeof(int));
    BYTE* blured = new BYTE[ widthN * heightN ];
    memset(blured, 0, widthN * heightN);
    
    int i, j;
    for(i = 0; i < heightN - 3; i++){
        for(j = 0; j < widthN - 3; j++){
            *(blured + ((i + 1) * widthN) + j + 1) = (*(noised + i * widthN + j)
                                                        + *(noised + i * widthN + j + 1)
                                                        + *(noised + i * widthN + j + 2)
                                                    + *(noised + ((i + 1) * widthN) + j)
                                                        + *(noised + ((i + 1) * widthN ) + j + 1)
                                                        + *(noised + ((i + 1) * widthN) + j + 2)
                                                    + *(noised + ((i + 2) * widthN) + j)
                                                        + *(noised + ((i + 2) * widthN) + j + 1)
                                                        + *(noised + ((i + 2) * widthN) + j + 2)) / 9;
        }
    }
    return blured;
}
/*--------------------------------------------------------------------------------------*/
BYTE* blurGaussian(const BYTE* const noised, int widthN, int heightN){
    int *gaussian = new int[3 * 3];
    *(gaussian) = 2;
    *(gaussian + 1) = 1;
    *(gaussian + 2) = 2;
    *(gaussian + 3) = 1;
    *(gaussian + 4) = 0;
    *(gaussian + 5) = 1;
    *(gaussian + 6) = 2;
    *(gaussian + 7) = 1;
    *(gaussian + 8) = 2;

    BYTE* blured = new BYTE[ widthN * heightN ];
    memset(blured, 0, widthN * heightN);

    int i, j;
    for(i = 0; i < heightN - 3; i++){
        for(j = 0; j < widthN - 3; j++){
            *(blured + ((i + 1) * widthN) + j + 1) = ((*(noised + i * widthN + j) >> *(gaussian))
                                                        + (*(noised + i * widthN + j + 1) >> *(gaussian + 1))
                                                        + (*(noised + i * widthN + j + 2) >> *(gaussian + 2))
                                                    + (*(noised + ((i + 1) * widthN) + j) >> *(gaussian + 3))
                                                        + (*(noised + ((i + 1) * widthN ) + j + 1) >> *(gaussian + 4))
                                                        + (*(noised + ((i + 1) * widthN) + j + 2) >> *(gaussian + 5))
                                                    + (*(noised + ((i + 2) * widthN) + j) >> *(gaussian + 6))
                                                        + (*(noised + ((i + 2) * widthN) + j + 1) >> *(gaussian + 7))
                                                        + (*(noised + ((i + 2) * widthN) + j + 2) >> *(gaussian + 8))
                                                    ) >> 2;
        }
    }
    return blured;
}
/*--------------------------------------------------------------------------------------*/
BYTE* blurMedian(const BYTE* const noised, int widthN, int heightN){
    BYTE* blured = new BYTE[ widthN * heightN ];
    memset(blured, 0, widthN * heightN);

    int i, j;
    for(i = 0; i < heightN - 3; i++){
        for(j = 0; j < widthN - 3; j++){
            *(blured + ((i + 1) * widthN) + j + 1) = getMiddle(noised + i * widthN + j,
                                                                noised + ((i + 1) * widthN) + j,
                                                                noised + ((i + 2) * widthN) + j);
        }
    }
    return blured;
}
/*--------------------------------------------------------------------------------------*/
BYTE getMiddle(const BYTE* const p1, const BYTE* const p2, const BYTE* const p3){
    BYTE array[9] = { 0 };
    int i, j;
    for(i = 0; i < 3; i++){
        array[i] = *(p1 + i);
        array[3 + i] = *(p2 + i);
        array[6 + i] = *(p3 + i);
    }
    
    for(j = 0; j < 8; j++){
        for(i = 0; i < 8; i++){
            if((int)array[i] >= (int)array[i + 1]){
                BYTE temp = array[i];
                array[i] = array[i + 1];
                array[i + 1] = temp;
            }
        }
    }
    return array[4];
}
/*--------------------------------------------------------------------------------------*/
int thresHoldOtsu(const BYTE* const ramIntensity, int width, int height){
    int* tHold = new int[256];
    int i, pixelTotal, thValue;
    DWORD sum = 0;
    memset(tHold, 0, 256 * sizeof(int));

    for(i = 0; i < width * height; i++){
            tHold[(int)*(ramIntensity + i)]++;
    }
    for (i = 0; i < 256; i++){
        sum += i * (*(tHold + i));
    }
    pixelTotal = width * height;
    
    float sumB = 0;
    int wB = 0;
    int wF = 0;
    float varMax = 0;
    thValue = 0;


    for (i = 0; i < 256; i++) {
        wB += *(tHold + i);
        if (wB == 0) continue;
        wF = pixelTotal - wB;
        if (wF == 0) break;
        sumB += (float)(i * (*(tHold + i)));
        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;

        float varBetween = (float) (wB * wF) * (mB - mF) * (mB - mF);
        
        
        if (varBetween > varMax) {
            varMax = varBetween;
            thValue = i;
        }
    }
    dout << "thValue = " << 255 - thValue << endl;
#if DEBUG > 1
    for(i = 0; i < 256; i++){
        dout << *(tHold + i) << ", ";
    }
    dout << endl;
#endif /* DEBUG > 1 */
    return 255 - thValue;
}
/*--------------------------------------------------------------------------------------*/
int thresHold(const BYTE* const ramIntensity, int width, int height){
    int* tHold = new int[ 256 ];
    int i, pixelTotal, thValue;
    DWORD sum = 0;
    memset(tHold, 0, 256 * sizeof(int));

    for(i = 0; i < width * height; i++){
            tHold[(int)*(ramIntensity + i)]++;
    }
    double total = 0;
    for (i = 0; i < 256; i++){
        sum += i * (*(tHold + i));
        if(*(tHold + i)) total++;
    }
    dout << endl;
#if WITH_MAHALONOBIS
    double mean = sum / 256.0;
    double sumForVariance = 0;
    for (i = 0; i < 256; i++){
        sumForVariance += pow(i - mean, 2) * (*(tHold + i));
    }
    
    double v2 = sumForVariance / total; // varyans^2
    cout << "v2 = " << v2 << endl;
#endif /* WITH_MAHALONOBIS */
    int done = 1;
    int T1 = 0, T2 = 255;
    while(done){
        DWORD sumT1 = 0, sumT2 = 0;
        float sumT1i = 0, sumT2i = 0;

        for(i = 0; i < 256; i++){
#if WITH_MAHALONOBIS
            if(sqrt(pow(T1 - i, 2) / v2) < sqrt(pow(T2 - i, 2) / v2)){
#else
            if(fabs(T1 - i) < fabs(T2 - i)){
#endif /* WITH_MAHALONOBIS */
                sumT1 += i * (*(tHold + i));
                sumT1i += *(tHold + i);
            } else{
                sumT2 += i * (*(tHold + i));
                sumT2i += *(tHold + i);
            }
        }
        if(sumT1i == 0){
            sumT1i = 1;
        }
        if(sumT2i == 0){
            sumT2i = 1;
        }
        int T1u = sumT1/sumT1i;
        int T2u = sumT2/sumT2i;
        float epsilon = 2;
        if((fabs(T1 - T1u) < epsilon && fabs(T2 - T2u) < epsilon) || \
             fabs(T1 - T2u) < epsilon && fabs(T2 - T1u) < epsilon){
            dout << "T1 = " << T1 << endl;
            dout << "T2 = " << T2 << endl;
            done = 0;
        }else{
            T1 = T1u;
            T2 = T2u;
        }
    }
#if 0
    int min = T1;
    for(int i = (T1 < T2 ? T1 : T2); i < (T1 < T2 ? T2 : T1); i++){
        if(*(tHold + min) > *(tHold + i)) min = i;
    }
    
    dout << "thValue = " << min << endl;
    return min;
#else
    return (T1 + T2) / 2;
#endif
}
/*--------------------------------------------------------------------------------------*/
BYTE* getDilation(BYTE* binaryImage, int width, int height, mask_t* dilation){
    BYTE* resultDilation = new BYTE[(height + dilation->height) * (width + dilation->width)];
    memset(resultDilation, 255, (height + dilation->height) * (width + dilation->width));

    for(int i = dilation->height / 2; i < height; i++){
        for(int j = dilation->width / 2; j < width; j++){
            for(int row = 0; row < dilation->height; row++){
                for(int column = 0; column < dilation->width; column++){
                    if((i - dilation->height / 2 + row) < 0 || (i - dilation->height / 2 + row) >= height || 
                       (j - dilation->width / 2 + column) < 0 || (j - dilation->width / 2 + column) >= width) continue;

                    if(!(*(dilation->mask + row * dilation->width + column) | *(binaryImage + \
                        (i - dilation->height / 2 + row) * width + (j - dilation->width / 2 + column)))){
                        *(resultDilation + (i + dilation->height / 2) * (width + dilation->width) + j + dilation->width / 2) = 0;
                        break;
                    }
                }
            }
        }
    }
    return resultDilation;
}
/*--------------------------------------------------------------------------------------*/
BYTE* getErosion(BYTE* binaryImage, int width, int height, mask_t* erosion){
    BYTE* resultErosion = new BYTE[(height - erosion->height) * (width - erosion->width)];
    memset(resultErosion, 255, (height - erosion->height) * (width - erosion->width));

    for(int i = erosion->height / 2; i < height; i++){
        for(int j = erosion->width / 2; j < width; j++){
            int erosionCounter = 0;
            for(int row = 0; row < erosion->height; row++){
                for(int column = 0; column < erosion->width; column++){
                    if((i - erosion->height / 2 + row) < 0 || (i - erosion->height / 2 + row) >= height || 
                       (j - erosion->width / 2 + column) < 0 || (j - erosion->width / 2 + column) >= width) {
                        erosionCounter++;
                        continue;
                    }
                    if(*(erosion->mask + row * erosion->width + column) | *(binaryImage + \
                        (i - erosion->height / 2 + row) * width + (j - erosion->width / 2 + column))){
                        break;
                    }

                    erosionCounter++;
                    if(erosionCounter == erosion->width * erosion->height){
                        *(resultErosion + (i - erosion->height / 2) * (width - erosion->width) + j + erosion->width / 2) = 0;
                    }
                }
            }
        }
    }
    return resultErosion;
}
/*--------------------------------------------------------------------------------------*/
BYTE* getOpened(BYTE* binaryImage, int widthTwo, int heightTwo, int times){
    mask_t opening(3, 3, 1);
    BYTE* hold = new BYTE[widthTwo * heightTwo];

    for(int i = 0; i < widthTwo * heightTwo; i++){
        *(hold + i) = *(binaryImage + i);
    }
    for(int i = 0; i < times; i++){
        hold = getErosion(hold, widthTwo, heightTwo, &opening);
        widthTwo -= opening.width - 1;
        heightTwo -= opening.height - 1;
    }
    for(int i = 0; i < times; i++){
        hold = getDilation(hold, widthTwo, heightTwo, &opening);
        widthTwo += opening.width - 1;
        heightTwo += opening.height - 1;
    }
    return hold;
}
/*--------------------------------------------------------------------------------------*/
BYTE* getFrame(BYTE* binaryImage, int width, int height){
    mask_t dilation(3, 3, 1);

    BYTE* rDilation = getDilation(binaryImage, width, height, &dilation);
    BYTE* result = new BYTE[width*height];

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            *(result + (i * width) + j) = (*(binaryImage + (i * width) + j) == \
                                           *(rDilation + (i + dilation.height / 2) * \
                                            (width + dilation.width) + j + dilation.width / 2)) \
                                            ? 255 : 0;
        }
    }
    return result;
}
/*--------------------------------------------------------------------------------------*/
BYTE* regionIdentification(BYTE* binaryImage, int width, int height, BYTE* returnEtiket){
    int label = 2;
    int* result = new int[width * height];

    for(int i = 0; i < width * height; i++){
        *(result + i) = *(binaryImage + i) ? -1 : 0;
    }
    /*      k
      a b c l
      d o   */

    // -1 background, 0 data, 1 unstable state
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(*(result + i * width + j) != 0) continue;

            int counter = 0;
            int hold = -3;
            int holder = -2;

            // a, b, c
            if(i - 1 >= 0){
                for(int l = 0; l < 3; l++){
                    if(j + l - 1 < width && j + l > 0){
                        hold = *(result + (i - 1) * width + j + l - 1);
                        if(hold > 1 && holder != hold){
                            holder = hold;
                            counter++;
                        }
                    }
                }
            }
            // k, l
            if(j + 2 < width){
                for(int l = 1; l < 3; l++){
                    if(i - 2 >= 0){
                        hold = *(result + (i - l) * width + (j + 2));
                        if(hold > 1 && holder != hold){
                            holder = hold;
                            counter++;
                        }
                    }
                }
            }
            // d
            if(j - 1 >= 0){
                hold = *(result + i * width + j - 1);
                if(hold > 1 && holder != hold){
                    holder = hold;
                    counter++;
                }
            }

            if(counter == 1) *(result + i * width + j) = holder;
            else if(counter > 1) *(result + i * width + j) = 1;
            else if(holder != -1) *(result + i * width + j) = label++;
        }
    }

    for(int i = 1; i < height - 1; i++){
        for(int j = 1; j < width - 1; j++){
            if( *(result + i * width + j) == 1){
                int min = label;
                int max = 0;

                for(int countI = 0; countI < 3; countI++){
                    for(int countJ = 0; countJ < 3; countJ++){
                        int holdPixel = *(result + (i + countI - 1) * width + j + countJ - 1);
                        if(max < holdPixel) max = holdPixel;
                        if(min > holdPixel && holdPixel > 1) min = holdPixel;
                    }
                }
                *(result + i * width + j) = min;
                
                for(int k = 0; k < width * height; k++){
                    if(*(result + k) == max) *(result + k) = min;
                }
            }
        }
    }

    BYTE* returnResult = new BYTE[width * height];

    //TODO: can be edit
    int k = 240 / (label - 2);
    
    returnEtiket = new BYTE[2];
    *(returnEtiket) = label - 2;
    *(returnEtiket + 1) = k;

    for(int i = 0; i < width * height; i++){
        int x = *(result + i);
        if(x == 1){
            *(returnResult + i) = 255;
        }else if(x == -1){
            *(returnResult + i) = 0;
        }else{
            *(returnResult + i) = (*(result + i) - 1) * k;
        }
    }

    dout << "label number: " << label - 2 << endl;
    return returnResult;
}
/*--------------------------------------------------------------------------------------*/
double getVariance(BYTE* binaryImage, int startX, int startY, int sizeW, int sizeH, int width){
    double sum = 0;

    for(int i = startX; i < startX + sizeH; i++){
        for(int j = startY; j < startY + sizeW; j++){
            if(*(binaryImage + i * width + j)){
                sum++;
            }
        }
    }
    double mean = sum / (double)(sizeW * sizeH);
    sum = 0;
    for(int i = startX; i < startX + sizeH; i++){
        for(int j = startY; j < startY + sizeW; j++){
            BYTE holder = *(binaryImage + i * width + j);
            if(holder){
                sum += (1 - mean) * (1 - mean);
            }
        }
    }
    double result = sum / (double)((sizeW * sizeH) - 1);
#if DEBUG > 9
    dout << "mean = " << mean << " sum = " << sum << " result = " << result << endl;
#endif    
    return result;
}
/*--------------------------------------------------------------------------------------*/
double moment(BYTE* binaryImage, int p, int q, int startX, int startY, int sizeW, int sizeH, int width){
    double result = 0;
    for(int i = startX; i < startX + sizeH; i++){
        for(int j = startY; j < startY + sizeW; j++){
            if(*(binaryImage + i * width + j) != labelHolder) continue;
            
            result += pow(i, p) * pow(j, q);
        }
    }
    return result;
}
/*--------------------------------------------------------------------------------------*/
double centralMoment(BYTE* binaryImage, int p, int q, int startX, int startY, int sizeW, int sizeH, int width){
    double iMean = moment(binaryImage, 1, 0, startX, startY, sizeW, sizeH, width);
    double jMean = moment(binaryImage, 0, 1, startX, startY, sizeW, sizeH, width);
    double total = moment(binaryImage, 0, 0, startX, startY, sizeW, sizeH, width);
    iMean /= total;
    jMean /= total;

    double result = 0;
    for(int i = startX; i < startX + sizeH; i++){
        for(int j = startY; j < startY + sizeW; j++){
            if(*(binaryImage + i * width + j) != labelHolder) continue;

            result += pow(i - iMean, p) * pow(j - jMean, q);
        }
    }
    return result;
}
/*--------------------------------------------------------------------------------------*/
double normalizedCentralMoment(BYTE* binaryImage, int p, int q, int startX, int startY, int sizeW, int sizeH, int width){
    double cMoment = centralMoment(binaryImage, p, q, startX, startY, sizeW, sizeH, width);
    double cMomentZero = centralMoment(binaryImage, 0, 0, startX, startY, sizeW, sizeH, width);
    int Y = ((p + q) / 2) + 1;

    return (cMoment / pow(cMomentZero, Y));
}
/*--------------------------------------------------------------------------------------*/
double getFi(BYTE* binaryImage, int fiNumber, int startX, int startY, int sizeW, int sizeH, int width, int label){
    double returnResult = 0;
    labelHolder = label;

    if(fiNumber == 1){
        returnResult = normalizedCentralMoment(binaryImage, 2, 0, startX, startY, sizeW, sizeH, width) 
                        + normalizedCentralMoment(binaryImage, 0, 2, startX, startY, sizeW, sizeH, width);
    } else if(fiNumber == 2){
        returnResult = pow(normalizedCentralMoment(binaryImage, 2, 0, startX, startY, sizeW, sizeH, width) 
                        - normalizedCentralMoment(binaryImage, 0, 2, startX, startY, sizeW, sizeH, width), 2)
                        + (normalizedCentralMoment(binaryImage, 1, 1, startX, startY, sizeW, sizeH, width) * 4);
    } else if(fiNumber == 3){
        returnResult = pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            - (3 * normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width)), 2)
                        + pow((3 * normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width))
                                - normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2);
    } else if(fiNumber == 4){
        returnResult = pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width), 2)
                        + pow(normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2);
    } else if(fiNumber == 5){
        returnResult = ((normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            - (3 * normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width)))
                        * (normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width))
                        * ( (pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width), 2)) 
                            - ( 3 * pow(normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                                    + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2)) ))

                        + ((3 * normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                            - normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width))
                        * (normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width))
                        * ( (3 * pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                                + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width), 2)) 
                            - pow(normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                                + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2) ));
    } else if(fiNumber == 6){
        returnResult = ( (normalizedCentralMoment(binaryImage, 2, 0, startX, startY, sizeW, sizeH, width) 
                            - normalizedCentralMoment(binaryImage, 0, 2, startX, startY, sizeW, sizeH, width))
                        * (pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width), 2)
                         - pow(normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2)) )

                        + 4 * normalizedCentralMoment(binaryImage, 1, 1, startX, startY, sizeW, sizeH, width)
                            * (normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width) 
                                + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width))
                            * (normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                                + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width));
    } else if(fiNumber == 7){
        returnResult = ((3 * normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width) 
                            - normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width))
                        * (normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width))
                        * ( pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                                + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width), 2)
                            - (3 * pow(normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                                        + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2))))

                        - (normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width) 
                            - 3 * normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width)
                        * (normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                            + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width))
                        * (3 * pow(normalizedCentralMoment(binaryImage, 3, 0, startX, startY, sizeW, sizeH, width)
                                + normalizedCentralMoment(binaryImage, 1, 2, startX, startY, sizeW, sizeH, width), 2)
                            - pow(normalizedCentralMoment(binaryImage, 2, 1, startX, startY, sizeW, sizeH, width)
                                    + normalizedCentralMoment(binaryImage, 0, 3, startX, startY, sizeW, sizeH, width), 2)));
    } else{
        cout << "Unknown Fi!" << endl;
    }
    return returnResult;
}
/*--------------------------------------------------------------------------------------*/
void getPoints(BYTE* binaryImage, int* startX, int* startY, int* sizeH, int* sizeW, BYTE label, int width, int height){
    *startX = height; *startY = width; *sizeH = 0; *sizeW = 0;

    int iMax = 0, jMax = 0;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(*(binaryImage + i * width + j) == label){
                if(i < *startX) *startX = i;
                if(j < *startY) *startY = j;

                if(iMax < i) iMax = i;
                if(jMax < j) jMax = j;
            }
        }
    }

    *sizeH = iMax - *startX + 1;
    *sizeW = jMax - *startY + 1;
}
/*--------------------------------------------------------------------------------------*/
int* thresHoldWithK(const BYTE* const ramIntensity, int k, int width, int height){
    int* tHold = new int[ 256 ];
    int i;

    memset(tHold, 0, 256 * sizeof(int));
    int* allT = new int[k];

    for(i = 0; i < width * height; i++){
            tHold[(int)*(ramIntensity + i)]++;
    }

    int done = 0;
    int gap = 250 / (k - 1);

    for(i = 0; i < k; i++){
        allT[i] = i * gap;
    }

    while(!done){
        DWORD sumsT[k] = {0};
        float sumsTi[k] = {0};

        for(i = 0; i < 256; i++){
            int min = fabs(allT[0] - i);
            int j, minJ = 0;
            for(j = 0; j < k; j++){
                if(fabs(allT[j] - i) < min){
                    min = fabs(allT[j] - i);
                    minJ = j;
                }
            }
            sumsT[minJ] += i * (*(tHold + i));
            sumsTi[minJ] += *(tHold + i);
        }

        int allTu[k];
        float epsilon = 1;
        int doneCounter = 0;

        for(i = 0; i < k; i++){
            if(sumsTi[i] == 0){
                sumsTi[i] = 1;
            }
            allTu[i] = sumsT[i] / sumsTi[i];
            if(fabs(allT[i] - allTu[i]) < epsilon){
                doneCounter++;
            }else{
                allT[i] = allTu[i];
            }
        }

        if(doneCounter == k){
            for(i = 0; i < k; i++){
                dout << "T" << i + 1 << " = " << allT[i] << endl;
            }
            done = 1;
        }
    }

    return allT;
}
/*--------------------------------------------------------------------------------------*/
int* getAllT(int* tHold, int k, int width, int height){
    int* allT = new int[k];
    int i;

    int done = 0;
    int gap = 250 / (k - 1);

    for(i = 0; i < k; i++){
        allT[i] = i * gap;
    }
    for(i = 0; i < k; i++){
        dout << allT[i] << " ";
    }
    dout << endl;

    while(!done){
        DWORD sumsT[k] = {0};
        float sumsTi[k] = {0};

        for(i = 0; i < 256; i++){
            int min = fabs(allT[0] - i);
            int j, minJ = 0;
            for(j = 0; j < k; j++){
                if(fabs(allT[j] - i) < min){
                    min = fabs(allT[j] - i);
                    minJ = j;
                }
            }
            sumsT[minJ] += i * (*(tHold + i));
            sumsTi[minJ] += *(tHold + i);
        }

        int allTu[k];
        float epsilon = 1;
        int doneCounter = 0;

        for(i = 0; i < k; i++){
            if(sumsTi[i] == 0){
                sumsTi[i] = 1;
            }
            allTu[i] = sumsT[i] / sumsTi[i];
            if(fabs(allT[i] - allTu[i]) < epsilon){
                doneCounter++;
            }else{
                allT[i] = allTu[i];
            }
        }

        if(doneCounter == k){
            for(i = 0; i < k; i++){
                dout << "T" << i + 1 << " = " << allT[i] << endl;
            }
            done = 1;
        }
    }

    return allT;
}
/*--------------------------------------------------------------------------------------*/
void printStructers(BITMAPFILEHEADER h, BITMAPINFOHEADER i){
    cout << "-------------------------------" << endl;
    cout << "bfType______0x" << hex << h.bfType << endl;
    cout << "bfSize______0x" << hex << h.bfSize << endl;
    cout << "bfReserved1_0x" << hex << h.bfReserved1 << endl;
    cout << "bfReserved2_0x" << hex << h.bfReserved2 << endl;
    cout << "bfOffBits___0x" << hex << h.bfOffBits << endl;
    
    cout << "biSize__________0x" << hex << i.biSize << endl;
    cout << "biWidth_________0x" << hex << i.biWidth << endl;
    cout << "biHeight________0x" << hex << i.biHeight << endl;
    cout << "biPlanes________0x" << hex << i.biPlanes << endl;
    cout << "biBitCount______0x" << hex << i.biBitCount << endl;
    cout << "biCompression___0x" << hex << i.biCompression << endl;
    cout << "biSizeImage_____0x" << hex << i.biSizeImage << endl;
    cout << "biXPelsPerMeter_0x" << hex << i.biXPelsPerMeter << endl;
    cout << "biYPelsPerMeter_0x" << hex << i.biYPelsPerMeter << endl;
    cout << "biClrUsed_______0x" << hex << i.biClrUsed << endl;
    cout << "biClrImportant__0x" << hex << i.biClrImportant << endl;
    cout << dec << "-------------------------------" << endl;
    return;
}
/*--------------------------------------------------------------------------------------*/