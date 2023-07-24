#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"
#include "ImageCollector.h"
#include "ImagePipeline.h"

using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

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
/// Incomplete function for testing line scan camera; model like functions in the area scan camera program
/// </summary>
void testLineScan(const size_t numImagesX, const size_t numImagesY, const char* const singleImageFilename, const char* const output) {
    CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
	camera.Open();

	camera.GevStreamChannelSelector.SetValue(GevStreamChannelSelector_StreamChannel0);
	camera.GevSCPSPacketSize.SetValue(3000);
	camera.GevSCPD.SetValue(1500);

	camera.Width.SetToMaximum();
	camera.Height.SetValue(2048);

	camera.AcquisitionLineRateAbs.SetValue(30000);
	camera.ExposureMode.SetValue(ExposureMode_Timed);
	camera.ExposureAuto.SetValue(ExposureAuto_Off);
	camera.ExposureTimeRaw.SetValue(25000);
	camera.GainRaw.SetValue(256);

	const size_t totalNumImages = numImagesX * numImagesY;
	vector<Image> stripes;

	for (int i = 0; i < numImages X; i++){
		IImageCollector* imageCollector = new LineScanImageCollector(camera, numImagesY);
		vector<Image> images = imageCollector->GetImages();
		ImageStitcher imageStitcher(images);
		const uint32_t imageWidth = images[0].m_sizeX;
		const uint32_t imageHeight = images[0].m_sizeY;
		Image IMG((size_t)camera.PayloadSize.GetValue() * numImagesY);
		imageStitcher.GetStitchedImage(1, numImagesY, IMG);
		ImagePipeline pipeline;
		pipeline.addSingleImageTransformation(ImagePipelineHelper::blendEdges(imageWidth, imageHeight, min(imageWidth, imageHeight) / 2));
		pipeline.performTransformations(IMG);
		IMG.display();
		stripes.push_back(IMG);
		IMG.releaseBuffer();
		delete imageCollector;
		cin.get();
	}

	ImageStitcher imageStitcher(stripes);
	const uint32_t imageWidth = stripes[0].m_sizeX;
	const uint32_t imageHeight = stripes[0].m_sizeY;
	if (singleImageFilename) imageStitcher.saveAllImages(singleImageFilename, ImageFileFormat_Bmp);
	Image IMG((size_t)camera.PayloadSize.GetValue() * totalNumImages);
	imageStitcher.GetStitchedImage(numImagesX, 1, IMG);
	ImagePipeline pipeline;
	pipeline.addSingleImageTransformation(ImagePipelineHelper::blendEdges(imageWidth, imageHeight, min(imageWidth, imageHeight) / 2));
	pipeline.performTransformations(IMG);
	if (output) IMG.saveImage(output, ImageFileFormat_Bmp);
	IMG.display();
	IMG.releaseBuffer();
	delete imageCollector;
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
        //testLineScan(7, 3, "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\", "C:\\Users\\xavier02\\Desktop\\head 8 Exp 500,000 f4 10cm no lights\\composite.bmp");

         testLineScan(1, 10, 0, 0);
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
