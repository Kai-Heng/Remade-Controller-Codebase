#include "Image.h"

std::vector<Image> ImageSplitter::getSplittedImages(const size_t imageWidth, const size_t imageHeight)
{
	vector<Image> v;

	size_t numImagesX = ceil((double)image.m_sizeX / imageWidth);
	size_t numImagesY = ceil((double)image.m_sizeY / imageHeight);

	v.reserve(numImagesX * numImagesY);

	cout << "Number of images: " << numImagesX * numImagesY << endl;

	for (uint64_t j = 0; j < numImagesY; j++) {
		for (uint64_t i = 0; i < numImagesX; i++) {
			size_t isize = imageWidth * imageHeight;
			Image temp(isize);

			temp.m_sizeX					= imageWidth;
			temp.m_sizeY					= imageHeight;
			temp.m_PylonImageOrientation	= image.m_PylonImageOrientation;
			temp.m_PaddingX					= image.m_PaddingX;
			temp.m_PixelType				= image.m_PixelType;

			for (uint64_t y = 0; y < imageHeight; y++) {
				for (uint64_t x = 0; x < imageWidth; x++) {
					if (i * imageWidth + x >= image.m_sizeX || j * imageHeight + y >= image.m_sizeY) {
						temp.m_Buffer[y * imageWidth + x] = 0;
					}
					else {
						temp.m_Buffer[y * imageWidth + x] = image.m_Buffer[j * imageHeight * image.m_sizeX + y * image.m_sizeX + i * imageWidth + x];
					}
				}
			}
			v.push_back(temp);
			//temp.releaseBuffer();
		}
	}

	for (auto i : v) {
		i.display();
		Sleep(1000);
	}

	return v;
}