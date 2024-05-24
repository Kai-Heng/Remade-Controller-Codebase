#pragma once
#include <functional>

#include <pylon/PylonIncludes.h>
#include <pylon/PylonGUI.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"

using namespace Pylon;

/// <summary>
/// Pipeline to perform transformations on an image
/// </summary>
class ImagePipeline {
public:
    /// <summary>
    /// Frees up held resources
    /// </summary>
    ~ImagePipeline();

    /// <summary>
    /// Adds a transformation for single pixels to the pipeline
    /// </summary>
    /// <param name="transformation">Function which transforms a single pixel</param>
    /// <returns>Self</returns>
    ImagePipeline* addSinglePixelTransformation(const function<uint8_t(uint8_t)> transformation);

    /// <summary>
    /// Adds a transformation for single images to the pipeline
    /// </summary>
    /// <param name="transformation">Function which transforms a single image</param>
    /// <returns>Self</returns>
    ImagePipeline* addSingleImageTransformation(const function<void(Image&)> transformation);

    /// <summary>
    /// Performs stored transformations on the image provided
    /// </summary>
    /// <param name="image">Image to transform</param>
    void performTransformations(Image& image);

private:
    list<function<uint8_t(uint8_t)>> singlePixelTransformations;
    list<function<void(Image&)>> singleImageTransformations;
};

/// <summary>
/// Helper functions to produce pipeline steps
/// </summary>
class ImagePipelineHelper {
public:
    /// <summary>
    /// Produces function to trim leftmost columns from image
    /// </summary>
    /// <param name="columns">Number of columns to trim</param>
    /// <returns>New function to trim leftmost columns</returns>
    static function<void(Image&)> trimLeft(const uint32_t columns);

    /// <summary>
    /// Produces a function to trim rightmost columns from image
    /// </summary>
    /// <param name="columns">Number of columns to trim</param>
    /// <returns>New function to trim rightmost columns</returns>
    static function<void(Image&)> trimRight(const uint32_t columns);

    /// <summary>
    /// Produces a function to trim topmost rows from image
    /// </summary>
    /// <param name="rows">Number of rows to trim</param>
    /// <returns>New function to trim topmost rows</returns>
    static function<void(Image&)> trimTop(const uint32_t rows);

    /// <summary>
    /// Produces a function to trim bottommost rows
    /// </summary>
    /// <param name="rows">Number of rows to trim</param>
    /// <returns>New function to trim bottommost rows</returns>
    static function<void(Image&)> trimBottom(const uint32_t rows);

    /// <summary>
    /// Produces a function to blend together edges of single images within composite image to improve coherency
    /// </summary>
    /// <param name="singleImageWidth">Width of single image</param>
    /// <param name="singleImageHeight">Height of single image</param>
    /// <param name="furthestDistanceFromEdge">Furthest distance from edge to be blended</param>
    /// <returns>New function to blend together edges of single images in composite image</returns>
    static function<void(Image&)> blendEdges(const uint32_t singleImageWidth, const uint32_t singleImageHeight, const uint32_t furthestDistanceFromEdge);

    /// <summary>
    /// Produces a function to blackout an entire image (sets all pixel values to 0)
    /// </summary>
    /// <returns>New function to blackout an entire image</returns>
    static function<uint8_t(uint8_t)> blackout();

    /// <summary>
    /// Produces a function to reduce brightness of image
    /// </summary>
    /// <param name="amount">Amount by which to reduce brightness; for example, to reduce by 25% use 0.25 (retain 75%)</param>
    /// <returns>New function to reduce brightness of image</returns>
    static function<uint8_t(uint8_t)> dull(const double amount);

    /// <summary>
    /// Produces a function to increase brightness of image
    /// </summary>
    /// <param name="amount">Amount by which to increase brightness</param>
    /// <returns>New function to increase brightness of image</returns>
    static function<uint8_t(uint8_t)> brighten(const double amount);

    /// <summary>
    /// Produces a function to increase contrast of image
    /// </summary>
    /// <param name="amount">Amount by which to increase contrast; range from -255 to 255</param>
    /// <returns>New function to increase contrast of image</returns>
    static function<uint8_t(uint8_t)> contrast(const double amount);
};