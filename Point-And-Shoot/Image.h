#pragma once
#include <mutex>

#include <pylon/PylonIncludes.h>
#include <pylon/PylonGUI.h>
#include <pylon/BaslerUniversalInstantCamera.h>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

/// <summary>
/// Stores image and relevant information
/// </summary>
class Image 
{
public:
    uint8_t* m_Buffer = NULL;
    EPixelType m_PixelType = PixelType_Undefined;
    uint32_t m_sizeX = 0;
    uint32_t m_sizeY = 0;
    EImageOrientation m_PylonImageOrientation = ImageOrientation_TopDown;
    size_t m_PaddingX = 0;
    
    /// <summary>
    /// Creates a new image with the specified image buffer size
    /// </summary>
    /// <param name="payloadSize">Size of image buffer</param>
    Image(const size_t payloadSize);

    ~Image();

    /// <summary>
    /// Deletes the memory storing the image buffer
    /// </summary>
    void releaseBuffer();

    /// <summary>
    /// Rotates the image 90 degrees clockwise
    /// </summary>
    void rotateClockwise();

    /// <summary>
    /// Rotates the image 90 degrees counterclockwise
    /// </summary>
    void rotateCounterClockwise();

    /// <summary>
    /// Trims off the leftmost columns
    /// </summary>
    /// <param name="columns">Number of columns to trim</param>
    void trimLeft(const uint32_t columns);

    /// <summary>
    /// Trims off the rightmost columns
    /// </summary>
    /// <param name="columns">Number of columns to trim</param>
    void trimRight(const uint32_t columns);

    /// <summary>
    /// Trims off the top rows
    /// </summary>
    /// <param name="rows">Number of rows to trim</param>
    void trimTop(const uint32_t rows);

    /// <summary>
    /// Trims off the bottom rows
    /// </summary>
    /// <param name="rows">Number of rows to trim</param>
    void trimBottom(const uint32_t rows);

    /// <summary>
    /// Retains an ellipse of the image and blacks out pixels remaining in the image after trimming outside of the ellipse; centered at center of image by default
    /// </summary>
    /// <param name="radiusX">Radius of ellipse in horizontal dimension</param>
    /// <param name="radiusY">Radius of ellipse in vertical dimension</param>
    void ellipse(const uint32_t radiusX, const uint32_t radiusY);

    /// <summary>
    /// Retains a circle of the image and blacks out pixels remaining in the image after trimming outside of the circle; centered at center of image by default
    /// </summary>
    /// <param name="radius">Radius of the circle</param>
    void circle(const uint32_t radius);

    /// <summary>
    /// Fills entire image with black
    /// </summary>
    void blackout();

    /// <summary>
    /// Reduces all pixel values by the specified amount
    /// </summary>
    /// <param name="amount">Amount by which to reduce pixels; for example, to reduce by 25%, use 0.25 (retains 75% of pixel value)</param>
    void dull(const double amount);

    /// <summary>
    /// Brightens all pixel values by the specified amount
    /// </summary>
    /// <param name="amount">Amount by which to brighten pixels; for example, to brighten by 25%, use 0.25 (value becomes 125% of original value)</param>
    void brighten(const double amount);

    /// <summary>
    /// Increases the contrast between high-value and low-value pixels by specified amount
    /// </summary>
    /// <param name="amount">Amount by which to adjust contrast, between -255 and 255</param>
    void contrast(const double amount);

    /// <summary>
    /// Saves the image to the specified file in the specified format
    /// </summary>
    /// <param name="filename">Full path to file including name</param>
    /// <param name="format">Format of image</param>
    void saveImage(const char* const filename, const EImageFileFormat format) const;

    /// <summary>
    /// Creates an image from another image
    /// </summary>
    /// <param name="image">Original image</param>
    /// <returns>New image</returns>
    static Image from(const IImage& image);

    /// <summary>
    /// Loads an image from the specified file
    /// </summary>
    /// <param name="filename">Full path to file</param>
    /// <returns>Image loaded from disk</returns>
    static Image loadImage(const char* const filename);

    /// <summary>
    /// Displays an image
    /// </summary>
    /// <param name="window">Window number in which to display image; defaults to 0 (first window)</param>
    void display(const unsigned int window = 0) const;
};

/// <summary>
/// Stitches multiple images together
/// </summary>
class ImageStitcher
{
public:
    /// <summary>
    /// Creates a new image stitcher with the images to be stitched
    /// </summary>
    /// <param name="images">Images to be stitched</param>
    ImageStitcher(vector<Image> images) {
        Imagelist = images;
    }

    /// <summary>
    /// Adds an image to be stitched
    /// </summary>
    /// <param name="image">Image to add to list of images to be stitched</param>
    void addGrabResult(const Image& image);
    
    /// <summary>
	/// Stitches together the stored images
	/// </summary>
	/// <param name="numImagesX">Number of images to be stitched in the horizontal dimension</param>
	/// <param name="numImagesY">Number of images to be stitched in the vertical dimension</param>
	/// <param name="resultImage">Image in which to store the resulting stitched image</param>
	void GetStitchedImage(const size_t numImagesX, const size_t numImagesY, Image& resultImage);

    /// <summary>
    /// Cleans up the image stitcher by deleting the memory storing the images that were stitched together
    /// <summary>
    ~ImageStitcher(void)
    {
        if (Imagelist.size() > 0)
        {
            for (Image image : Imagelist) image.releaseBuffer();
            Imagelist.clear();
        }
    };

    /// <summary>
    /// Saves all images in the list to the specified path in the specified format
    /// </summary>
    /// <param name="filename">Prefix of all images filenames including path; images saved as filename000.bmp</param>
    /// <param name="format">Format of images</param>
    void saveAllImages(const char* const filename, EImageFileFormat format) const;

    /// <summary>
    /// Displays all images each in a separate window starting with the specified window
    /// </summary>
    /// <param name="startWindow">Window number from which to start displaying images; defaults to 0</param>
    void displayAllImages(const unsigned int startWindow = 0) const;

private:
    mutex mtx_add, mtx_get;
    vector<Image> Imagelist;
};