#pragma once
#include <string>

// Main-thread UIKit boundary. Paths always stay in this app's sandbox.
std::string ios_data_directory();
std::string ios_import_rom(); // empty on cancellation/failure; diagnostics use SDL's error string
