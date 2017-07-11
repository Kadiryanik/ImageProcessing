#include <iostream>
#include <fstream>
#include <cstring>      //for memset
#include <ctime>        //for time
#include <iomanip>      //for setw
#include <string>
#include <math.h>       //for fabs

#include "bmp.h"

using namespace std;

#define DEBUG 1
#define PRINTSIZE 6

#if DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif

#define NOISE_MASK_TEST 0
#define THRESHOLD_TEST 1
/* Depend: THRESHOLD_TEST */
#define DILATION_TEST 0
#define EROSION_TEST 0
#define OPEN_CLOSE_FRAME_TEST 0
#define REGION_TEST 0
/* Depend: REGION_TEST */
#define WITH_EROSION 0
/* REGION_TEST */
#define VARIANCE_TEST 0
#define MOMENT_TEST 0
/* Depend: REGION_TEST && MOMENT_TEST */
#define LEARN_TEST 0
#define LEARNED_TEST 0
/* REGION_TEST && MOMENT_TEST */
/* THRESHOLD_TEST */
#define THRESHOLD_K_TEST 0
#define DRAW_TESTS 0

int main(int argc, char *argv[])
{
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

#if NOISE_MASK_TEST
    int widthN = 220, heightN = 220;
    BYTE* noised = new BYTE[ widthN * heightN ];
    
    const char* filePathFace = "images/lenaFace.bmp";
    const char* filePathMean = "images/mean.bmp";
    const char* filePathMedian = "images/median.bmp";
    const char* filePathGaussian = "images/gaussian.bmp";

    for(i = 0; i < heightN; i++){
        for(j = 0; j < widthN; j++){
            *(noised + i * widthN + j) = *(ramIntensity + ((190 + i) * width) + j + 180);
        }
    }

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

    fileTwo.open("images/backup/test.bmp", ios::in | ios::binary);

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
    mask_t dilation(3, 3, 1);
    //dout << heightTwo << " " << widthTwo << endl;
    saveBMP(filePathDilation, heightTwo + dilation.height, widthTwo + dilation.width, 
                convertIntensityToBMP(getDilation(binaryImage, widthTwo, heightTwo, &dilation), widthTwo + dilation.width, heightTwo + dilation.height, &sizeTwo));
    dout << "DILATION_TEST succed!" << endl;
#endif /* DILATION_TEST */

#if EROSION_TEST
    const char* filePathErosion = "images/erosion.bmp";
    mask_t erosion(3, 3, 1);
    
    saveBMP(filePathErosion, heightTwo - erosion.height, widthTwo - erosion.width, 
        convertIntensityToBMP(getErosion(binaryImage, widthTwo, heightTwo, &erosion), widthTwo - erosion.width, heightTwo - erosion.height, &sizeTwo));
    dout << "EROSION_TEST succed!" << endl;
#endif /* EROSION_TEST */

#if OPEN_CLOSE_FRAME_TEST
    const char* filePathOpened = "images/opened.bmp";
    saveBMP(filePathOpened, heightTwo, widthTwo, convertIntensityToBMP(getOpened(binaryImage, widthTwo, heightTwo, 2), widthTwo, heightTwo, &sizeTwo));

    const char* filePathFrame = "images/frame.bmp";
    saveBMP(filePathFrame, heightTwo, widthTwo, convertIntensityToBMP(getFrame(binaryImage, widthTwo, heightTwo), widthTwo, heightTwo, &sizeTwo));

    dout << "OPEN_CLOSE_FRAME_TEST succed!" << endl;
#endif /* OPEN_CLOSE_FRAME_TEST */

#if REGION_TEST
    BYTE* etiketler;
#if WITH_EROSION
    int regionWidth = widthTwo - erosion.width;
    int regionHeight = heightTwo - erosion.height;
    BYTE* regionPtr = regionIdentification(getErosion(binaryImage, widthTwo, heightTwo, &erosion), regionWidth, regionHeight, etiketler);

    saveBMP("images/regionResult.bmp", regionHeight, regionWidth, 
        convertIntensityToBMP(regionPtr, regionWidth, regionHeight, &size));
    dout << "REGION_TEST succed!" << endl;
#else /* WITH_EROSION */
    int regionWidth = widthTwo;
    int regionHeight = heightTwo;
    BYTE* regionPtr = regionIdentification(binaryImage, widthTwo, heightTwo, etiketler);

    saveBMP("images/regionResult.bmp", heightTwo, widthTwo, 
        convertIntensityToBMP(regionPtr, widthTwo, heightTwo, &size));
    dout << "REGION_TEST succed!" << endl;
#endif /* WITH_EROSION */
#endif /* REGION_TEST */

#if VARIANCE_TEST
    dout << "variance = " << getVariance(binaryImage, 0, 0, widthTwo, heightTwo, width) << endl;
#endif /* VARIANCE_TEST */

#if MOMENT_TEST
    int startX, startY, sizeW, sizeH;
    int etiketCounter = 1;

#if LEARN_TEST
    objectMoments_t test;
    test.fi[0] = 0;
    test.fi[1] = 0;
    test.fi[2] = 0;
    test.fi[3] = 0;
    test.fi[4] = 0;
    test.fi[5] = 0;
    test.fi[6] = 0;

    //start 1 because of background value 0
    int pcs = 0;
    for(int j = 1; j < 255; j++){
        getPoints(regionPtr, &startX, &startY, &sizeH, &sizeW, j, regionWidth, regionHeight);
        if(sizeH < 1 || sizeW < 1) continue;

        dout << "label = " << etiketCounter++ << ", value = " << j << endl;
        for(int i = 1; i < 8; i++){
            test.fi[i - 1] += getFi(regionPtr, i, startX, startY, sizeW, sizeH, regionWidth, j);
            //dout << "fi " << i << " result = " << test.fi[i - 1] << endl;
        }
        pcs++;
    }
    dout << pcs << endl;
    for(int i = 1; i < 8; i++){
        test.fi[i - 1] /= pcs;
        dout << "fi " << i << " result = " << test.fi[i - 1] << endl;
    }
#endif /* LEARN_TEST */
#if LEARNED_TEST
    int objectsNumber = 4;
    objectMoments_t objects[objectsNumber];
    objects[0].name = "mercimek"; //kırmızı
    objects[0].fi[0] = 0.163213;
    objects[0].fi[1] = -0.00791088;  
    objects[0].fi[2] = 0.00411905;
    objects[0].fi[3] = 9.22417e-05;
    objects[0].fi[4] = 1.12648e-07;
    objects[0].fi[5] = 1.68098e-06;
    objects[0].fi[6] = -0.0038401;

    objects[1].name = "badem"; // yeşil
    objects[1].fi[0] = 0.18519;
    objects[1].fi[1] = -0.0081568;
    objects[1].fi[2] = 0.473546;
    objects[1].fi[3] = 0.0478745;
    objects[1].fi[4] = 0.00874992;
    objects[1].fi[5] = 0.00383577;
    objects[1].fi[6] = 0.0874483;

    objects[2].name = "cubuk"; // mavi
    objects[2].fi[0] = 2.26793;
    objects[2].fi[1] = 3.27271;
    objects[2].fi[2] = 185.63;
    objects[2].fi[3] = 153.64;
    objects[2].fi[4] = 68316.7;
    objects[2].fi[5] = 292.904;
    objects[2].fi[6] = 38286.8;

    objects[3].name = "nohut"; // sarı
    objects[3].fi[0] = 0.165108;
    objects[3].fi[1] = 0.0129526;
    objects[3].fi[2] = 0.0336183;
    objects[3].fi[3] = 0.000833087;
    objects[3].fi[4] = -3.0922e-06;
    objects[3].fi[5] = -6.36499e-06;
    objects[3].fi[6] = 0.0158372;

    BYTE gapB = 250 / (objectsNumber + 1);

    for(int j = 1; j < 255; j++){
        getPoints(regionPtr, &startX, &startY, &sizeH, &sizeW, j, regionWidth, regionHeight);
        if(sizeH < 1 || sizeW < 1) continue;

        int isObjs[objectsNumber];
        for(int i = 0; i < objectsNumber; i++){
            isObjs[i] = 0;
        }

        dout << "label = " << setw(4) << left << etiketCounter++ << " - ";// << ", value = " << j << endl;
        for(int i = 1; i < 8; i++){
            double temp = getFi(regionPtr, i, startX, startY, sizeW, sizeH, regionWidth, j);
            //dout << "fi " << i << " result = " << test.fi[i - 1] << endl;
            double min = 1000000;
            for(int obj = 0; obj < objectsNumber; obj++){
                if(min > fabs(objects[obj].fi[i - 1] - temp)) min = fabs(objects[obj].fi[i - 1] - temp);
            }
            double epsilon = 0.00000001;
            for(int obj = 0; obj < objectsNumber; obj++){
                if(fabs(min - fabs(objects[obj].fi[i - 1] - temp)) <= epsilon){
                    isObjs[obj]++;
                    break;
                }
            }
        }
        BYTE grayColor = 0;
        int max = isObjs[0];
        for(int obj = 1; obj < objectsNumber; obj++){
            if(max < isObjs[obj]){
                max = isObjs[obj];
            }
        }
        for(int obj = 0; obj < objectsNumber; obj++){
            //max == isObjs[obj]
            if(max == isObjs[obj]){
                dout << setw(11) << objects[obj].name << setw(6) << " <-> " << isObjs[0] << ", " << isObjs[1] << ", " << isObjs[2] << ", " << isObjs[3] << " " <<endl;
                grayColor = gapB * (obj + 1);
                break;
            }
        }

        for(int i = startX; i < startX + sizeH; i++){
            for(int k = startY; k < startY + sizeW; k++){
                if(*(regionPtr + i * regionWidth + k) != j) continue;
                *(binaryImage + i * widthTwo + k) = grayColor;
            }
        }
    }
    const char* filePathResult = "images/realResult.bmp";
    const char* filePathResultC = "images/realResultColored.bmp";

    int sonEtiketler[objectsNumber];
    for(int obj = 0; obj < objectsNumber; obj++){
        sonEtiketler[obj] = gapB * (obj + 1);
    }

    saveBMP(filePathResultC, heightTwo, widthTwo, convertIntensityToColoredBMP(binaryImage, sonEtiketler, objectsNumber, widthTwo, heightTwo, &size));
    saveBMP(filePathResult, heightTwo, widthTwo, convertIntensityToBMP(binaryImage, widthTwo, heightTwo, &sizeTwo));
#endif /* LEARNED_TEST */
#endif /* MOMENT_TEST */

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
    //saveBMP(filePathClustedC, height, width, convertIntensityToColoredBMP(clustedImage, allT, k, width, height, &size));
    saveBMP(filePathClusted, height, width, convertIntensityToBMP(clustedImage, width, height, &size));

    delete[] clustedImage;

    dout << "THRESHOLD_K_TEST succed!" << endl;
#endif /* THRESHOLD_K_TEST */

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

    delete[] buffer;
    delete[] ramIntensity;

    file.close();
    return 0;
}