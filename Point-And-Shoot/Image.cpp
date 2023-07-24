#include "Image.h"

Image::Image(const size_t payloadSize) {
    m_Buffer = new uint8_t[payloadSize];
}

Image::~Image() {}

void Image::releaseBuffer()
{
    delete[] m_Buffer;
    m_Buffer = NULL;
}

void Image::rotateClockwise() {
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    uint8_t* orig_buffer = new uint8_t[payloadSize];
    memcpy(orig_buffer, m_Buffer, payloadSize);
    for (size_t i = 0; i < payloadSize; i++) m_Buffer[i] = orig_buffer[(m_sizeY - 1 - i % m_sizeY) * m_sizeX + i / m_sizeY];
    delete[] orig_buffer;
    const uint32_t temp = m_sizeX;
    m_sizeX = m_sizeY;
    m_sizeY = temp;
}

void Image::rotateCounterClockwise() {
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    uint8_t* orig_buffer = new uint8_t[payloadSize];
    memcpy(orig_buffer, m_Buffer, payloadSize);
    for (size_t i = 0; i < payloadSize; i++) m_Buffer[i] = orig_buffer[(i % m_sizeY) * m_sizeX + m_sizeX - 1 - i / m_sizeY];
    delete[] orig_buffer;
    const uint32_t temp = m_sizeX;
    m_sizeX = m_sizeY;
    m_sizeY = temp;
}

void Image::trimLeft(const uint32_t columns) {
    if (columns > m_sizeX) return;
    const uint32_t newSizeX = m_sizeX - columns;
    uint8_t* new_buffer = new uint8_t[((size_t)newSizeX) * m_sizeY];
    for (uint32_t i = 0; i < m_sizeY; i++) memcpy(new_buffer + ((size_t)i) * newSizeX, m_Buffer + ((size_t)i) * m_sizeX + columns, newSizeX);
    m_sizeX = newSizeX;
    delete[] m_Buffer;
    m_Buffer = new_buffer;
}

void Image::trimRight(const uint32_t columns) {
    if (columns > m_sizeX) return;
    const uint32_t newSizeX = m_sizeX - columns;
    uint8_t* new_buffer = new uint8_t[((size_t)newSizeX) * m_sizeY];
    for (uint32_t i = 0; i < m_sizeY; i++) memcpy(new_buffer + ((size_t)i) * newSizeX, m_Buffer + ((size_t)i) * m_sizeX, newSizeX);
    m_sizeX = newSizeX;
    delete[] m_Buffer;
    m_Buffer = new_buffer;
}

void Image::trimTop(const uint32_t rows) {
    if (rows > m_sizeY) return;
    m_sizeY -= rows;
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    uint8_t* new_buffer = new uint8_t[payloadSize];
    memcpy(new_buffer, m_Buffer + ((size_t)rows) * m_sizeX, payloadSize);
    delete[] m_Buffer;
    m_Buffer = new_buffer;
}

void Image::trimBottom(const uint32_t rows) {
    if (rows > m_sizeY) return;
    m_sizeY -= rows;
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    uint8_t* new_buffer = new uint8_t[payloadSize];
    memcpy(new_buffer, m_Buffer, payloadSize);
    delete[] m_Buffer;
    m_Buffer = new_buffer;
}

void Image::ellipse(const uint32_t radiusX, const uint32_t radiusY) {
    if (2 * radiusX > m_sizeX || 2 * radiusY > m_sizeY) return;
    const uint32_t trimX = m_sizeX / 2 - radiusX;
    trimLeft(trimX);
    trimRight(trimX);
    const uint32_t trimY = m_sizeY / 2 - radiusY;
    trimTop(trimY);
    trimBottom(trimY);
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    const uint32_t centerX = m_sizeX / 2;
    const uint32_t centerY = m_sizeY / 2;
    const uint32_t radiusXSquared = radiusX * radiusX;
    const uint32_t radiusYSquared = radiusY * radiusY;
    for (size_t i = 0; i < payloadSize; i++) if (((double)((i % m_sizeX - centerX) * (i % m_sizeX - centerX))) / radiusXSquared + ((double)((i / m_sizeX - centerY) * (i / m_sizeX - centerY))) / radiusYSquared > 1) m_Buffer[i] = 0;
}

void Image::circle(const uint32_t radius) {
    if (2 * radius > min(m_sizeX, m_sizeY)) return;
    const uint32_t trimX = m_sizeX / 2 - radius;
    trimLeft(trimX);
    trimRight(trimX);
    const uint32_t trimY = m_sizeY / 2 - radius;
    trimTop(trimY);
    trimBottom(trimY);
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    const uint32_t centerX = m_sizeX / 2;
    const uint32_t centerY = m_sizeY / 2;
    const uint32_t radiusSquared = radius * radius;
    for (size_t i = 0; i < payloadSize; i++) if ((i % m_sizeX - centerX) * (i % m_sizeX - centerX) + (i / m_sizeX - centerY) * (i / m_sizeX - centerY) > radiusSquared) m_Buffer[i] = 0;
}

void Image::blackout() {
    fill(m_Buffer, m_Buffer + ((size_t)m_sizeX) * m_sizeY, 0);
}

void Image::dull(const double amount) {
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    for (size_t i = 0; i < payloadSize; i++) {
        if (!m_Buffer[i] || !(m_Buffer[i] - 255)) continue;
        m_Buffer[i] = (uint8_t)max(0, m_Buffer[i] * (1 - amount));
    }
}

void Image::brighten(const double amount) {
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    for (size_t i = 0; i < payloadSize; i++) {
        if (!m_Buffer[i] || !(m_Buffer[i] - 255)) continue;
        m_Buffer[i] = (uint8_t)min(255, m_Buffer[i] * (1 + amount));
    }
}

void Image::contrast(const double amount) {
    //uint8_t min = 255;
    //uint8_t max = 0;
    const size_t payloadSize = ((size_t)m_sizeX) * m_sizeY;
    /*for (uint32_t i = 0; i < payloadSize; i++) {
        if (m_Buffer[i] < min) min = m_Buffer[i];
        if (m_Buffer[i] > max) max = m_Buffer[i];
    }
    const double range = max - min + 1.0;
    for(uint32_t i = 0; i < payloadSize; i++) {
        if (!m_Buffer[i]) continue;
        m_Buffer[i] = min(m_Buffer[i] * ((m_Buffer[i] - min) / range + 0.5) * 2, 255);
    }*/
    const double contrastFactor = 259.0 * (255.0 + amount) / (255.0 * (259.0 - amount));
    for (size_t i = 0; i < payloadSize; i++) {
        if (!m_Buffer[i] || !(m_Buffer[i] - 255)) continue;
        m_Buffer[i] = (uint8_t)max(0, min(255, contrastFactor * (m_Buffer[i] - 128.0) + 128.0));
    }
}

void Image::saveImage(const char* const filename, const EImageFileFormat format) const {
    CPylonImage temp;
    temp.AttachUserBuffer(m_Buffer, ((size_t)m_sizeX) * m_sizeY, m_PixelType, m_sizeX, m_sizeY, m_PaddingX, m_PylonImageOrientation);
    temp.Save(format, filename);
}

Image Image::from(const IImage& image) {
    Image img(((size_t)image.GetHeight()) * image.GetWidth());

    img.m_PaddingX = image.GetPaddingX();
    img.m_PixelType = image.GetPixelType();
    img.m_sizeY = image.GetHeight();
    img.m_sizeX = image.GetWidth();
    img.m_PylonImageOrientation = image.GetOrientation();
    memcpy(img.m_Buffer, image.GetBuffer(), ((size_t)img.m_sizeX) * img.m_sizeY);

    return img;
}

Image Image::loadImage(const char* const filename) {
    CPylonImage temp;
    temp.Load(filename);
    return from(temp);
}

void Image::display(const unsigned int window) const {
    CPylonImage pylonimage;
    pylonimage.AttachUserBuffer(m_Buffer, ((size_t)m_sizeX) * m_sizeY, m_PixelType, m_sizeX, m_sizeY, m_PaddingX, m_PylonImageOrientation);
	Pylon::DisplayImage(window, pylonimage);
}

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

    const Image firstImage = ListTemp.front();
    const uint32_t imageWidth = firstImage.m_sizeX;
    const uint32_t rowSize = imageWidth * numImagesX;
    const uint32_t imageHeight = firstImage.m_sizeY;
    const uint32_t columnSize = imageHeight * numImagesY;

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