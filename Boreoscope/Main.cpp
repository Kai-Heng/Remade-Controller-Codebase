#include <pylon/PylonIncludes.h>
//#include <string>
#include <OECore.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"
#include "ImageCollector.h"
#include "ImagePipeline.h"
#include "ICircleUnwrapper.h"
#include "IEllipseUnwrapper.h"
#include <map>
#include <vector>
#include <opencv2/opencv.hpp>
#include <sstream>


using namespace Pylon;
using namespace std;
using namespace Basler_UniversalCameraParams;
using namespace OELib;

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

/**
* 360Lib Suite Unwrap function
*/

//void unwrapImage(string imagePath) {
//    EllipseUnwrapper eUnw = IEllipseUnwrapper::Create();
//
//    ImagePtr srcImage = ImageFactory::load(imagePath);
//    ImagePtr outImage;
//
//    ////Recipe 1
//    //// First Elipse points
//    //Point p1 = Point(1345, 843);
//    //Point p2 = Point(1609, 795);
//    //Point p3 = Point(1719, 977);
//    //Point p4 = Point(1577, 1185);
//    //Point p5 = Point(1325, 1107);
//    //Point p6 = Point(1289, 977);
//
//    //vector<Point> firstElipse;
//
//    //firstElipse.push_back(p1);
//    //firstElipse.push_back(p2);
//    //firstElipse.push_back(p3);
//    //firstElipse.push_back(p4);
//    //firstElipse.push_back(p5);
//    //firstElipse.push_back(p6);
//
//
//    //// Second Elipse Points
//    //Point p21 = Point(1007, 477);
//    //Point p22 = Point(819, 807);
//    //Point p23 = Point(1087, 1551);
//    //Point p24 = Point(1593, 1691);
//    //Point p25 = Point(2143, 1261);
//    //Point p26 = Point(2203, 931);
//    //Point p27 = Point(1976, 451);
//
//    //vector<Point> secondElipse;
//
//    //secondElipse.push_back(p21);
//    //secondElipse.push_back(p22);
//    //secondElipse.push_back(p23);
//    //secondElipse.push_back(p24);
//    //secondElipse.push_back(p25);
//    //secondElipse.push_back(p26);
//    //secondElipse.push_back(p27);
//
//    //Circle searchArea = Circle(1495, 982, 144);
//    
//    /*
//    //Recipe 2
//    // First Elipse points
//    Point p1 = Point(1295, 1010);
//    Point p2 = Point(1423, 802);
//    Point p3 = Point(1563, 780);
//    Point p4 = Point(1444, 1192);
//    Point p5 = Point(1692, 1119);
//    Point p6 = Point(1740, 989);
//
//    vector<Point> firstElipse;
//
//    firstElipse.push_back(p1);
//    firstElipse.push_back(p2);
//    firstElipse.push_back(p3);
//    firstElipse.push_back(p4);
//    firstElipse.push_back(p5);
//    firstElipse.push_back(p6);
//
//
//    // Second Elipse Points
//    Point p21 = Point(1641, 285);
//    Point p22 = Point(1113, 417);
//    Point p23 = Point(840, 741);
//    Point p24 = Point(1229, 1643);
//    Point p25 = Point(1913, 1553);
//    Point p26 = Point(2205, 1073);
//    Point p27 = Point(2137, 681);
//
//    vector<Point> secondElipse;
//
//    secondElipse.push_back(p21);
//    secondElipse.push_back(p22);
//    secondElipse.push_back(p23);
//    secondElipse.push_back(p24);
//    secondElipse.push_back(p25);
//    secondElipse.push_back(p26);
//    secondElipse.push_back(p27);
//
//    Circle searchArea = Circle(1495, 982, 144);
//    */
//
//    /*
//    //Recipe 3
//    // First Elipse points
//    Point p1 = Point(1563, 781);
//    Point p2 = Point(1639, 809);
//    Point p3 = Point(1691, 865);
//    Point p4 = Point(1733, 981);
//    Point p5 = Point(1697, 1099);
//    Point p6 = Point(1477, 1207);
//    Point p7 = Point(1293, 981);
//    Point p8 = Point(1341, 867);
//
//    vector<Point> firstElipse;
//
//    firstElipse.push_back(p1);
//    firstElipse.push_back(p2);
//    firstElipse.push_back(p3);
//    firstElipse.push_back(p4);
//    firstElipse.push_back(p5);
//    firstElipse.push_back(p6);
//    firstElipse.push_back(p7);
//    firstElipse.push_back(p8);
//
//
//    // Second Elipse Points
//    Point p21 = Point(2079, 1433);
//    Point p22 = Point(1163, 1599);
//    Point p23 = Point(1571, 1701);
//    Point p24 = Point(817, 920);
//    Point p25 = Point(1325, 315);
//    Point p26 = Point(1735, 321);
//    Point p27 = Point(2137, 627);
//    Point p28 = Point(2221, 952);
//
//    vector<Point> secondElipse;
//
//    secondElipse.push_back(p21);
//    secondElipse.push_back(p22);
//    secondElipse.push_back(p23);
//    secondElipse.push_back(p24);
//    secondElipse.push_back(p25);
//    secondElipse.push_back(p26);
//    secondElipse.push_back(p27);
//    secondElipse.push_back(p28);
//
//
//    Circle searchArea = Circle(1511, 990, 146.12);
//    */
//
//    /*
//    //Recipe 4
//    // First Elipse points
//    Point p1 = Point(1393, 813);
//    Point p2 = Point(1609, 785);
//    Point p3 = Point(1729, 941);
//    Point p4 = Point(1685, 1109);
//    Point p5 = Point(1593, 1189);
//    Point p6 = Point(1377, 1149);
//    Point p7 = Point(1305, 945);
//    Point p8 = Point(1349, 845);
//
//    vector<Point> firstElipse;
//
//    firstElipse.push_back(p1);
//    firstElipse.push_back(p2);
//    firstElipse.push_back(p3);
//    firstElipse.push_back(p4);
//    firstElipse.push_back(p5);
//    firstElipse.push_back(p6);
//    firstElipse.push_back(p7);
//    firstElipse.push_back(p8);
//
//
//    // Second Elipse Points
//    Point p21 = Point(1029, 481);
//    Point p22 = Point(1433, 285);
//    Point p23 = Point(1805, 345);
//    Point p24 = Point(2113, 597);
//    Point p25 = Point(2229, 949);
//    Point p26 = Point(2065, 1433);
//    Point p27 = Point(1877, 1601);
//    Point p28 = Point(1317, 1665);
//    Point p29 = Point(841, 1197);
//
//    vector<Point> secondElipse;
//
//    secondElipse.push_back(p21);
//    secondElipse.push_back(p22);
//    secondElipse.push_back(p23);
//    secondElipse.push_back(p24);
//    secondElipse.push_back(p25);
//    secondElipse.push_back(p26);
//    secondElipse.push_back(p27);
//    secondElipse.push_back(p28);
//    secondElipse.push_back(p29);
//
//     Circle searchArea = Circle(1511, 990, 146);
//
//    */
//
//
//
//
//
//    //Recipe 5
//    // First Elipse points
//    Point p1 = Point(1361, 837);
//    Point p2 = Point(1465, 781);
//    Point p3 = Point(1617, 785);
//    Point p4 = Point(1685, 841);
//    Point p5 = Point(1737, 961);
//    Point p6 = Point(1725, 1049);
//    Point p7 = Point(1661, 1141);
//    Point p8 = Point(1545, 1197);
//    Point p9 = Point(1433, 1185);
//    Point p10 = Point(1337, 1109);
//    Point p11 = Point(1305, 949);
//    Point p12 = Point(1337, 853);
//
//    vector<Point> firstElipse;
//
//    firstElipse.push_back(p1);
//    firstElipse.push_back(p2);
//    firstElipse.push_back(p3);
//    firstElipse.push_back(p4);
//    firstElipse.push_back(p5);
//    firstElipse.push_back(p6);
//    firstElipse.push_back(p7);
//    firstElipse.push_back(p8);
//    firstElipse.push_back(p9);
//    firstElipse.push_back(p10);
//    firstElipse.push_back(p11);
//    firstElipse.push_back(p12);
//
//
//    // Second Elipse Points
//    Point p21 = Point(1029, 477);
//    Point p22 = Point(1185, 365);
//    Point p23 = Point(1473, 281);
//    Point p24 = Point(1729, 313);
//    Point p25 = Point(1881, 381);
//    Point p26 = Point(2037, 509);
//    Point p27 = Point(2161, 697);
//    Point p28 = Point(2237, 953);
//    //Point p29 = Point(2161, 1681);
//    Point p30 = Point(1985, 1497);
//    Point p31 = Point(1657, 1681);
//    Point p32 = Point(1293, 1661);
//    Point p33 = Point(893, 1305);
//    Point p34 = Point(825, 913);
//    Point p35 = Point(925, 605);
//    Point p36 = Point(997, 521);
//
//    vector<Point> secondElipse;
//
//    secondElipse.push_back(p21);
//    secondElipse.push_back(p22);
//    secondElipse.push_back(p23);
//    secondElipse.push_back(p24);
//    secondElipse.push_back(p25);
//    secondElipse.push_back(p26);
//    secondElipse.push_back(p27);
//    secondElipse.push_back(p28);
//    //secondElipse.push_back(p29);
//    secondElipse.push_back(p30);
//    secondElipse.push_back(p31);
//    secondElipse.push_back(p32);
//    secondElipse.push_back(p33);
//    secondElipse.push_back(p34);
//    secondElipse.push_back(p35);
//    secondElipse.push_back(p36);
//
//    Circle searchArea = Circle(1511, 990, 146);
//
//    eUnw->SetAngleScanStep(0.8);
//    eUnw->SetContrastThreshold(30);
//    eUnw->SetMinScore(0.2);
//    eUnw->SetLevels(1);
//    eUnw->SetMaxKeypoints(44);
//    eUnw->SetUnwrapImgResolution(2200, 500);
//    eUnw->SetLockEllipses(true);
//    eUnw->SetFirstTemplate(srcImage, firstElipse);
//    eUnw->SetSecondTemplate(srcImage, secondElipse);
//    eUnw->SetSearchingArea(searchArea);
//
//
//    //unwrap
//    outImage = eUnw->Unwrap(srcImage);
//    string unwrapLocation = "C:\\Users\\cceelab\\Desktop\\remade-controller-codebase\\unwrap\\" + currentDateTime() + ".bmp";
//    unwrappedImageLoc.push_back(unwrapLocation);
//    outImage.get()->writeToFile(unwrapLocation.c_str());
//}

void openCVUnwrap(string filePath) {
    cv::Mat circleImage = cv::imread(filePath, cv::IMREAD_COLOR);

    if (circleImage.empty()) {
        cerr << "Error: Could not load image." << endl;
    }

    int width = circleImage.cols - 78;
    int height = circleImage.rows - 95;

    int stripHeight = height;
    int stripWidth = width;

    cv::Mat stripImage = cv::Mat::zeros(stripHeight, stripWidth, CV_8UC3);

    for (int x = 0; x < stripHeight; x++) {
        for (int y = 0; y < stripWidth; y++) {
            double angle = static_cast<double>(y) / stripWidth * CV_PI;
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