#pragma once
#ifndef CONCURRENTSYSTEM_H
#define CONCURRENTSYSTEM_H

#include <windows.h>
#include <winhttp.h>
#include <string>

#include "Image.h"

#define DEFAULT_LIGHT_IP_ADDRESS L"192.168.0.5"

/// <summary>
/// Creates a formatted string of the current system time when called
/// Author: Jia Ze (Xavier) Chong 
/// </summary>
/// <returns>String of current date and time</returns>
const std::string currentDateTime();

/// <summary>
/// This function helps take an image using the deadman switch input.
/// Author: Jia Ze (Xavier) Chong
/// </summary>
/// <param name="camera"></param>
/// <returns>Returns captured image object generated.</returns>
Image captureImageDeadman(CBaslerUniversalInstantCamera& camera);

/// <summary>
/// This functions helps take an image using keyboard for input.
/// Author: Jia Ze (Xavier) Chong
/// </summary>
/// <param name="camera"></param>
/// <returns>Returns captured image object generated.</returns>
Image captureImageKeyboard(CBaslerUniversalInstantCamera& camera);

/// <summary>
/// Checks to make sure that the "image" directory exists, creates it if it doesn't.
/// </summary>
void checkForImageDirectory(std::string start_time);

/// <summary>
/// Returns a new instance of a path used in order to capture and save imaages correctly
/// </summary>
/// <returns>Returns new instance of string upon call</returns>
std::string getImagePath(std::string aux);

#endif // !CONCURRENTSYSTEM_H
