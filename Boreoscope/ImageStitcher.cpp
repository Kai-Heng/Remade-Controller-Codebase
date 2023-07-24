#include "Image.h"

void ImageStitcher::saveAllImages(const char* const filenamePrefix, const EImageFileFormat format) const {
    size_t filenamelen = 0;
    while (filenamePrefix[filenamelen++]);
    char* const newfilename = (char*)malloc((filenamelen + 7) * sizeof(char));
    if (!newfilename) return;
    for (size_t i = 0; i < filenamelen; i++) newfilename[i] = filenamePrefix[i];
    newfilename[filenamelen + 2] = '.';
    newfilename[filenamelen + 3] = 'b';
    newfilename[filenamelen + 4] = 'm';
    newfilename[filenamelen + 5] = 'p';
    newfilename[filenamelen + 6] = 0;
    for (size_t i = 0; i < Imagelist.size(); i++) {
        newfilename[filenamelen - 1] = '0' + (i / 100) % 10;
        newfilename[filenamelen] = '0' + (i / 10) % 10;
        newfilename[filenamelen + 1] = '0' + i % 10;
        Imagelist[i].saveImage(newfilename, format);
    }
}

void ImageStitcher::displayAllImages(const unsigned int startWindow) const {
    const size_t stopWindow = startWindow + Imagelist.size();
    for (size_t i = startWindow; i < stopWindow; i++) Imagelist[i].display(i);
}

void ImageStitcher::GetStitchedImage(const size_t numImagesX, const size_t numImagesY, Image& resultImage) {
    const size_t totalNumImages = numImagesX * numImagesY;
	
    if (Imagelist.size() < totalNumImages) return;
    
    vector<Image> ListTemp;
    ListTemp.reserve(totalNumImages);

    mtx_get.lock();

    for (size_t i = 0; i < totalNumImages; i++)
    {
        // make a tem list in order to avoid blocking the main list while memcopy().
        ListTemp.push_back(Imagelist[0]);
        Imagelist.erase(Imagelist.begin());
    }  
    mtx_get.unlock();

    const Image firstImage      = ListTemp.front();
    const uint32_t imageWidth   = firstImage.m_sizeX;
    const uint32_t rowSize      = imageWidth * numImagesX;
    const uint32_t imageHeight  = firstImage.m_sizeY;
    const uint32_t columnSize   = imageHeight * numImagesY;

    resultImage.m_PaddingX = firstImage.m_PaddingX;
    resultImage.m_PixelType = firstImage.m_PixelType;
    resultImage.m_PylonImageOrientation = firstImage.m_PylonImageOrientation;
    resultImage.m_sizeX = rowSize;
    resultImage.m_sizeY = columnSize;

    uint32_t x = 0, y = 0;
    while (y < columnSize) {
        memcpy(resultImage.m_Buffer + ((size_t)x) * imageWidth + ((size_t)y) * rowSize, ListTemp[x + y / imageHeight * numImagesX].m_Buffer + ((size_t)(y % imageHeight)) * imageWidth, imageWidth);
        x++;
        if (x == numImagesX) {
            y++;
            x = 0;
        }
    }

    for (Image image : ListTemp) image.releaseBuffer();
    ListTemp.clear();
}

void ImageStitcher::addGrabResult(const Image& image) {
    mtx_add.lock();
    Imagelist.push_back(image);
    mtx_add.unlock();
}