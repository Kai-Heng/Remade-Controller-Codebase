#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>
#include <ctime>
#include <experimental/filesystem> // Still skeptical of this and the #define above. Look into possibly fixing this in the future.

#include "ConcurrentSystem.h"
#include "Image.h"

namespace fs = std::experimental::filesystem;

const std::string currentDateTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[200];

    localtime_s(&tstruct, &now);
    strftime(buf, sizeof(buf), "%Y_%m_%d_%H_%M_%S", &tstruct);

    return buf;
}

Image captureImageDeadman(CBaslerUniversalInstantCamera& camera)
{
    camera.LineSelector.SetValue(LineSelector_Line1);
    camera.LineMode.SetValue(LineMode_Input);
    camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
    camera.TriggerMode.SetValue(TriggerMode_On);
    camera.TriggerSource.SetValue(TriggerSource_Line1);
    camera.TriggerActivation.SetValue(TriggerActivation_RisingEdge);

    camera.StartGrabbing(1);

    while (camera.IsGrabbing())
    {
        cout << "Waiting for Trigger" << endl;
        CGrabResultPtr ptrGrabResult;
        camera.RetrieveResult(60000, ptrGrabResult);

        if (ptrGrabResult->GrabSucceeded()) {
            return Image::from(ptrGrabResult);
        }
        else cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
    }

    return NULL;
}

Image captureImageKeyboard(CBaslerUniversalInstantCamera& camera)
{
    camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
    camera.TriggerMode.SetValue(TriggerMode_On);
    camera.TriggerSource.SetValue(TriggerSource_Software);

    camera.StartGrabbing(1);

    while (camera.IsGrabbing())
    {
        char temp;
        cout << "Press any key on the keyboard to capture\n";
        cin >> temp;
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

void checkForImageDirectory(std::string startTime) {
    std::string folderName = startTime;
    fs::path currentPath = fs::current_path();
    fs::path folderPath = currentPath / folderName;

    if (fs::exists(folderPath) && fs::is_directory(folderPath)) {
        std::cout << "Folder '" << folderName << "' already exists." << std::endl;
    }
    else {
        if (fs::create_directory(folderPath)) {
            std::cout << "Folder '" << folderName << "' created successfully." << std::endl;
        }
        else {
            std::cerr << "Failed to create folder '" << folderName;
            return;
        }
    }
}

std::string getImagePath(std::string aux) {

    int pos = aux.rfind('\\');
    std::string path = aux.substr(0, pos + 1);
    path += "image\\";
    path += "i";
    path += currentDateTime();
    path += ".bmp";

    return path;

}