#pragma once

#ifndef XMLTAGGING_H
#define XMLTAGGING_H

#include <fstream>
#include <string>

using namespace std;

/// <summary>
/// Submits an entry with its object tag and specified value to desired .xml file
/// </summary>
/// <param name="file"></param>
/// <param name="tag"></param>
/// <param name="value"></param>
void write_xml_entry(ofstream& file, const char* tag, const char* value);

/// <summary>
/// Initializes the creation tags for an .xml file
/// </summary>
/// <param name="file"></param>
void init_xml_file(ofstream& file);

/// <summary>
/// Finishes the tags and closes the desired .xml file
/// </summary>
/// <param name="file"></param>
void close_xml_file(ofstream& file);

/// <summary>
/// Searches and deletes the desired matching entry inside of an .xml file
/// </summary>
/// <param name="file"></param>
/// <param name="entry"></param>
/// <returns></returns>
bool delete_xml_entry(ofstream& file, string& entry);

/// <summary>
/// Controls the flow of channel number when taking photos
/// </summary>
/// <param name="channel"></param>
/// <returns></returns>
int iterate_channel(int channel);

/// <summary>
/// Controls the flow of column letter when taking photos
/// </summary>
/// <param name="column"></param>
/// <returns></returns>
char iterate_column(char column);

/// <summary>
/// This function is called whenever the executable is ran within a new package that cannot identify a default xml file.
/// </summary>
void update_config_file(ofstream& file);

#endif //XMLTAGGING_H
