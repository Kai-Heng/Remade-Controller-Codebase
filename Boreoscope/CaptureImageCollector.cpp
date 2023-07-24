#include "ImageCollector.h"

CaptureImageCollector::CaptureImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages) :
    camera(camera),
    numImages(numImages)
{}

vector<Image> CaptureImageCollector::GetImages() {
    vector<Image> images;
    camera.MaxNumBuffer = MaxNumBuffer;

    camera.StartGrabbing(numImages);

    while(camera.IsGrabbing())
    {
        TriggerCapture(camera);
        CGrabResultPtr ptrGrabResult;
        camera.RetrieveResult(CaptureTimeout, ptrGrabResult);

        if (ptrGrabResult->GrabSucceeded()) images.push_back(Image::from(ptrGrabResult));
        else cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
    }

    return images;
}

CBaslerUniversalInstantCamera& CaptureImageCollector::GetCamera() const {
    return camera;
}