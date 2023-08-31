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
#include <csignal>
#include <conio.h>

#include "ConcurrentSystem.h"
#include "Image.h"
#include "xmltagging.h"

using namespace std;

/// <summary>
/// These constants are used to identify the device when we initialize a socket connection between the program and the controller.
/// </summary>
const char* serverIp = "192.168.0.5";
int port = 777;

char column = 'A';
int row = 1;
int channel_num = 1;
string randomInput;
string START_TIME;

/// <summary>
/// Function returns the time of day. This is currently used for naming images.
/// </summary>
/// <param name="tp"></param>
/// <param name="tzp"></param>
/// <returns></returns>
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

void signalHandler(int signal) {
    if (signal == SIGINT) {
        // Perform cleanup tasks here.
        cout << "Terminating process... Cleaning up..." << endl;
        PylonTerminate();

        Sleep(1000);

        exit(signal);
    }
}

//Client side
int main(int argc, char* argv[])
{
    // Register the signal handler for SIGINT -> Terminates process properly.
    // TODO: This method needs further investigation, not sure if anything is really happening.
    signal(SIGINT, signalHandler);

    START_TIME = currentDateTime();

    //Check for images directory
    checkForImageDirectory(START_TIME);

    string aux(argv[0]);
    int pos = aux.rfind('\\');
    string path = aux.substr(0, pos + 1);
    path += "image\\";
    path += "i";
    path += currentDateTime();
    path += ".bmp";

    // Try loop required for handling errors and warnings thrown by the Pylon Camera libraries
    try {

        PylonInitialize();

        PylonAutoInitTerm autoInitTerm;
        CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        camera.Open();

        camera.Width.SetToMaximum();
        camera.Height.SetToMaximum();
        camera.ExposureMode.SetValue(ExposureMode_Timed);
        camera.ExposureAuto.SetValue(ExposureAuto_Off);
        camera.ExposureTimeRaw.SetValue(10000); // This can be a set reference depending on implementation. Maybe add a config file that handles this?

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
            cout << "3. RUN:SEQUENCE (Entire face imaging sequence);\n";

            cout << "-- Read Commands --\n";
            cout << "9. Exit\n";

            cout << ">";
            int commands;
            cin >> commands;
            string data;
            string channel;
            string nnnn;

            if (commands < 4) {
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
                        cout << "\033[31m";
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        cout << "\033[0m";
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
                        Image::from(ptrGrabResult).saveImage((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                        std::ofstream file((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                        close_xml_file(file);
                        channel_num = iterate_channel(channel_num);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "\033[31m";
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        cout << "\033[0m";
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
                        Image::from(ptrGrabResult).saveImage((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                        std::ofstream file((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                        close_xml_file(file);
                        channel_num = iterate_channel(channel_num);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "\033[31m";
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        cout << "\033[0m";
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
                        Image::from(ptrGrabResult).saveImage((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                        std::ofstream file((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                        close_xml_file(file);
                        channel_num = iterate_channel(channel_num);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "\033[31m";
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        cout << "\033[0m";
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
                        Image::from(ptrGrabResult).saveImage((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                        std::ofstream file((START_TIME + "/" + to_string(column).c_str() + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                        init_xml_file(file);
                        write_xml_entry(file, "Lens-Type", "12mm");
                        write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                        close_xml_file(file);
                        channel_num = iterate_channel(channel_num);

                        Sleep(500);
                    }
                    else
                    {
                        cout << "\033[31m";
                        cout << "Error: " << ptrGrabResult->GetErrorCode()
                            << " " << ptrGrabResult->GetErrorDescription() << endl;
                        cout << "\033[0m";
                        PylonTerminate();
                        exit(1);

                    }

                    cout << "Camera shuttering terminated..." << endl;
                }
            }
            else if (commands == 3) {

                while (row != 19) {

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
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "12mm");
                            write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                            write_xml_entry(file, "Raw-Exposure", to_string(camera.ExposureTimeRaw.GetValue()).c_str());
                            close_xml_file(file);
                            channel_num = iterate_channel(channel_num);

                            Sleep(500);
                        }
                        else
                        {
                            cout << "\033[31m";
                            cout << "Error: " << ptrGrabResult->GetErrorCode()
                                << " " << ptrGrabResult->GetErrorDescription() << endl;
                            cout << "\033[0m";
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
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "12mm");
                            write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                            write_xml_entry(file, "Raw-Exposure", to_string(camera.ExposureTimeRaw.GetValue()).c_str());
                            close_xml_file(file);
                            channel_num = iterate_channel(channel_num);

                            Sleep(500);
                        }
                        else
                        {
                            cout << "\033[31m";
                            cout << "Error: " << ptrGrabResult->GetErrorCode()
                                << " " << ptrGrabResult->GetErrorDescription() << endl;
                            cout << "\033[0m";
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
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "12mm");
                            write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                            write_xml_entry(file, "Raw-Exposure", to_string(camera.ExposureTimeRaw.GetValue()).c_str());
                            close_xml_file(file);
                            channel_num = iterate_channel(channel_num);

                            Sleep(500);
                        }
                        else
                        {
                            cout << "\033[31m";
                            cout << "Error: " << ptrGrabResult->GetErrorCode()
                                << " " << ptrGrabResult->GetErrorDescription() << endl;
                            cout << "\033[0m";
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
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "12mm");
                            write_xml_entry(file, "Channel", to_string(channel_num).c_str());
                            write_xml_entry(file, "Raw-Exposure", to_string(camera.ExposureTimeRaw.GetValue()).c_str());
                            close_xml_file(file);
                            channel_num = iterate_channel(channel_num);

                            Sleep(500);
                        }
                        else
                        {
                            cout << "\033[31m";
                            cout << "Error: " << ptrGrabResult->GetErrorCode()
                                << " " << ptrGrabResult->GetErrorDescription() << endl;
                            cout << "\033[0m";
                            PylonTerminate();
                            exit(1);

                        }

                        cout << "Camera shuttering terminated..." << endl;
                    }
                    if (column == 'D') row++;
                    column = iterate_column(column);
                    cout << "\nContinue to Row: " << row << ", Column: " << column << ". Press any key (q to quit).\n" << endl;
                    cin >> randomInput;
                    if (randomInput == "q") {
                        row = 19;
                    }

                }

            }

                if (commands != 9) {
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

        }
        catch (GenICam::GenericException& e)
            {
                cout << "\033[31m";
                cerr << "An exception occurred." << endl << e.GetDescription() << endl << e.GetSourceFileName()
                    << ":" << e.GetSourceLine() << endl;
                cout << "Press any button to exit" << endl << "Input: ";
                string s;
                cin >> s;
                cout << "\033[0m";
                exit(1);
            }

        WSACleanup();
        return 0;
        }