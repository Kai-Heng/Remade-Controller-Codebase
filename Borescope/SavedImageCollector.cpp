#include "ImageCollector.h"

SavedImageCollector::SavedImageCollector(char** const filenames, const unsigned int numImages) :
    filenames(filenames),
    numImages(numImages)
{}

vector<Image> SavedImageCollector::GetImages() {
    vector<Image> images;
    for (unsigned int i = 0; i < numImages; i++) images.push_back(Image::loadImage(filenames[i]));
    return images;
}