#include "ImageCollector.h"

TimedCaptureImageCollector::TimedCaptureImageCollector(const size_t numImages, const double secondsBetweenImageCaptures) :
    TimedCaptureImageCollector(*new CBaslerUniversalInstantCamera(CTlFactory::GetInstance().CreateFirstDevice()), numImages, secondsBetweenImageCaptures)
{}

TimedCaptureImageCollector::TimedCaptureImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages, const double secondsBetweenImageCaptures) :
    CaptureImageCollector(camera, numImages),
    secondsBetweenImageCaptures(secondsBetweenImageCaptures),
    initialAcquisitionFrameRateEnable(camera.AcquisitionFrameRateEnable.GetValue()),
    initialAcquisitionFrameRateAbs(camera.AcquisitionFrameRateAbs.GetValue())
{
    CaptureTimeout = (unsigned int)(secondsBetweenImageCaptures * 1000 + 1000);
    camera.AcquisitionFrameRateEnable.SetValue(true);
    camera.AcquisitionFrameRateAbs.SetValue(1 / secondsBetweenImageCaptures);
}

TimedCaptureImageCollector::~TimedCaptureImageCollector() {
    camera.AcquisitionFrameRateEnable.SetValue(initialAcquisitionFrameRateEnable);
    camera.AcquisitionFrameRateAbs.SetValue(initialAcquisitionFrameRateAbs);
}
