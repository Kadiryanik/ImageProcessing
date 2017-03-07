#include <iostream>
#include <fstream>
#include <math.h>   //for fabs
#include <cstring>  //for memset
#include <iomanip> //for setw

#include "bmp.h"

using namespace std;

#define DEBUG 1
#define LOAD_ANYWAY 0
#define PRINTSIZE 6

#define PI 3.141592

#if DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif

/*--------------------------------------------------------------------------------------*/
BYTE* loadBMP(int* width, int* height, long* size, ifstream &file){
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
int saveBMP(const char *filename, int width, int height, BYTE *data){
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
}//ConvetBMPToIntensity
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
} //ConvertIntensityToBMP
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
DWORD* getIntegralImage(BYTE* ramIntensity, int width, int height){
    DWORD* integralImage = new DWORD[(width + 1) * (height + 1)];
    
    int i,j;
    memset(integralImage, 0, sizeof (integralImage));
 
    for(i = 0; i < height; i++){
        for(j = 0; j < width; j++){
            *(integralImage + ((width + 1) * (i + 1)) + j + 1) = *(integralImage + ((width + 1) * (i + 1)) + j)
                                                                + *(integralImage + ((width + 1) * i) + j + 1) 
                                                                - *(integralImage + ((width + 1) * i) + j)
                                                                + *(ramIntensity + (width * i) + j);
        }
    }
   
    return integralImage;
}
/*--------------------------------------------------------------------------------------*/
int getTotal(const DWORD* const integralImage, int x, int y, int lastX, int lastY, int width, int height){
    int sum = 0;

    if(x >= height || y >= width || lastX >= height || lastY >= width
        || x < 0 || y < 0 || lastX < 0 || lastY < 0){
        cout << "getTotal(): Out of memory area" << endl;
        return -1;
    }

    lastX += 1;
    lastY += 1;

    sum = *(integralImage + ((width + 1) * x) + y) + *(integralImage + ((width + 1) * lastX) + lastY)
        - *(integralImage + ((width + 1) * x) + lastY) - *(integralImage + ((width + 1) * lastX) + y);

    return sum;
}
/*--------------------------------------------------------------------------------------*/
//hModel, size, (i * (width - w - 1)) + j, sum, min, max
int* haarCascade(int hModel, int size, const DWORD* const integralImage, int width, int height){
    if(hModel == HAAR_MODEL_1){
        int w = 2 * size;  // width
        int h = 1 * size;  // height
        
        h--; w--;

        int* result = new int[ (width - w) * (height - h) ];
        
        int sum, i, j;
        
        for(i = 0; i < height - h; i++){
            for (j = 0; j < width - w; j++){
                sum = 0;

                /* leftArea */
                sum = getTotal(integralImage, i, j, i + h, (j + (w / 2)), width, height);
                /* rightArea */
                sum -= getTotal(integralImage, i, (j + (w / 2)) + 1, i + h, j + w, width, height);

                *(result + ((width - w) * i) + j) = sum;
            }
        }
#if DEBUG > 1
        for(i = 0; i < PRINTSIZE; i++){
            for(j = 0; j < PRINTSIZE; j++){
                dout << setw (5) << *(result + i * (width - w) + j) << " ";
            }
            dout << endl;
        }
#endif /* DEBUG > 1 */
        return result;
    } else if(hModel == HAAR_MODEL_2){
        int w = 1 * size;  // width
        int h = 2 * size;  // height
        h--; w--;

        int* result = new int[ (width - w) * (height - h) ];
        int sum, i, j;
        
        for(i = 0; i < height - h; i++){
            for (j = 0; j < width - w; j++){
                sum = 0;

                /* leftArea */
                sum = getTotal(integralImage, i + (h / 2) + 1, j, i + h, j + w, width, height);
                /* rightArea */
                sum -= getTotal(integralImage, i, j, i + (h / 2), j + w, width, height);

                *(result + ((width - w) * i) + j) = sum;
            }
        }
#if DEBUG > 1
        for(i = 0; i < PRINTSIZE; i++){
            for(j = 0; j < PRINTSIZE; j++){
                dout << setw (5) << *(result + i * (width - w) + j) << " ";
            }
            dout << endl;
        }
#endif /* DEBUG > 1 */
        return result;
    } else if(hModel == HAAR_MODEL_3){
        int w = 3 * size;  // width
        int h = 1 * size;  // height
        h--; w--;

        int* result = new int[ (width - w) * (height - h) ];
        int sum, i, j;
        
        for(i = 0; i < height - h; i++){
            for (j = 0; j < width - w; j++){
                sum = 0;

                /* leftArea */
                sum = getTotal(integralImage, i, j, i + h, j + (w / 3), width, height);
                /* rightArea */
                sum += getTotal(integralImage, i, j + (2 * ((w + 1) / 3)), i + h, j + w, width, height);
                /* midArea */
                sum -= getTotal(integralImage, i, j + ((w + 1) / 3), i + h, j + (2 * ((w + 1) / 3)) - 1, width, height);

                *(result + ((width - w) * i) + j) = sum;
            }
        }
#if DEBUG > 1
        for(i = 0; i < PRINTSIZE; i++){
            for(j = 0; j < PRINTSIZE; j++){
                dout << setw (5) << *(result + i * (width - w) + j) << " ";
            }
            dout << endl;
        }
#endif /* DEBUG > 1 */
        return result;
    } else if(hModel == HAAR_MODEL_4){
        int w = 1 * size;  // width
        int h = 3 * size;  // height
        h--; w--;

        int* result = new int[ (width - w) * (height - h) ];
        int sum, i, j;
        
        for(i = 0; i < height - h; i++){
            for (j = 0; j < width - w; j++){
                sum = 0;

                /* leftArea */
                sum = getTotal(integralImage, i, j, i + (h / 3), j + w, width, height);
                /* rightArea */
                sum += getTotal(integralImage, i + (2 * ((h + 1) / 3)), j, i + h, j + w, width, height);
                /* midArea */
                sum -= getTotal(integralImage, i + ((h + 1) / 3), j, i + (2 * ((h + 1) / 3)) - 1, j + w, width, height);

                *(result + ((width - w) * i) + j) = sum;
            }
        }
#if DEBUG > 1
        for(i = 0; i < PRINTSIZE; i++){
            for(j = 0; j < PRINTSIZE; j++){
                dout << setw (5) << *(result + i * (width - w) + j) << " ";
            }
            dout << endl;
        }
#endif /* DEBUG > 1 */
        return result;
    } else if(hModel == HAAR_MODEL_5){
        int w = 2 * size;  // width
        int h = 2 * size;  // height
        h--; w--;

        int* result = new int[ (width - w) * (height - h) ];
        int sum, i, j;
        
        for(i = 0; i < height - h; i++){
            for (j = 0; j < width - w; j++){
                sum = 0;

                /* left + */
                sum = getTotal(integralImage, i, j, i + (h / 2), j + (w / 2), width, height);
                /* right + */
                sum += getTotal(integralImage, i + ((h + 1) / 2), j + ((w + 1) / 2), i + h, j + w, width, height);
                /* left - */
                sum -= getTotal(integralImage, i + ((h + 1) / 2), j, i + h, j + (w / 2), width, height);
                /* right - */
                sum -= getTotal(integralImage, i, j + ((w + 1) / 2), i + (h / 2), j + w, width, height);
                
                *(result + ((width - w) * i) + j) = sum;
            }
        }
#if DEBUG > 1
        for(i = 0; i < PRINTSIZE; i++){
            for(j = 0; j < PRINTSIZE; j++){
                dout << setw (5) << *(result + i * (width - w) + j) << " ";
            }
            dout << endl;
        }
#endif /* DEBUG > 1 */
        return result;
    } else {
        cout << "Undefined HAAR_MODEL" << endl;
        return NULL;
    }
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