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

class LineScanImageCollector : public CaptureImageCollector {
public:
    LineScanImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages);

    vector<Image> GetImages() override;

    void TriggerCapture(CBaslerUniversalInstantCamera& camera) override;

    ~LineScanImageCollector();

private:
    const TriggerSelectorEnums initialTriggerSelector;
    const TriggerModeEnums initialTriggerMode;
    const TriggerSourceEnums initialTriggerSource;
};
