#include <iostream>
#include "ImageCollector.h"

using namespace std;

ManualCaptureImageCollector::ManualCaptureImageCollector(const size_t numImages) :
    ManualCaptureImageCollector(*new CBaslerUniversalInstantCamera(CTlFactory::GetInstance().CreateFirstDevice()), numImages)
{}

ManualCaptureImageCollector::ManualCaptureImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages) :
    CaptureImageCollector(camera, numImages),
    initialTriggerSelector(camera.TriggerSelector.GetValue()),
    initialTriggerMode(camera.TriggerMode.GetValue()),
    initialTriggerSource(camera.TriggerSource.GetValue())
{
    camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
    camera.TriggerMode.SetValue(TriggerMode_On);
    camera.TriggerSource.SetValue(TriggerSource_Software);
}

ManualCaptureImageCollector::~ManualCaptureImageCollector() {
    camera.TriggerSelector.SetValue(initialTriggerSelector);
    camera.TriggerMode.SetValue(initialTriggerMode);
    camera.TriggerSource.SetValue(initialTriggerSource);
}

void ManualCaptureImageCollector::TriggerCapture(CBaslerUniversalInstantCamera& camera) {
    cout << "Ready" << endl;
    cin.get();
    cout << "Capturing..." << endl;
    camera.TriggerSoftware.Execute();
}