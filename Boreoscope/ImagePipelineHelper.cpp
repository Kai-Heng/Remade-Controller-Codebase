#include "ImagePipeline.h"

function<void(Image&)> ImagePipelineHelper::trimLeft(const uint32_t columns) {
    return [columns](Image& image) {
        image.trimLeft(columns);
    };
}

function<void(Image&)> ImagePipelineHelper::trimRight(const uint32_t columns) {
    return [columns](Image& image) {
        image.trimRight(columns);
    };
}

function<void(Image&)> ImagePipelineHelper::trimTop(const uint32_t rows) {
    return [rows](Image& image) {
        image.trimTop(rows);
    };
}

function<void(Image&)> ImagePipelineHelper::trimBottom(const uint32_t rows) {
    return [rows](Image& image) {
        image.trimBottom(rows);
    };
}

function<void(Image&)> ImagePipelineHelper::blendEdges(const uint32_t singleImageWidth, const uint32_t singleImageHeight, const uint32_t furthestDistanceFromEdge) {
    return [singleImageWidth, singleImageHeight, furthestDistanceFromEdge](Image& image) {
        const uint32_t payloadSize = image.m_sizeX * image.m_sizeY;
        for (uint32_t i = singleImageWidth; i < image.m_sizeX; i += singleImageWidth) {
            for (uint32_t j = 0; j < image.m_sizeY; j++) {
                for (uint32_t k = 0; k < furthestDistanceFromEdge; k++) {
                    uint8_t* const first = image.m_Buffer + i + ((size_t)j) * image.m_sizeY - k;
                    uint8_t* const second = image.m_Buffer + i + ((size_t)j) * image.m_sizeY + k;
                    const uint8_t avg = (*first + *second) / 2;
                    *first = (*first * (k + 1) + avg) / (k + 2);
                    *second = (*second * (k + 1) + avg) / (k + 2);
                }
            }
        }
        const uint32_t completeRowSize = singleImageHeight * image.m_sizeX;
        for (uint32_t i = completeRowSize; i < payloadSize; i += completeRowSize) {
            for (uint32_t j = 0; j < image.m_sizeX; j++) {
                for (uint32_t k = 0; k < furthestDistanceFromEdge; k++) {
                    uint8_t* const first = image.m_Buffer + i + j - ((size_t)k) * image.m_sizeX;
                    uint8_t* const second = image.m_Buffer + i + j + ((size_t)k) * image.m_sizeX;
                    const uint8_t avg = (*first + *second) / 2;
                    *first = (*first * (k + 1) + avg) / (k + 2);
                    *second = (*second * (k + 1) + avg) / (k + 2);
                }
            }
        }
    };
}

function<uint8_t(uint8_t)> ImagePipelineHelper::blackout() {
    return [](const uint8_t pixel) {
        return 0;
    };
}

function<uint8_t(uint8_t)> ImagePipelineHelper::dull(const double amount) {
    return [amount](const uint8_t pixel) {
        return (uint8_t)max(0, pixel * (1 - amount));
    };
}

function<uint8_t(uint8_t)> ImagePipelineHelper::brighten(const double amount) {
    return [amount](const uint8_t pixel) {
        return (uint8_t)min(255, pixel * (1 + amount));
    };
}

function<uint8_t(uint8_t)> ImagePipelineHelper::contrast(const double amount) {
    const double contrastFactor = 259.0 * (255.0 + amount) / (255.0 * (259.0 - amount));
    return [contrastFactor](const uint8_t pixel) {
        return (uint8_t)max(0, min(255, contrastFactor * (pixel - 128.0) + 128.0));
    };
}