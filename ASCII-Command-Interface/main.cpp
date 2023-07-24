#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define PORT 777

#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <cstdint>
#include <fstream>
#include <time.h>
#include <array>
#include <sstream>
#include <conio.h>
#include <windows.h>

#include "ConcurrentSystem.h"
#include "Image.h"
#include "xmltagging.h"

using namespace std;

/// <summary>
/// These constants are used to identify the device when we initialize a socket connection between the program and the controller.
/// </summary>
const char* serverIp = "192.168.0.5";
int port = 777;

int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
//Client side
int main(int argc, char* argv[])
{
    //Check for images directory
    checkForImageDirectory();

    string aux(argv[0]);
    int pos = aux.rfind('\\');
    string path = aux.substr(0, pos + 1);
    path += "image\\";
    path += "i";
    path += currentDateTime();
    path += ".bmp";

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

        WSADATA wsaData;
        int iResult;

        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
         if (iResult != 0) {
             printf("WSAStartup failed: %d\n", iResult);
               return 1;
         }

        //create a message buffer 
        char msg[1500];
        //setup a socket and connection tools 
        
        struct hostent* host = gethostbyname(serverIp);
        sockaddr_in sendSockAddr;
        memset((char*)&sendSockAddr, 0, sizeof(sendSockAddr));
        sendSockAddr.sin_family = AF_INET;

        //memory access issue
        sendSockAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));

        sendSockAddr.sin_port = htons(port);
        int clientSd = socket(AF_INET, SOCK_STREAM, 0);
        //try to connect...
        int status = connect(clientSd, (sockaddr*)&sendSockAddr, sizeof(sendSockAddr));
        if (status < 0)
        {
            cout << "Error connecting to socket!" << endl;
            return -1;
        }
        cout << "Connected to the server!" << endl;
        int bytesRead = 0;
        int bytesWritten = 0;
        struct timeval start1, end1;
        gettimeofday(&start1, NULL);
        while (1)
        {
            cout << "----------- Commands ----------" << endl;
            cout << "-- Action Commands --\n";
            cout << "1. RUN:SEQUENCE (Photo by photo);\n";
            cout << "2. RUN:SEQUENCE (Full quadrant-cluster capture);\n";

            cout << "-- Read Commands --\n";
            cout << "9. Exit\n";

            cout << ">";
            int commands;
            cin >> commands;
            string data;
            string channel;
            string nnnn;

            if (commands == 1 || commands == 2) {
                data = "RUN:SEQUENCE;";
            }
            else {
                data = "exit";
            }
            memset(&msg, 0, sizeof(msg));//clear the buffer
            strcpy(msg, data.c_str());
            if (data == "exit")
            {
                send(clientSd, (char*)&msg, strlen(msg), 0);
                PylonTerminate();
                break;
            }

            // Logic for the first choice from above
            if (commands == 1) {
                cout << "Starting camera shuttering sequence...\n";

                path = getImagePath(aux);

                camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                camera.TriggerMode.SetValue(TriggerMode_On);
                camera.TriggerSource.SetValue(TriggerSource_Software);

                camera.StartGrabbing(1);
                bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);
                while (camera.IsGrabbing())
                {
                    camera.TriggerSoftware.Execute();
                    CGrabResultPtr ptrGrabResult;
                    camera.RetrieveResult(60000, ptrGrabResult);

                    if (ptrGrabResult->GrabSucceeded()) {
                        Image::from(ptrGrabResult).saveImage(path.c_str(), ImageFileFormat_Bmp);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        PylonTerminate();
                        exit(1);

                    }

                    cout << "Camera shuttering terminated..." << endl;
                }
            }
            // Logic for the second case from above...
            else if (commands == 2) {
                cout << "Capturing channel 1...\n";

                path = getImagePath(aux);

                camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                camera.TriggerMode.SetValue(TriggerMode_On);
                camera.TriggerSource.SetValue(TriggerSource_Software);

                camera.StartGrabbing(1);
                bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);

                while (camera.IsGrabbing())
                {
                    camera.TriggerSoftware.Execute();
                    CGrabResultPtr ptrGrabResult;
                    camera.RetrieveResult(60000, ptrGrabResult);

                    if (ptrGrabResult->GrabSucceeded()) {
                        Image::from(ptrGrabResult).saveImage(path.c_str(), ImageFileFormat_Bmp);
                        std::ofstream file(("image/" + currentDateTime() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", "1");
                        close_xml_file(file);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        PylonTerminate();
                        exit(1);

                    }

                    cout << "Camera shuttering terminated..." << endl;
                }

                cout << "Capturing channel 2...\n";

                path = getImagePath(aux);

                camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                camera.TriggerMode.SetValue(TriggerMode_On);
                camera.TriggerSource.SetValue(TriggerSource_Software);

                camera.StartGrabbing(1);
                bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);

                while (camera.IsGrabbing())
                {
                    camera.TriggerSoftware.Execute();
                    CGrabResultPtr ptrGrabResult;
                    camera.RetrieveResult(60000, ptrGrabResult);

                    if (ptrGrabResult->GrabSucceeded()) {
                        Image::from(ptrGrabResult).saveImage(path.c_str(), ImageFileFormat_Bmp);
                        std::ofstream file(("image/" + currentDateTime() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", "2");
                        close_xml_file(file);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        PylonTerminate();
                        exit(1);

                    }

                    cout << "Camera shuttering terminated..." << endl;
                }

                cout << "Capturing channel 3...\n";

                path = getImagePath(aux);

                camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                camera.TriggerMode.SetValue(TriggerMode_On);
                camera.TriggerSource.SetValue(TriggerSource_Software);

                camera.StartGrabbing(1);
                bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);

                while (camera.IsGrabbing())
                {
                    camera.TriggerSoftware.Execute();
                    CGrabResultPtr ptrGrabResult;
                    camera.RetrieveResult(60000, ptrGrabResult);

                    if (ptrGrabResult->GrabSucceeded()) {
                        Image::from(ptrGrabResult).saveImage(path.c_str(), ImageFileFormat_Bmp);
                        std::ofstream file(("image/" + currentDateTime() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", "3");
                        close_xml_file(file);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        PylonTerminate();
                        exit(1);

                    }

                    cout << "Camera shuttering terminated..." << endl;
                }

                cout << "Capturing channel 4...\n";

                path = getImagePath(aux);

                camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                camera.TriggerMode.SetValue(TriggerMode_On);
                camera.TriggerSource.SetValue(TriggerSource_Software);

                camera.StartGrabbing(1);
                bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);

                while (camera.IsGrabbing())
                {
                    camera.TriggerSoftware.Execute();
                    CGrabResultPtr ptrGrabResult;
                    camera.RetrieveResult(60000, ptrGrabResult);

                    if (ptrGrabResult->GrabSucceeded()) {
                        Image::from(ptrGrabResult).saveImage(path.c_str(), ImageFileFormat_Bmp);
                        std::ofstream file(("image/" + currentDateTime() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", "4");
                        close_xml_file(file);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        PylonTerminate();
                        exit(1);

                    }

                    cout << "Camera shuttering terminated..." << endl;
                }
            }

                if (commands != 12) {
                    cout << "Awaiting server response..." << endl;
                    memset(&msg, 0, sizeof(msg));//clear the buffer
                    bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0);
                    if (!strcmp(msg, "exit"))
                    {
                        cout << "Server has quit the session" << endl;
                        break;
                    }
                    cout << "Server: " << msg << endl;
                }
           }

        gettimeofday(&end1, NULL);
        closesocket(clientSd);
        cout << "********Session********" << endl;
        cout << "Bytes written: " << bytesWritten <<
        " Bytes read: " << bytesRead << endl;
        cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec)
            << " secs" << endl;
        cout << "Connection closed" << endl;

        }
        catch (GenICam::GenericException& e)
            {
                cerr << "An exception occurred." << endl << e.GetDescription() << endl << e.GetSourceFileName()
                    << ":" << e.GetSourceLine() << endl;
                cout << "Press any button to exit" << endl << "Input: ";
                string s;
                cin >> s;
                exit(1);
            }

        WSACleanup();
        return 0;
        }