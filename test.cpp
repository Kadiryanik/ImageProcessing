#include <iostream>
#include <fstream>
#include <cstring> //for memset
#include <ctime> //for time
#include <iomanip> //for setw
#include <string>

#include "bmp.h"

using namespace std;

#define DEBUG 0
#define PRINTSIZE 6

#if DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif
//çerçeveleme
//nxn resmi n/2 x n/2 ye çevirme n/4 e aynı şekilde

#define INTEGRAL_TEST 0
#define HAAR_F_TEST 0
#define GET_DATA 0

#define NOISE_MASK_TEST 0
#define THRESHOLD_TEST 1
#define DILATION_TEST 0
#define EROSION_TEST 0
#define REGION_TEST 0
#define OPEN_CLOSE_FRAME_TEST 0

#define THRESHOLD_K_TEST 0
#define RGB_TEST 0
#define DRAW_TESTS 0

int main(int argc, char *argv[])
{
#if !GET_DATA
    int height, width;
    int holdHeight, holdWidth;
    long size;
    ifstream file;
    int i, j, k;
    int start_c, stop_c;
    const char* filePath = "images/result.bmp";

    file.open("images/backup/test.bmp", ios::in | ios::binary);

    BYTE* buffer = loadBMP(&height, &width, &size, file);
    holdWidth = width;
    holdHeight = height;

    if(buffer == NULL){
        cout << "Error: buffer null, returned in LoadBMP()!" << endl;
        return 1;
    }
    
    BYTE* ramIntensity = convertBMPToIntensity(buffer, width, height);

    if(ramIntensity == NULL){
        cout << "Error: ramIntensity null, returned in convertBMPToIntensity()!" << endl;
        return 1;
    }
#endif /* !GET_DATA */

#if INTEGRAL_TEST
    start_c = clock();
    DWORD* integralImage = getIntegralImage(ramIntensity, width, height);
    stop_c = clock();

#if DEBUG
    for(i = 0; i < PRINTSIZE; i++){
        for(j = 0; j < PRINTSIZE; j++){
            dout << setw (5) << (int)*(ramIntensity + i * width + j) << " ";
        }
        dout << endl;
    }
    dout << endl << endl;

    for(i = 0; i < PRINTSIZE; i++){
        for(j = 0; j < PRINTSIZE; j++){
            dout << setw (5) << *(integralImage + i * (width + 1) + j) << " ";
        }
        dout << endl;
    }
#endif /* DEBUG */
    //dout << "Total = " << getTotal(integralImage, 0, 0, 1, 1, width, height) << endl;

    dout << "Calculate integralImage time: " << (stop_c - start_c) / double(CLOCKS_PER_SEC) << endl << endl;

#if HAAR_F_TEST
    for(i = 1; i < 6; i++){
        start_c = clock();
        int* result = haarFeature(i, 1, 1, integralImage, width, height, 1);
        stop_c = clock();

        dout << i << " - Calculate haarFeature time: " << (stop_c - start_c) / double(CLOCKS_PER_SEC) << endl << endl;        
    }
#endif /* HAAR_F_TEST */
    delete[] integralImage;
#endif /* INTEGRAL_TEST */

#if NOISE_MASK_TEST
    int widthN = 220, heightN = 220;
    BYTE* noised = new BYTE[ widthN * heightN ];
    
    const char* filePathFace = "images/lenaFace.bmp";
    const char* filePathMean = "images/mean.bmp";
    const char* filePathMedian = "images/median.bmp";
    const char* filePathGaussian = "images/gaussian.bmp";
#if 1
    for(i = 0; i < heightN; i++){
        for(j = 0; j < widthN; j++){
            *(noised + i * widthN + j) = *(ramIntensity + ((190 + i) * width) + j + 180);
        }
    }
#else
    for(i = 0; i < heightN; i++){
        for(j = 0; j < widthN; j++){
            *(noised + i * widthN + j) = *(ramIntensity + ((30 + i) * width) + j + 20);
        }
    }
#endif
    saveBMP(filePathFace, heightN, widthN, convertIntensityToBMP(noised, widthN, heightN, &size));
    saveBMP(filePathMean, heightN, widthN, convertIntensityToBMP(blurMean(noised, widthN, heightN), widthN, heightN, &size));
    saveBMP(filePathGaussian, heightN, widthN, convertIntensityToBMP(blurGaussian(noised, widthN, heightN), widthN, heightN, &size));
    saveBMP(filePathMedian, heightN, widthN, convertIntensityToBMP(blurMedian(noised, widthN, heightN), widthN, heightN, &size));

    delete [] noised;
#endif /* NOISE_MASK_TEST */

#if THRESHOLD_TEST
    int heightTwo, widthTwo;
    long sizeTwo;
    ifstream fileTwo;

    fileTwo.open("images/backup/test5.bmp", ios::in | ios::binary);

    BYTE* bufferTwo = loadBMP(&heightTwo, &widthTwo, &sizeTwo, fileTwo);
    fileTwo.close();

    if(bufferTwo == NULL){
        cout << "Error: bufferTwo null, returned in LoadBMP()!" << endl;
        return 1;
    }
    
    BYTE* ramIntensityTwo = convertBMPToIntensity(bufferTwo, widthTwo, heightTwo);

    start_c = clock();
    int tHold = thresHold(ramIntensityTwo, widthTwo, heightTwo);

    const char* filePathBinary = "images/binary.bmp";

    BYTE* binaryImage = new BYTE[widthTwo * heightTwo];

    for(i = 0; i < widthTwo*heightTwo; i++){
        *(binaryImage + i) = *(ramIntensityTwo + i) > tHold ? 255 : 0; 
    }
    stop_c = clock();
    saveBMP(filePathBinary, heightTwo, widthTwo, convertIntensityToBMP(binaryImage, widthTwo, heightTwo, &sizeTwo));

    dout << "Calculate thresHold time: " << (stop_c - start_c) / double(CLOCKS_PER_SEC) << endl << endl;

#if DILATION_TEST
    const char* filePathDilation = "images/dilation.bmp";
    mask_t dilation(5, 5, 1);

    saveBMP(filePathDilation, heightTwo + dilation.height, widthTwo + dilation.width, 
                convertIntensityToBMP(getDilation(binaryImage, widthTwo, heightTwo, &dilation), widthTwo + dilation.width, heightTwo + dilation.height, &sizeTwo));
    dout << "DILATION_TEST succed!" << endl;
#endif /* DILATION_TEST */

#if EROSION_TEST
    const char* filePathErosion = "images/erosion.bmp";
    mask_t erosion(5, 5, 1);

    saveBMP(filePathErosion, heightTwo - erosion.height, widthTwo - erosion.width, 
        convertIntensityToBMP(getErosion(binaryImage, widthTwo, heightTwo, &erosion), widthTwo - erosion.width, heightTwo - erosion.height, &sizeTwo));
    dout << "EROSION_TEST succed!" << endl;
#endif /* EROSION_TEST */

#if OPEN_CLOSE_FRAME_TEST
    const char* filePathFrame = "images/frame.bmp";

    saveBMP(filePathFrame, heightTwo, widthTwo, convertIntensityToBMP(getFrame(binaryImage, widthTwo, heightTwo), widthTwo, heightTwo, &sizeTwo));
    dout << "OPEN_CLOSE_FRAME_TEST succed!" << endl;
#endif /* OPEN_CLOSE_FRAME_TEST */

#if REGION_TEST
    saveBMP("images/regionResult.bmp", heightTwo - erosion.height, widthTwo - erosion.width, 
        convertIntensityToBMP(regionIdentification(getErosion(binaryImage, widthTwo, heightTwo, &erosion), widthTwo - erosion.width, heightTwo - erosion.height), widthTwo - erosion.width, heightTwo - erosion.height, &size));
    dout << "REGION_TEST succed!" << endl;
#endif /* REGION_TEST */
    delete[] binaryImage;

    dout << "THRESHOLD_TEST succed!" << endl;
#endif /* THRESHOLD_TEST */

#if THRESHOLD_K_TEST
    cout << "THRESHOLD_K_TEST k: ";
    cin >> k;
    
    const char* filePathClusted = "images/clusted.bmp";
    const char* filePathClustedC = "images/clustedColorful.bmp";
    
    int* allT = thresHoldWithK(ramIntensity, k, width, height);

    for(i = 0; i < k - 1; i++){
        for(i = 0; i < k - 1; i++){
            if(allT[i] > allT[i + 1]){
                int temp = allT[i];
                allT[i] = allT[i + 1];
                allT[i + 1] = temp;
            }
        }
    }
    int gap = 255 / k;
    BYTE* clustedImage = new BYTE[width * height];

    for(i = 0; i < width*height; i++){
        for(j = 0; j < k - 1; j++){
            if(*(ramIntensity + i) <= allT[0]){
                *(clustedImage + i) =  0;
            } else if(*(ramIntensity + i) >= allT[k - 1]){
                *(clustedImage + i) =  255;
            } else{
                if(*(ramIntensity + i) >= allT[j] && *(ramIntensity + i) <= allT[j + 1]){
                   *(clustedImage + i) = j * (gap);
                }
            }
        }
    }
    saveBMP(filePathClustedC, height, width, convertIntensityToColoredBMP(clustedImage, allT, k, width, height, &size));
    saveBMP(filePathClusted, height, width, convertIntensityToBMP(clustedImage, width, height, &size));

    delete[] clustedImage;

    dout << "THRESHOLD_K_TEST succed!" << endl;
#endif /* THRESHOLD_K_TEST */

#if RGB_TEST
    const char* filePathRGB = "images/rgb.bmp";
    int tholdBlue[256] = {0};
    int tholdGreen[256] = {0};
    int tholdRed[256] = {0};

    cout << "rgb k: ";
    cin >> k;

    for(i = 0; i < holdHeight * holdWidth * 3; i += 3){
        tholdBlue[*(buffer + i)]++;       //b
        tholdGreen[*(buffer + i + 1)]++;  //g
        tholdRed[*(buffer + i + 2)]++;    //r
    }

    int* allTblue = getAllT(tholdBlue, k, width, height);
    int* allTgreen = getAllT(tholdGreen, k, width, height);
    int* allTred = getAllT(tholdRed, k, width, height);

    for(i = 0; i < k - 1; i++){
        for(i = 0; i < k - 1; i++){
            if(allTblue[i] > allTblue[i + 1]){
                int temp = allTblue[i];
                allTblue[i] = allTblue[i + 1];
                allTblue[i + 1] = temp;
            }
            if(allTgreen[i] > allTgreen[i + 1]){
                int temp = allTgreen[i];
                allTgreen[i] = allTgreen[i + 1];
                allTgreen[i + 1] = temp;
            }
            if(allTred[i] > allTred[i + 1]){
                int temp = allTred[i];
                allTred[i] = allTred[i + 1];
                allTred[i + 1] = temp;
            }
        }
    }
#if 1
    for(i = 0; i < holdWidth*holdHeight*3; i+=3){
        for(j = 0; j < k - 1; j++){
            if(*(buffer + i) <= allTblue[0]){
                *(buffer + i) =  allTblue[0] / 2;
            } else if(*(buffer + i) >= allTblue[k - 1]){
                *(buffer + i) =  (allTblue[k - 1] + 255) / 2;
            } else{
                if(*(buffer + i) >= allTblue[j] && *(buffer + i) <= allTblue[j + 1]){
                   *(buffer + i) = (allTblue[j] + allTblue[j + 1]) / 2;
                }
            }

            if(*(buffer + i + 1) <= allTgreen[0]){
                *(buffer + i + 1) =  allTgreen[0] / 2;
            } else if(*(buffer + i + 1) >= allTgreen[k - 1]){
                *(buffer + i) =  (allTgreen[k - 1] + 255) / 2;
            } else{
                if(*(buffer + i + 1) >= allTgreen[j] && *(buffer + i + 1) <= allTgreen[j + 1]){
                   *(buffer + i + 1) = (allTgreen[j] + allTgreen[j + 1]) / 2;
                }
            }

            if(*(buffer + i + 2) <= allTred[0]){
                *(buffer + i + 2) =  allTred[0] / 2;;
            } else if(*(buffer + i + 2) >= allTred[k - 1]){
                *(buffer + i + 2) =  (allTred[k - 1] + 255) / 2;
            } else{
                if(*(buffer + i + 2) >= allTred[j] && *(buffer + i + 2) <= allTred[j + 1]){
                   *(buffer + i + 2) = (allTred[j] + allTred[j + 1]) / 2;
                }
            }
        }
    }
#else
    RGB_data colors[3];
    colors[0].b = 255;
    colors[0].g = 0;
    colors[0].r = 0;
    
    colors[1].b = 0;
    colors[1].g = 255;
    colors[1].r = 0;

    colors[2].b = 0;
    colors[2].g = 0;
    colors[2].r = 255;

    for(i = 0; i < holdWidth*holdHeight*3; i+=3){
        for(j = 0; j < k - 1; j++){
            if(*(buffer + i) <= allTblue[0]){
                *(buffer + i) =  colors[0].b;
            } else if(*(buffer + i) >= allTblue[k - 1]){
                *(buffer + i) =  colors[1].b;
            } else{
                if(*(buffer + i) >= allTblue[j] && *(buffer + i) <= allTblue[j + 1]){
                   *(buffer + i) = colors[2].b;
                }
            }

            if(*(buffer + i + 1) <= allTgreen[0]){
                *(buffer + i + 1) =  colors[0].g;
            } else if(*(buffer + i + 1) >= allTgreen[k - 1]){
                *(buffer + i) =  colors[1].g;
            } else{
                if(*(buffer + i + 1) >= allTgreen[j] && *(buffer + i + 1) <= allTgreen[j + 1]){
                   *(buffer + i + 1) = colors[2].g;
                }
            }

            if(*(buffer + i + 2) <= allTred[0]){
                *(buffer + i + 2) = colors[0].r;
            } else if(*(buffer + i + 2) >= allTred[k - 1]){
                *(buffer + i + 2) =  colors[1].r;
            } else{
                if(*(buffer + i + 2) >= allTred[j] && *(buffer + i + 2) <= allTred[j + 1]){
                   *(buffer + i + 2) = colors[2].r;
                }
            }
        }
    }
#endif

    saveBMP(filePathRGB, holdHeight, holdWidth, buffer);
#endif /* RGB_TEST */

#if GET_DATA
    int N = 1, M = 1;
    int* haarVectorFace;
    int* haarVectorNonFace;

    haarVectorFace = new int[N];
    haarVectorNonFace = new int[M];

    haarModels_t hModels[5];

    hModels[0].height = 1;
    hModels[0].width = 2;
    hModels[1].height = 2;
    hModels[1].width = 1;
    hModels[2].height = 1;
    hModels[2].width = 3;
    hModels[3].height = 3;
    hModels[3].width = 1;
    hModels[4].height = 2;
    hModels[4].width = 2;

    int width = 19, height = 19;//resmin boyutları

    DWORD* faceIntegralImages = new DWORD[(width + 1) * (height + 1) * N];
    DWORD* nonFaceIntegralImages = new DWORD[(width + 1) * (height + 1) * M];

    BYTE* faces, *nonFaces;
    faces = new BYTE[width * height * N];
    nonFaces = new BYTE[width * height * M];

    int start_c = clock();

    int i;
    for(i = 0; i < N; i++){
        //string filename = ("images/face_database/TrainingNonFaces/" + i + ".pgm");
        ifstream file("images/face_database/TrainingFaces/1.pgm", ios::in | ios::binary);
        file.seekg(13, ios::beg);
        file.read((char *)(faces + i * (width*height)), width*height);
        file.close();
    }
    faceIntegralImages = getAllIntegralImages(faces, width, height, N);


    for(i = 0; i < M; i++){
        //const char* filename = ("images/face_database/TrainingNonFaces/" + i + ".pgm");
        ifstream file("images/face_database/TrainingNonFaces/1.pgm", ios::in | ios::binary);
        file.seekg(13, ios::beg);
        file.read((char *)(nonFaces + i * (width*height)), width*height);
        file.close();
    }
    nonFaceIntegralImages = getAllIntegralImages(nonFaces, width, height, M);
    int counter=0;
    int hModelCounter, hModelHeight, hModelWidth;
    //TODO: hModelCounter üst sınır 6 olcak
    
    for(hModelCounter = 1; hModelCounter < 6; hModelCounter++){
        for(hModelHeight = 1; hModelHeight < height; hModelHeight++){
            for(hModelWidth = 1; hModelWidth < width; hModelWidth++){
                int* result = haarFeature(hModelCounter, hModelHeight, hModelWidth, faceIntegralImages, width, height, N);
                if(result == NULL) break;
                int* resultNonFace = haarFeature(hModelCounter, hModelHeight, hModelWidth, nonFaceIntegralImages, width, height, M);
                if(resultNonFace == NULL) break;
                //dout << "height: " << hModelHeight << ", width: " << hModelWidth << " succeed!" << endl;

                int row, column, location = 0;

                int rowUpLimit = height - (hModels[hModelCounter - 1].height * hModelHeight);
                int columnUpLimit = width - (hModels[hModelCounter - 1].width * hModelWidth);
                for(row = 0; row < rowUpLimit; row++){
                    for(column = 0; column < columnUpLimit; column++){
                        for(i = 0; i < N; i++){
                            haarVectorFace[i] = *(result + (i * rowUpLimit * columnUpLimit) + location);
                        }
                        for(i = 0; i < M; i++){
                            haarVectorNonFace[i] = *(resultNonFace + (i * rowUpLimit * columnUpLimit) + location);
                        }
                        //TODO: burdan elde edilenler fonksyonda dosyaya yazılabilir
                        //calculate(haarVectorFace, haarVectorNonFace, N, M);

                        location++;
                        counter++;
                    }
                }
                //dout << "location: " << location << endl;
                location = 0;
                delete[] result;
                delete[] resultNonFace;
            }
        }
    }
    dout << "counter = " << counter << ", time = " << (clock() - start_c) / double(CLOCKS_PER_SEC) << endl;

#endif /* GET_DATA */

#if DRAW_TESTS
    int centerX = 300;
    int centerY = 290;

    drawPlus(centerX, centerY, width, ramIntensity);

    /* Test drawCircle */
    if(!drawCircle(centerX, centerY, 110, ramIntensity, width, height)){
        cout << "Error: drawCircle (big) failed!" << endl;
    }else{
        dout << "drawCircle (big) succed!" << endl;
    }

    /* Test drawCircle */
    if(!drawCircle(centerX, centerY, 50, ramIntensity, width, height)){
        cout << "Error: drawCircle (small) failed!" << endl;
    }else{
        dout << "drawCircle (small) succed!" << endl;
    }

    /* Test drawRect */
    if(!drawRect(centerX, centerY, 110, 110, ramIntensity, width, height)){
        cout << "Error: drawRect failed!" << endl;
    }else{
        dout << "drawRect succed!" << endl;
    }

    /* Test drawElips */
    if(!drawElips(centerX, centerY, 110, 50, ramIntensity, width, height)){
        cout << "Error: drawElips failed!" << endl;
    }else{
        dout << "drawElips succed" << endl;
    }

#if DEBUG > 10
    const char* filePathDebug = "images/intensity.bmp";
    saveBMP(filePathDebug, height, width, ramIntensity);
#endif /* DEBUG > 1 */

    saveBMP(filePath, height, width, convertIntensityToBMP(ramIntensity, width, height, &size));
#endif /* DRAW_TESTS */

#if !GET_DATA
    delete[] buffer;
    delete[] ramIntensity;

    file.close();
#endif /* !GET_DATA */

    return 0;
}