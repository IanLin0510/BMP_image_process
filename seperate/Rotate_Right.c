//引入標頭檔案以使用標準輸入/輸出、動態記憶體分配、布爾型別和數學函數。
//定義常數以表示 BMP 文件頭和資訊頭的偏移量和大小。
//定義自訂資料型別以提高程式碼可讀性。

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// 定義 BMP 文件頭和資訊頭的偏移量及大小
#define BITMAP_DATA_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0

typedef unsigned int uint32;
typedef short int16;
typedef unsigned char uchar;

//以二進位模式開啟 BMP 文件。
//讀取圖像資料偏移量、寬度、高度和每像素的位數。
//計算每像素的位元組數和每行的位元組數（包含填充位元組）。
//為像素資料分配記憶體並逐行讀取像素資料。
//關閉文件並返回成功標誌。


bool readImage(const char *fileName, uchar **pixels, uint32 *width, uint32 *height, uint32 *bytesPerPixel)
{
    // 以二進位模式開啟 BMP 文件
    FILE *imageFile = fopen(fileName, "rb");
    if (imageFile == NULL) {
        printf("無法開啟文件 %s \n", fileName);
        return false;
    }

    // 讀取 BMP 文件頭中的圖像資料偏移量
    uint32 dataOffset;
    fseek(imageFile, BITMAP_DATA_OFFSET, SEEK_SET);
    fread(&dataOffset, 4, 1, imageFile);

    // 讀取圖像的寬度
    fseek(imageFile, WIDTH_OFFSET, SEEK_SET);
    fread(width, 4, 1, imageFile);

    // 讀取圖像的高度
    fseek(imageFile, HEIGHT_OFFSET, SEEK_SET);
    fread(height, 4, 1, imageFile);

    // 讀取每像素的位數
    int16 bitsPerPixel;
    fseek(imageFile, BITS_PER_PIXEL_OFFSET, SEEK_SET);
    fread(&bitsPerPixel, 2, 1, imageFile);

    // 計算每像素的位元組數
    *bytesPerPixel = ((uint32)bitsPerPixel) / 8;

    // 計算每行的位元組數（包含填充位元組）
    int paddedRowSize = (int)(4 * ceil((float)(*width)/4.0f)) * (*bytesPerPixel);
    int unpaddedRowSize = (*width) * (*bytesPerPixel);
    int totalSize = unpaddedRowSize * (*height);

    // 為像素資料分配記憶體
    *pixels = (uchar*)malloc(totalSize);

    // 從底部到頂部逐行讀取像素資料
    int i = 0;
    uchar *currentRowPointer = *pixels + ((*height-1) * unpaddedRowSize);

    for (i = 0; i < *height; i++) {
        fseek(imageFile, dataOffset + (i * paddedRowSize), SEEK_SET);
        fread(currentRowPointer, 1, unpaddedRowSize, imageFile);
        currentRowPointer -= unpaddedRowSize;
    }

    fclose(imageFile);
    return true;
}

//以二進位模式開啟輸出文件。
//寫入 BMP 文件頭和資訊頭。
//計算並寫入文件大小、偏移量、寬度、高度等資訊。
//逐行寫入像素資料，考慮填充位元組。
//關閉文件。

void writeImage(const char *fileName, uchar *pixels, uint32 width, uint32 height, uint32 bytesPerPixel)
{
    // 以二進位模式開啟輸出文件
    FILE *outputFile = fopen(fileName, "wb");

    // === BMP 文件頭 === //
    // 寫入 BM 標誌
    const char *BM = "BM";
    fwrite(&BM[0], 1, 1, outputFile);
    fwrite(&BM[1], 1, 1, outputFile);

    // 計算並寫入文件大小
    int paddedRowSize = (int)(4 * ceil((float)width/4.0f)) * bytesPerPixel;
    uint32 fileSize = paddedRowSize * height + HEADER_SIZE + INFO_HEADER_SIZE;
    fwrite(&fileSize, 4, 1, outputFile);

    // 寫入保留位
    uint32 reserved = 0x0000;
    fwrite(&reserved, 4, 1, outputFile);

    // 寫入圖像資料偏移量
    uint32 dataOffset = HEADER_SIZE + INFO_HEADER_SIZE;
    fwrite(&dataOffset, 4, 1, outputFile);

    // === BMP 資訊頭 === //
    // 寫入資訊頭大小
    uint32 infoHeaderSize = INFO_HEADER_SIZE;
    fwrite(&infoHeaderSize, 4, 1, outputFile);

    // 寫入寬度和高度
    fwrite(&width, 4, 1, outputFile);
    fwrite(&height, 4, 1, outputFile);

    // 寫入色彩平面數，總是1
    int16 planes = 1;
    fwrite(&planes, 2, 1, outputFile);

    // 寫入每像素位數
    int16 bitsPerPixel = bytesPerPixel * 8;
    fwrite(&bitsPerPixel, 2, 1, outputFile);

    // 寫入壓縮方式
    uint32 compression = NO_COMPRESION;
    fwrite(&compression, 4, 1, outputFile);

    // 寫入圖像大小（位元組）
    uint32 imageSize = width * height * bytesPerPixel;
    fwrite(&imageSize, 4, 1, outputFile);

    // 寫入解析度（每米像素數）
    uint32 resolutionX = 3937; // 100 dpi * 39.37 inch/meter
    uint32 resolutionY = 3937; // 100 dpi * 39.37 inch/meter
    fwrite(&resolutionX, 4, 1, outputFile);
    fwrite(&resolutionY, 4, 1, outputFile);

    // 寫入使用顏色數
    uint32 colorsUsed = MAX_NUMBER_OF_COLORS;
    fwrite(&colorsUsed, 4, 1, outputFile);

    // 寫入重要顏色數
    uint32 importantColors = ALL_COLORS_REQUIRED;
    fwrite(&importantColors, 4, 1, outputFile);

    // 從底部到頂部逐行寫入像素資料
    int i = 0;
    int unpaddedRowSize = width * bytesPerPixel;

    for (i = 0; i < height; i++) {
        int pixelOffset = ((height - 1) - i) * unpaddedRowSize;
        fwrite(&pixels[pixelOffset], 1, paddedRowSize, outputFile);
    }

    fclose(outputFile);
    return;
}

//順時鐘轉
void rotateRight(uchar *pixels, uint32 width, uint32 height, uint32 bytesPerPixel)
{
    int unpaddedRowSize = width * bytesPerPixel;
    uchar tempPixel;

    // 上面兩個換
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width / 2; j++) {
            for (int k = 0; k < bytesPerPixel; k++) {
                // 交換每行中的對稱像素
                tempPixel = *(pixels + i * unpaddedRowSize + j * bytesPerPixel + k);
                *(pixels + i * unpaddedRowSize + j * bytesPerPixel + k) = *(pixels + i * unpaddedRowSize + (width - 1 - j) * bytesPerPixel + k);
                *(pixels + i * unpaddedRowSize + (width - 1 - j) * bytesPerPixel + k) = tempPixel;
            }
        }
    }
    //我想知道高度寬度一半是多少

    printf("%d,  %d",height,width);

    //左邊兩個垂直轉
    for (int i = 0; i < height / 2; i++) {
        for (int j = 0; j < unpaddedRowSize/2; j++) {
            tempPixel = *(pixels + i * unpaddedRowSize + j);
            *(pixels + i * unpaddedRowSize + j) = *(pixels + (height - 1 - i) * unpaddedRowSize + j);
            *(pixels + (height - 1 - i) * unpaddedRowSize + j) = tempPixel;
        }
    }
    //下面兩個換
    for (int i = 50; i < height; i++) {
        for (int j = 0; j < width / 2; j++) {
            for (int k = 0; k < bytesPerPixel; k++) {
                // 交換每行中的對稱像素
                tempPixel = *(pixels + i * unpaddedRowSize + j * bytesPerPixel + k);
                *(pixels + i * unpaddedRowSize + j * bytesPerPixel + k) = *(pixels + i * unpaddedRowSize + (width - 1 - j) * bytesPerPixel + k);
                *(pixels + i * unpaddedRowSize + (width - 1 - j) * bytesPerPixel + k) = tempPixel;
            }
        }
    }
}

int main(void)
{
    uchar *pixels;
    uint32 width;
    uint32 height;
    uint32 bytesPerPixel;

    // 讀取圖像
    if (!readImage("img.bmp", &pixels, &width, &height, &bytesPerPixel)) {
        return -1;
    }
 
    
    //順時針用下面
    rotateRight(pixels, width, height, bytesPerPixel);

    writeImage("img4.bmp", pixels, width, height, bytesPerPixel);

    free(pixels);
    

    return 0;
}
