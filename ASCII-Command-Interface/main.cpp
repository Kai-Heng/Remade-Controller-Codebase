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
#include <thread>

#include "ConcurrentSystem.h"
#include "Image.h"
#include "xmlworker.h"

using namespace std;

/// <summary>
/// These constants are used to identify the device when we initialize a socket connection between the program and the controller.
/// </summary>
const char* dcs400e = "192.168.0.5";
const char* dcs100e = "192.168.0.1";
int port = 777;
int q_exposure = 14550;
char column = 'A';
int row = 1;
int channel_num = 1;
string randomInput;
string START_TIME;

bool increment = true;
bool rowSwitch = false;

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

    cout << "Checking for config file within directory..." << endl;
    ifstream configFile;

    // Now try to access the file
    configFile.open("config.xml");

    if (!configFile) {
        std::ofstream config("config.xml");

        // Initialize the config.xml file
        init_xml_file(config);
        write_xml_entry(config, "Cylinder Head No.", "1");
        write_xml_entry(config, "Face Type", "Front");
        write_xml_entry(config, "Lens Type", "6mm");
        write_xml_entry(config, "Working Distance", "100mm");
        write_xml_entry(config, "Raw Exposure", "12000");
        close_xml_file(config);

        cout << "New config.xml file created" << endl;
    }

    cout << "Welcome to the REMADE cylinder head imaging executable.\n Enter 0 to enter default configuration or press 1 to update config:" << endl;
    int configChecker;
    cin >> configChecker;

    switch (configChecker) {
    case 0:
        cout << "Applying current configuration info..." << endl;
        break;
    case 1:
        cout << "Updating configuration info..." << endl;
        // update_config_file(configFile);
        break;
    default:
        cout << "You've chosen another option!" << endl;
        break;
    };


    //Check for images directory
    //checkForImageDirectory(START_TIME);

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
        camera.ExposureTimeRaw.SetValue(q_exposure); // This can be a set reference depending on implementation. Maybe add a config file that handles this?
        //camera.GainRaw.SetValue(8); // Look into if this actually helps ??

        WSADATA wsaData;
        int iResult;

        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed: %d\n", iResult);
            return 1;
        }

        //create a message buffer 
        char msg[1500];
        char msg2[1500];

        //setup socket(s) and connection tools 
        SOCKET socket1 = socket(AF_INET, SOCK_STREAM, 0);
        SOCKET socket2 = socket(AF_INET, SOCK_STREAM, 0);

        if (socket1 == INVALID_SOCKET || socket2 == INVALID_SOCKET) {
            std::cerr << "Failed to create socket(s)." << std::endl;
            WSACleanup();
            return 1;
        }

        sockaddr_in serverAddr1;
        serverAddr1.sin_family = AF_INET;
        serverAddr1.sin_port = htons(port); // Replace with the actual port number
        serverAddr1.sin_addr.s_addr = inet_addr(dcs400e); // IP address of the first server


        if (connect(socket1, (struct sockaddr*)&serverAddr1, sizeof(serverAddr1)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to 192.168.0.5." << std::endl;
            closesocket(socket1);
            WSACleanup();
            return 1;
        }

        cout << "successfully connect to 192.168.0.5\n";

        sockaddr_in serverAddr2;
        serverAddr2.sin_family = AF_INET;
        serverAddr2.sin_port = htons(port); // Replace with the actual port number
        serverAddr2.sin_addr.s_addr = inet_addr(dcs100e); // IP address of the second server

        if (connect(socket2, (struct sockaddr*)&serverAddr2, sizeof(serverAddr2)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to 192.168.0.1." << std::endl;
            closesocket(socket2);
            WSACleanup();
            return 1;
        }
        cout << "Connected to the server!" << endl;
        int bytesRead = 0;
        int bytesWritten = 0;
        struct timeval start1, end1;
        gettimeofday(&start1, NULL);
        while (1)
        {

            cout << "Enter Cylinder or Cluster Number: ";
            int cylinderNum;
            cin >> cylinderNum;

            cout << "Enter Face Number (0 for Front, 1 for Right, 2 for Back, 3 for Left): ";
            int faceNum;
            cin >> faceNum;

            string faceStr;
            switch (faceNum) {
            case 0:
                faceStr = "Front";
                break;
            case 1:
                faceStr = "Right";
                break;
            case 2:
                faceStr = "Back";
                break;
            case 3:
                faceStr = "Left";
                break;
            default:
                cout << "Invalid face number. Exiting.";
                exit(1);
            }

            string start_time_temp = currentDateTime();

            START_TIME = "Cylinder_Head_ " + to_string(cylinderNum) + "_" + faceStr;

            checkForImageDirectory(START_TIME);

            string aux(argv[0]);
            int pos = aux.rfind('\\');
            string path = aux.substr(0, pos + 1);
            path += "image\\";
            path += "i";
            path += currentDateTime();
            path += ".bmp";

            cout << "----------- Commands ----------" << endl;
            cout << "-- Action Commands --\n";
            cout << "1. RUN:SEQUENCE (Photo by photo);\n";
            cout << "2. RUN:SEQUENCE (Full quadrant-cluster capture);\n";
            cout << "3. RUN:SEQUENCE (Entire face imaging sequence);\n";
            cout << "4. UPDATE EXPOSURE(Entire face imaging sequence);\n";

            cout << "-- Read Commands --\n";
            cout << "9. Exit\n";

            cout << ">";
            int commands;
            cin >> commands;
            string data, data2;
            string channel;
            string nnnn;

            row = 1;
            column = 'A';

            if (commands < 5) {
                data = "RUN:SEQUENCE;";
                data2 = "TRIGGER:CHANNEL1;";
            }
            else {
                data = "exit";
                data2 = "DISCONNECT;";
            }
            memset(&msg, 0, sizeof(msg));//clear the buffer
            memset(&msg2, 0, sizeof(msg2));
            strcpy(msg, data.c_str());
            strcpy(msg2, data2.c_str());
            if (data == "exit" || data2 == "DISCONNECT;")
            {
                send(socket1, (char*)&msg, strlen(msg), 0);
                send(socket2, (char*)&msg2, strlen(msg2), 0);
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
                send(socket2, (char*)&msg2, strlen(msg2), 0);
                bytesWritten += send(socket1, (char*)&msg, strlen(msg), 0);
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
                send(socket2, (char*)&msg2, strlen(msg2), 0);
                bytesWritten += send(socket1, (char*)&msg, strlen(msg), 0);

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
                send(socket2, (char*)&msg2, strlen(msg2), 0);
                bytesWritten += send(socket1, (char*)&msg, strlen(msg), 0);

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
                send(socket2, (char*)&msg2, strlen(msg2), 0);
                bytesWritten += send(socket1, (char*)&msg, strlen(msg), 0);

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

                send(socket2, (char*)&msg2, strlen(msg2), 0);
                bytesWritten += send(socket1, (char*)&msg, strlen(msg), 0);


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

               // cout << "Input 0 for Front & Back Face Image, and 1 for Side Face Imaging: " << endl;
               // int faceFlag;
               // cin >> faceFlag;

                while (row <=11 && column <= 'B') {

                    cout << "Capturing channel 1...\n";

                    path = getImagePath(aux);

                    camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                    camera.TriggerMode.SetValue(TriggerMode_On);
                    camera.TriggerSource.SetValue(TriggerSource_Software);


                    std::thread thread1([socket1, msg]() {
                        send(socket1, (char*)msg, strlen(msg), 0);
                        });
                    thread1.join();
                    //std::thread thread2([socket2, msg2]() {
                        //send(socket2, (char*)&msg2, strlen(msg2), 0);
                        //});
                   // thread2.join();
                    camera.StartGrabbing(1);

                    while (camera.IsGrabbing())
                    {
                        camera.TriggerSoftware.Execute();
                        CGrabResultPtr ptrGrabResult;
                        camera.RetrieveResult(60000, ptrGrabResult);

                        if (ptrGrabResult->GrabSucceeded()) {
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "6mm");
                            write_xml_entry(file, "F-Ratio", "f/8");
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

                    std::thread thread3([socket1, msg]() {
                        send(socket1, (char*)msg, strlen(msg), 0);
                        });
                    thread3.join();
                    //std::thread thread4([socket2, msg2]() {
                        //send(socket2, (char*)&msg2, strlen(msg2), 0);
                        //});
                    //thread4.join();
                    camera.StartGrabbing(1);

                    while (camera.IsGrabbing())
                    {
                        camera.TriggerSoftware.Execute();
                        CGrabResultPtr ptrGrabResult;
                        camera.RetrieveResult(60000, ptrGrabResult);

                        if (ptrGrabResult->GrabSucceeded()) {
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "6mm");
                            write_xml_entry(file, "F-Ratio", "f/8");
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

                    std::thread thread5([socket1, msg]() {
                        send(socket1, (char*)msg, strlen(msg), 0);
                        });
                    thread5.join();
                    //std::thread thread6([socket2, msg2]() {
                      //  send(socket2, (char*)&msg2, strlen(msg2), 0);
                        //});
                    //thread6.join();
                    camera.StartGrabbing(1);

                    while (camera.IsGrabbing())
                    {
                        camera.TriggerSoftware.Execute();
                        CGrabResultPtr ptrGrabResult;
                        camera.RetrieveResult(60000, ptrGrabResult);

                        if (ptrGrabResult->GrabSucceeded()) {
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "6mm");
                            write_xml_entry(file, "F-Ratio", "f/8");
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
                    std::thread thread7([socket1, msg]() {
                        send(socket1, (char*)msg, strlen(msg), 0);
                        });
                    thread7.join();
                    //std::thread thread8([socket2, msg2]() {
                      //  send(socket2, (char*)&msg2, strlen(msg2), 0);
                        //});
                    //thread8.join();

                    while (camera.IsGrabbing())
                    {
                        camera.TriggerSoftware.Execute();
                        CGrabResultPtr ptrGrabResult;
                        camera.RetrieveResult(60000, ptrGrabResult);

                        if (ptrGrabResult->GrabSucceeded()) {
                            Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                            std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(channel_num).c_str() + ".xml").c_str());
                            init_xml_file(file);
                            write_xml_entry(file, "Lens-Type", "6mm");
                            write_xml_entry(file, "F-Ratio", "f/8");
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

                        cout << "Capturing channel 5...\n";

                        path = getImagePath(aux);

                        camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
                        camera.TriggerMode.SetValue(TriggerMode_On);
                        camera.TriggerSource.SetValue(TriggerSource_Software);
                        camera.ExposureTimeRaw.SetValue(80000);

                        camera.StartGrabbing(1);
                        std::thread thread7([socket1, msg]() {
                            //send(socket1, (char*)msg, strlen(msg), 0);
                            });
                        thread7.join();
                        //std::thread thread8([socket2, msg2]() {
                          //  send(socket2, (char*)&msg2, strlen(msg2), 0);
                            //});
                        //thread8.join();

                        while (camera.IsGrabbing())
                        {
                            camera.TriggerSoftware.Execute();
                            CGrabResultPtr ptrGrabResult;
                            camera.RetrieveResult(60000, ptrGrabResult);

                            if (ptrGrabResult->GrabSucceeded()) {
                                Image::from(ptrGrabResult).saveImage((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(5).c_str() + ".bmp").c_str(), ImageFileFormat_Bmp);
                                std::ofstream file((START_TIME + "/" + column + to_string(row).c_str() + "_Q" + to_string(5).c_str() + ".xml").c_str());
                                init_xml_file(file);
                                write_xml_entry(file, "Lens-Type", "6mm");
                                write_xml_entry(file, "F-Ratio", "f/8");
                                write_xml_entry(file, "Channel", to_string(5).c_str());
                                write_xml_entry(file, "Raw-Exposure", to_string(camera.ExposureTimeRaw.GetValue()).c_str());
                                close_xml_file(file);
                                //channel_num = iterate_channel(channel_num);

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
                            camera.ExposureTimeRaw.SetValue(q_exposure);
                        }


                        if (faceNum == 0 || faceNum == 2) {
                      
                            if (column == 'A' && row < 11) {
                               
                                row++;
                            }
                            else if (column == 'A' && row == 11) {
                             
                                column = 'B';
                                increment = false; // Set to decrementing
                            }
                            else if (column == 'B' && row > 1 && !increment) {
                            
                                row--;
                            }
                            else if (column == 'B' && row == 1) {
                         
                                column = 'C'; // termination
                                increment = true; 
                            }
                        }
                        else if (faceNum == 1 || faceNum == 3) {
                            
                            column = 'A'; 
                            row++;
                        }

                        cout << "\nContinue to Row: " << row << ", Column: " << column << ". Press any key (q to quit).\n" << endl;
                        cin >> randomInput;
                        if (randomInput == "q") {
                            row = 12;
                        }

                    }

                }

              
                if (commands != 9) {
                    cout << "Awaiting server response..." << endl;
                    memset(&msg, 0, sizeof(msg));//clear the buffer
                    bytesRead += recv(socket1, (char*)&msg, sizeof(msg), 0);
                    if (!strcmp(msg, "exit"))
                    {
                        cout << "Server has quit the session" << endl;
                        break;
                    }
                    cout << "Server: " << msg << endl;
                }
          
            }
            else if (commands == 4) {
                cout << "Enter a new Exposure Value: " << endl;
                double new_exposure;
                cin >> new_exposure;

                // Assuming q_exposure is declared elsewhere and camera is an initialized object with an ExposureTimeRaw property.
                q_exposure = round(new_exposure / 25) * 25;

                try {
                    camera.ExposureTimeRaw.SetValue(q_exposure);
                    cout << "Exposure updated to " << q_exposure << " successfully." << endl;
                }
                catch (const exception& e) {
                    cerr << "An error occurred while updating the exposure: " << e.what() << endl;
                }

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