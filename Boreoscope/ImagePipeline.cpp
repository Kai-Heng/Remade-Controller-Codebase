#include "ImagePipeline.h"

ImagePipeline::~ImagePipeline() {
	singlePixelTransformations.clear();
	singleImageTransformations.clear();
}

ImagePipeline* ImagePipeline::addSinglePixelTransformation(const function<uint8_t(uint8_t)> transformation) {
	singlePixelTransformations.push_back(transformation);
	return this;
}

ImagePipeline* ImagePipeline::addSingleImageTransformation(const function<void(Image&)> transformation) {
	singleImageTransformations.push_back(transformation);
	return this;
}

void ImagePipeline::performTransformations(Image& image) {
	uint8_t* buffer = image.m_Buffer;
	const size_t payloadSize = ((size_t)image.m_sizeX) * image.m_sizeY;
	if (singlePixelTransformations.size()) for (size_t i = 0; i < payloadSize; i++) for (const function<uint8_t(uint8_t)> transformation : singlePixelTransformations) buffer[i] = transformation(buffer[i]);
	for (const function<void(Image&)> transformation : singleImageTransformations) transformation(image);
}