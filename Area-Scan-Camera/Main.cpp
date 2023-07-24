#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"
#include "ImageCollector.h"
#include "ImagePipeline.h"
#include <map>

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

/// <summary>
/// Manually captures images and stitches them together into a composite image
/// </summary>
/// <param name="numImagesX">Number of images in horizontal dimension</param>
/// <param name="numImagesY">Number of images in vertical dimension</param>
/// <param name="singleImageFilename">Prefix path for each single image for saving; if set to 0, won't save images; for example, if set to "a/b/c" will be saved as "a/b/c000.bmp", "a/b/c001.bmp", etc.</param>
/// <param name="output">Full path to save composite image including filename with extension; if set to 0 won't save composite</param>
void createCompositeImage(const size_t numImagesX, const size_t numImagesY, const char* const singleImageFilename, const char* const output) {
    CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
	camera.Open();

	camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();
    camera.ExposureMode.SetValue(ExposureMode_Timed);
    camera.ExposureAuto.SetValue(ExposureAuto_Off);
    //camera.ExposureTimeRaw.SetValue(89950);
    //camera.ExposureTimeRaw.SetValue(27000);
    //camera.ExposureTimeRaw.SetValue(150375);
    //camera.ExposureTimeRaw.SetValue(7500);
    //camera.ExposureTimeRaw.SetValue(2500);
    camera.ExposureTimeRaw.SetValue(350000);
    const size_t totalNumImages = numImagesX * numImagesY;
    IImageCollector* imageCollector = new ManualCaptureImageCollector(camera, totalNumImages);
    vector<Image> images = imageCollector->GetImages();
    ImageStitcher imageStitcher(images);
    const uint32_t imageWidth = images[0].m_sizeX;
    const uint32_t imageHeight = images[0].m_sizeY;
    if(singleImageFilename) imageStitcher.saveAllImages(singleImageFilename, ImageFileFormat_Bmp);
    Image IMG((size_t)camera.PayloadSize.GetValue() * totalNumImages);
    imageStitcher.GetStitchedImage(numImagesX, numImagesY, IMG);
    ImagePipeline pipeline;
    pipeline.addSingleImageTransformation(ImagePipelineHelper::blendEdges(imageWidth, imageHeight, min(imageWidth, imageHeight) / 2));
    pipeline.performTransformations(IMG);
    if(output) IMG.saveImage(output, ImageFileFormat_Bmp);
    IMG.display();
    IMG.releaseBuffer();
    delete imageCollector;
}

/// <summary>
/// Used for calibrating vertical shift; loads starting image from disk and then captures current image and puts them together to see if they align
/// </summary>
/// <param name="start">Full path to starting image</param>
/// <param name="output">Full path to save composite image including filename and extension; if set to 0 won't save composite</param>
void calibrateVerticalShift(const char* const start, const char* const output) {
    Image startImage = Image::loadImage(start);
    CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
    camera.Open();

    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();
    camera.ExposureMode.SetValue(ExposureMode_Timed);
    camera.ExposureAuto.SetValue(ExposureAuto_Off);
    //camera.ExposureTimeRaw.SetValue(89950);
    //camera.ExposureTimeRaw.SetValue(27000);
    //camera.ExposureTimeRaw.SetValue(150375);
    //camera.ExposureTimeRaw.SetValue(7500);
    //camera.ExposureTimeRaw.SetValue(2500);
    camera.ExposureTimeRaw.SetValue(350000);
    IImageCollector* imageCollector = new CaptureImageCollector(camera, 1);
    vector<Image> images;
    images.push_back(startImage);
    images.push_back(imageCollector->GetImages()[0]);
    const uint32_t imageWidth = startImage.m_sizeX;
    const uint32_t imageHeight = startImage.m_sizeY;
    ImageStitcher imageStitcher(images);
    Image IMG(((size_t)imageWidth) * imageHeight * 2);
    imageStitcher.GetStitchedImage(1, 2, IMG);
    if(output) IMG.saveImage(output, ImageFileFormat_Bmp);
    IMG.display();
    IMG.releaseBuffer();
    delete imageCollector;
}

/// <summary>
/// Creates composite image from previously saved images
/// </summary>
/// <param name="numImagesX">Number of images in horizontal dimension</param>
/// <param name="numImagesY">Number of images in vertical dimension</param>
/// <param name="filenames">List of full paths to single images</param>
/// <param name="output">Full path to save composite image including filename and extension; if set to 0 won't save composite</param>
void createCompositeImageFromDisk(const size_t numImagesX, const size_t numImagesY, char** const filenames, const char* const output) {
    const size_t totalNumImages = numImagesX * numImagesY;
    IImageCollector* imageCollector = new SavedImageCollector(filenames, totalNumImages);
    vector<Image> images = imageCollector->GetImages();
    ImageStitcher imageStitcher(images);
    const uint32_t imageWidth = images[0].m_sizeX;
    const uint32_t imageHeight = images[0].m_sizeY;
    Image IMG(((size_t)imageWidth) * imageHeight * totalNumImages);
    imageStitcher.GetStitchedImage(numImagesX, numImagesY, IMG);
    ImagePipeline pipeline;
    pipeline.addSingleImageTransformation(ImagePipelineHelper::blendEdges(imageWidth, imageHeight, min(imageWidth, imageHeight) / 2));
    pipeline.performTransformations(IMG);
    if(output) IMG.saveImage(output, ImageFileFormat_Bmp);
    IMG.display();
    IMG.releaseBuffer();
    delete imageCollector;
}

/// <summary>
/// Incomplete function for testing cobot; model like functions above
/// </summary>
void testCobot(const size_t numImagesX, const size_t numImagesY, const char* const singleImageFilename, const char* const output) {
    CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
    camera.Open();
   
    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();
    camera.ExposureMode.SetValue(ExposureMode_Timed);
    camera.ExposureAuto.SetValue(ExposureAuto_Off);
    //camera.ExposureTimeRaw.SetValue(89950);
    //camera.ExposureTimeRaw.SetValue(27000);
    //camera.ExposureTimeRaw.SetValue(150375);
    //camera.ExposureTimeRaw.SetValue(7500);
    //camera.ExposureTimeRaw.SetValue(2500);
    camera.ExposureTimeRaw.SetValue(350000);

    const size_t totalNumImages = numImagesX * numImagesY;
    IImageCollector* imageCollector = new CobotImageCollector(camera, totalNumImages);
    vector<Image> images = imageCollector->GetImages();
    ImageStitcher imageStitcher(images);
    const uint32_t imageWidth = images[0].m_sizeX;
    const uint32_t imageHeight = images[0].m_sizeY;
    if (singleImageFilename) imageStitcher.saveAllImages(singleImageFilename, ImageFileFormat_Bmp);
    Image IMG((size_t)camera.PayloadSize.GetValue() * totalNumImages);
    imageStitcher.GetStitchedImage(numImagesX, numImagesY, IMG);
    ImagePipeline pipeline;
    pipeline.addSingleImageTransformation(ImagePipelineHelper::blendEdges(imageWidth, imageHeight, min(imageWidth, imageHeight) / 2));
    pipeline.performTransformations(IMG);
    if (output) IMG.saveImage(output, ImageFileFormat_Bmp);
    IMG.display();
    IMG.releaseBuffer();
    delete imageCollector;
}

/// <summary>
/// Algorithm for layering two images
/// </summary>
/// <param name="i1">Image 1</param>
/// <param name="i2">Image 2</param>
/// <returns>An image that is the result of the layering algorithm</returns>
Image layer(Image i1, Image i2) {
    //checks for equality
    if (i1.m_sizeX != i2.m_sizeX)
        throw "X size different";

    if (i1.m_sizeY != i2.m_sizeY)
        throw "Y size different";

    if (i1.m_PixelType != i2.m_PixelType)
        throw "Pixel type different";

    Image result = i1;

    //calculation of pixels
    for (int i = 0; i < i1.m_sizeX * i2.m_sizeY; i++) 
        result.m_Buffer[i] = abs(i1.m_Buffer[i] - i2.m_Buffer[i]);
    
    result.brighten(0.25);

    return result;
}

int main(int argc, char* argv[])
{
    int exitCode = 0;

    PylonInitialize();
	
    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    PylonAutoInitTerm autoInitTerm;

    try
    {
        // Example usages of functions defined above
        //testCobot();
        //createCompositeImage(1, 1, 0, 0);
        //calibrateVerticalShift("C:\\Users\\netid\\Downloads\\cylinder_head16\\single_image_laser2000.bmp", "C:\\Users\\netid\\Downloads\\cylinder_head16\\interesting_image.bmp");
        //createCompositeImage(7, 3, "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\composite.bmp");
        //createCompositeImageFromDisk(7, 3, new char* [] {"C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\000.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\001.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\002.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\003.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\004.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\005.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\006.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\007.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\008.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\009.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\010.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\011.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\012.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\013.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\014.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\015.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\016.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\017.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\018.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\019.bmp", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\020.bmp", }, "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\composite.bmp");
    
        Image image1 =  Image::loadImage("C:\\Users\\xavier02\\Desktop\\images slide deck\\dome + Q1.bmp");
        Image image2 =  Image::loadImage("C:\\Users\\xavier02\\Desktop\\images slide deck\\dome + Q2.bmp");
        Image image3 =  Image::loadImage("C:\\Users\\xavier02\\Desktop\\images slide deck\\dome + Q3.bmp");
        Image image4 =  Image::loadImage("C:\\Users\\xavier02\\Desktop\\images slide deck\\dome + Q4.bmp");
        Image image5 =  Image::loadImage("C:\\Users\\xavier02\\Desktop\\images slide deck\\dome + ring light.bmp");

        layer(layer(image1, image4), layer(image2, image3)).display();
    }
    catch (GenICam::GenericException &e)
    {
        cerr << "An exception occurred." << endl << e.GetDescription() << endl << e.GetSourceFileName() << ":" << e.GetSourceLine() << endl;
        exitCode = 1;
    }

    cin.get();

    PylonTerminate();

    return exitCode; 
}