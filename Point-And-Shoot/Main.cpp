#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>
#include "Image.h"

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


int main(int argc, char * argv[])
{
    string aux(argv[0]);
    int pos = aux.rfind('\\');
    string path;

    try {
        PylonInitialize();

        PylonAutoInitTerm autoInitTerm;

        CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
        camera.Open();

        camera.Width.SetToMaximum();
        camera.Height.SetToMaximum();
        camera.ExposureMode.SetValue(ExposureMode_Timed);
        camera.ExposureAuto.SetValue(ExposureAuto_Off);
        camera.ExposureTimeRaw.SetValue(50000);

        /*
        camera.GevStreamChannelSelector.SetValue(GevStreamChannelSelector_StreamChannel0);
        camera.GevSCPSPacketSize.SetValue(3000);
        camera.GevStreamChannelSelector.SetValue(GevStreamChannelSelector_StreamChannel0);
        camera.GevSCPD.SetValue(1500);
        */

        string lens = "12 mm";
        bool loop = true;
        char trigger = 'k';

        while (loop) {

            path = aux.substr(0, pos + 1);
            path += "image\\";
            path += "i";
            path += currentDateTime();
            path += ".bmp";

            cout << "Path: " << path << endl;
            cout << "1. lens" << endl;
            cout << "2. exposure" << endl;
            cout << "3. trigger type" << endl;
            cout << "4. capture" << endl;
            cout << "9. exit" << endl;
            char in;
            cin >> in;
            switch (in) {
            case '1': {
            LENS:
                cout << "Selected lens: " << lens << endl;
                cout << "Recomandded working distane: 10cm" << endl;
                cout << "c to change lens, anything else to exit" << endl;
                cout << "Input: ";
                string temp;
                cin >> temp;
                if (temp == "c") {
                    if (lens == "12 mm") {
                        lens = "8 mm";
                        camera.ExposureTimeRaw.SetValue(350000);
                    }
                    else if (lens == "8 mm") {
                        lens = "12 mm";
                        camera.ExposureTimeRaw.SetValue(50000);
                    }
                    goto LENS;
                }
                break;
            }

            case '2': {
                cout << "Current exposure value: " << camera.ExposureTimeRaw.GetValue() << endl;
                int temp;
                cout << "Enter new exposure value: ";
                cin >> temp;
                camera.ExposureTimeRaw.SetValue(temp);
                break;
            }

            case '3': {
            TRIGGER:
                cout << "Current trigger mode: ";
                if (trigger == 'k') cout << "keyboard" << endl;
                else if (trigger == 'd') cout << "deadman switch" << endl;
                cout << "c to change trigger mode, anything else to exit" << endl;
                cout << "Input: ";
                string temp;
                cin >> temp;
                if (temp == "c") {
                    if (trigger == 'k') trigger = 'd';
                    else if (trigger == 'd') trigger = 'k';
                    goto TRIGGER;
                }
                break;
            }

            case '4': {
                Image i(0);
                if (trigger == 'k') captureImageKeyboard(camera).saveImage(path.c_str(), ImageFileFormat_Bmp);
                else if (trigger == 'd') captureImageDeadman(camera).saveImage(path.c_str(), ImageFileFormat_Bmp);

                break;
            }

            case '9': {
                loop = false;
                break;
            }

            default: {
                cout << "Please enter 1, 2, 3, 4, or 9\n";
            }
            }

        }


    }
    catch (GenICam::GenericException& e)
    {
        cerr << "An exception occurred." << endl << e.GetDescription() << endl << e.GetSourceFileName() << ":" << e.GetSourceLine() << endl;
        cout << "Press any button to exit" << endl << "Input: ";
        string s;
        cin >> s;
    }

    PylonTerminate();

    return 1;
}