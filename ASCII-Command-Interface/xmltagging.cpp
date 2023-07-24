#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void write_xml_entry(ofstream& file, const char* tag, const char* value) {
	file << "<" << tag << ">" << value << "</" << tag << ">\n";
}

void init_xml_file(std::ofstream& file) {
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	file << "<Root>\n";
}

void close_xml_file(std::ofstream& file) {
	file << "</Root>\n";
	file.close();

	std::cout << "File successfully closed." << std::endl;
}

bool delete_xml_entry(ofstream& file, const string& entry) {
	if (!file.is_open()) {
		cout << "Failed to open XML file." << endl;
	}

	string line;
	bool entryFound = false;
	bool insideEntry = false;

	ifstream inputFile("test.xml");

	while (getline(inputFile, line)) {

	}

	return false;
}