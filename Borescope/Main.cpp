#include <pylon/PylonIncludes.h>
//#include <string>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"
#include "ImageCollector.h"
#include "ImagePipeline.h"
#include <map>
#include <vector>
#include <opencv2/opencv.hpp>
#include <sstream>


using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;

const string currentDateTime() {
    time_t      now = time(0);
    struct tm   tstruct;
    char        buf[200];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y_%m_%d_%I_%M_%S", &tstruct);

    return buf;
}

std::vector<string> unwrappedImageLoc;



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
    camera.ExposureTimeRaw.SetValue(250000);
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
    camera.Close();
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
    camera.ExposureTimeRaw.SetValue(550000);

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

cv::Mat translateImg(cv::Mat& img, int offsetx, int offsety) {
    cv::Mat trans_mat = (cv::Mat_<double>(2, 3) << 1, 0, offsetx, 0, 1, offsety);
    cv::warpAffine(img, img, trans_mat, img.size());
    return img;
}

/**
* @author Kai Heng Gan
*
* Unwrap a circular image to strip image with OpenCV
*/
void openCVUnwrap(string filePath) {
    cv::Mat circleImage = cv::imread(filePath, cv::IMREAD_COLOR);
    
    if (circleImage.empty()) {
        cerr << "Error: Could not load image." << endl;
    }

    int width = circleImage.cols-160;
    int height = circleImage.rows - 200;


    int stripHeight = height;
    int stripWidth = width;

    cv::Mat stripImage = cv::Mat::zeros(stripHeight, stripWidth, CV_8UC3);

    for (int x = 0; x < stripHeight; x++) {
        for (int y = 0; y < stripWidth; y++) {
            double angle = static_cast<double>(y) / stripWidth * 2 * CV_PI;
            double normalizedX = static_cast<double>(x) / stripHeight;
            int radius = static_cast<int>((normalizedX - 0.5) * height * 0.8 + height / 2);

            int circleY = static_cast<int> (radius * cos(angle) + width / 2);
            int circleX = static_cast<int> (radius * sin(angle) + height / 2);

            if (circleY >= 0 && circleY < width && circleX >= 0 && circleX < height) {
                stripImage.at<cv::Vec3b>(x, y) = circleImage.at<cv::Vec3b>(circleX, circleY);
            }
        }
    }

    cv::Rect roi(0, 0, width, 650);
    cv::Mat cropped = stripImage(roi);

    //Convert 24 bit depth image to 8 bit depth image
    cv::Mat gray;
    cv::cvtColor(cropped, gray, cv::COLOR_BGR2GRAY);

    string unwrappedPath = "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\unwrap\\" + currentDateTime() + ".bmp";
    unwrappedImageLoc.push_back(unwrappedPath);
    
    cv::imwrite(unwrappedPath, gray);

    cv::waitKey(0);
}

//char temp;

Image capturedImage(CBaslerUniversalInstantCamera& camera) {
    camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
    camera.TriggerMode.SetValue(TriggerMode_On);
    camera.TriggerSource.SetValue(TriggerSource_Software);

    //cout << "Press any key on the keyboard to capture | 'q' to quit \n";
    //cin >> temp;
    camera.StartGrabbing(1);

    while (camera.IsGrabbing())
    {
        camera.TriggerSoftware.Execute();
        CGrabResultPtr ptrGrabResult;
        camera.RetrieveResult(60000, ptrGrabResult);

        if (ptrGrabResult->GrabSucceeded()) {
            return Image::from(ptrGrabResult);
        }
        else cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
    }

    return NULL;
}



void captureImage() {
    CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
        camera.Open();

        camera.Width.SetToMaximum();
        camera.Height.SetToMaximum();
        camera.ExposureMode.SetValue(ExposureMode_Timed);
        camera.ExposureAuto.SetValue(ExposureAuto_Off);
        /*camera.ExposureTimeRaw.SetValue(699000);*/
        camera.ExposureTimeRaw.SetValue(700000);
        camera.GainRaw.SetValue(50);
        //camera.BlackLevelRaw.SetValue(504);

        string path = "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\original\\" + currentDateTime() + ".bmp";
        capturedImage(camera).saveImage(path.c_str(), ImageFileFormat_Bmp);

        camera.Close();


        openCVUnwrap(path.c_str());


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

        while (true) {
            int command;
            cout << "Options:\n1. Capture Image\n2. Compose Image from Disk\n3. Unwrap\n4. Release Buffer\n";
            cin >> command;


            if (command == 1) {
                captureImage();
            }
            else if (command == 2) {
                char character;
                string label;
                //cout << "Alphabet: ";
                //cin >> character;
                cout << "Label: ";
                cin >> label;

                ostringstream oss;

                oss << "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\composite\\composite" << label << ".bmp";
                //string src1 = oss1.str();
                //string src2 = oss2.str();
                //string src3 = oss3.str();
                //string out = oss4.str();

                //char* c1 = (char*) src1.c_str();
                //char* c2 = (char*) src2.c_str();
                //char* c3 = (char*) src3.c_str();

                string out = oss.str();
                if (unwrappedImageLoc.size() == 2) {
                    string src1 = unwrappedImageLoc.at(1);
                    string src2 = unwrappedImageLoc.at(0);

                    char* c1 = (char*)src1.c_str();
                    char* c2 = (char*)src2.c_str();


                    createCompositeImageFromDisk(1, 2, new char* [] {c1, c2}, out.c_str());
                }

                else if (unwrappedImageLoc.size() == 3) {
                    string src1 = unwrappedImageLoc.at(2);
                    string src2 = unwrappedImageLoc.at(1);
                    string src3 = unwrappedImageLoc.at(0);

                    char* c1 = (char*)src1.c_str();
                    char* c2 = (char*)src2.c_str();
                    char* c3 = (char*)src3.c_str();



                    createCompositeImageFromDisk(1, 3, new char* [] {c1, c2, c3}, out.c_str());
                }
                else if (unwrappedImageLoc.size() == 4) {
                    string src1 = unwrappedImageLoc.at(3);
                    string src2 = unwrappedImageLoc.at(2);
                    string src3 = unwrappedImageLoc.at(1);
                    string src4 = unwrappedImageLoc.at(0);

                    char* c1 = (char*)src1.c_str();
                    char* c2 = (char*)src2.c_str();
                    char* c3 = (char*)src3.c_str();
                    char* c4 = (char*)src4.c_str();


                    createCompositeImageFromDisk(1, 4, new char* [] {c1, c2, c3, c4}, out.c_str());
                }

                unwrappedImageLoc.clear();
            }

            else if(command == 3){
                //unwrapImage("C:\\Users\\cceelab\\Desktop\\kaiheng9\\original\\circularImage2.bmp");
                string imageName;
                cout << "Image name: ";
                cin >> imageName;
                string fileLoc = "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\original\\" + imageName + ".bmp";
                openCVUnwrap(fileLoc);
            }

            else if (command == 4) {
                unwrappedImageLoc.clear();
            }

            else if (command == 5) {
                PylonTerminate(true);
                break;
            }

            else if (command == 6) {
                char character;
                string label;
                //cout << "Alphabet: ";
                //cin >> character;
                cout << "Label: ";
                cin >> label;

                ostringstream oss;

                oss << "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\composite\\composite" << label << ".bmp";
                //string src1 = oss1.str();
                //string src2 = oss2.str();
                //string src3 = oss3.str();
                //string out = oss4.str();

                //char* c1 = (char*) src1.c_str();
                //char* c2 = (char*) src2.c_str();
                //char* c3 = (char*) src3.c_str();

                string out = oss.str();
                createCompositeImageFromDisk(1, 3, new char* [] {"C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\unwrap\\2024_03_28_04_20_07.bmp", "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\unwrap\\2024_03_28_04_20_07.bmp", "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\unwrap\\2024_03_28_04_20_07.bmp"}, out.c_str());

            }
            else {
                continue;
            }
        }
    }

    catch (GenICam::GenericException& e)
    {
        cerr << "An exception occurred." << endl << e.GetDescription() << endl << e.GetSourceFileName() << ":" << e.GetSourceLine() << endl;
        exitCode = 1;
    }

    cin.get();

    PylonTerminate(true);

    return exitCode;

}