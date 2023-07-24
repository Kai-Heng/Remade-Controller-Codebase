#pragma once
#include <pylon/PylonIncludes.h>
#include <pylon/PylonGUI.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"

using namespace Pylon;

/// <summary>
/// Interface for collecting images
/// </summary>
interface IImageCollector {
public:
    /// <summary>
    /// Collects images
    /// </summary>
    /// <returns>List of images collected</returns>
    virtual vector<Image> GetImages() = 0;
};

/// <summary>
/// Collects images from camera
/// </summary>
class CaptureImageCollector : public IImageCollector {
protected:
    /// <summary>
    /// Triggers image capture
    /// </summary>
    /// <param name="camera">Camera to trigger</param>
    virtual void TriggerCapture(CBaslerUniversalInstantCamera& camera) {}

public:
    unsigned int MaxNumBuffer = 20;
    unsigned int CaptureTimeout = 5000;

    /// <summary>
    /// Creates new image collector using camera
    /// </summary>
    /// <param name="camera">Camera to use to capture images</param>
    /// <param name="numImages">Number of images to capture</param>
    CaptureImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages);

    /// <summary>
    /// Collects images using camera
    /// </summary>
    /// <returns>List of captured images</returns>
    vector<Image> GetImages() override;

    /// <summary>
    /// Returns camera used to capture images
    /// </summary>
    /// <returns>Camera used to capture images</returns>
    CBaslerUniversalInstantCamera& GetCamera() const;

protected:
    CBaslerUniversalInstantCamera& camera;
    const size_t numImages;
};

/// <summary>
/// Collects images from camera at specified interval
/// </summary>
class TimedCaptureImageCollector : public CaptureImageCollector {
public:
    /// <summary>
    /// Creates timed image collector
    /// </summary>
    /// <param name="numImages">Number of images to capture</param>
    /// <param name="secondsBetweenImageCaptures">Delay between image captures</param>
    TimedCaptureImageCollector(const size_t numImages, const double secondsBetweenImageCaptures);

    /// <summary>
    /// Creates timed image collector
    /// </summary>
    /// <param name="camera">Camera to use to capture images</param>
    /// <param name="numImages">Number of images to capture</param>
    /// <param name="secondsBetweenImageCaptures">Delay between image captures</param>
    TimedCaptureImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages, const double secondsBetweenImageCaptures);

    ~TimedCaptureImageCollector();

private:
    const double secondsBetweenImageCaptures;
    const bool initialAcquisitionFrameRateEnable;
    const double initialAcquisitionFrameRateAbs;
};

/// <summary>
/// Collects images from camera when manually triggered
/// </summary>
class ManualCaptureImageCollector : public CaptureImageCollector {
public:
    /// <summary>
    /// Creates manual image collector
    /// </summary>
    /// <param name="numImages">Number of images to capture</param>
    ManualCaptureImageCollector(const size_t numImages);

    /// <summary>
    /// Creates manual image collector
    /// </summary>
    /// <param name="camera">Camera to use to capture images</param>
    /// <param name="numImages">Number of images to capture</param>
    ManualCaptureImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages);

    ~ManualCaptureImageCollector();

protected:
    /// <summary>
    /// Triggers image capture when enter key pressed in console window
    /// </summary>
    /// <param name="camera">Camera to trigger</param>
    void TriggerCapture(CBaslerUniversalInstantCamera& camera) override;

private:
    const TriggerSelectorEnums initialTriggerSelector;
    const TriggerModeEnums initialTriggerMode;
    const TriggerSourceEnums initialTriggerSource;
};

/// <summary>
/// Collects images from disk
/// </summary>
class SavedImageCollector : public IImageCollector {
public:
    /// <summary>
    /// Creates new saved image collector
    /// </summary>
    /// <param name="filenames">List of images to be loaded from disk</param>
    /// <param name="numImages">Number of images to be loaded from disk</param>
    SavedImageCollector(char** const filenames, const unsigned int numImages);

    /// <summary>
    /// Loads images from disk
    /// </summary>
    /// <returns>List of images loaded from disk</returns>
    vector<Image> GetImages() override;

private:
    char** const filenames;
    const unsigned int numImages;
};

/// <summary>
/// Collects images using the cobot
/// </summary>
class CobotImageCollector : public CaptureImageCollector {
public:
    /// <summary>
    /// Creates new image collector for cobot
    /// </summary>
    /// <param name="camera">Camera to use to capture images</param>
    /// <param name="numImages">Number of images to capture</param>
    CobotImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages);

    vector<Image> GetImages() override;

    ~CobotImageCollector();

private:
    const TriggerSelectorEnums initialTriggerSelector;
    const TriggerModeEnums initialTriggerMode;
    const TriggerSourceEnums initialTriggerSource;
};
