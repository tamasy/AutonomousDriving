#include "csv_parser.hpp"
#include <fstream>
#include <sstream>

namespace trajectory_editor {

CSVParser::CSVParser() : has_header_(true) {}

CSVParser::~CSVParser() = default;

std::vector<std::vector<std::string>> CSVParser::parseFile(const std::string& filepath) {
    std::vector<std::vector<std::string>> result;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return result;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::vector<std::string> fields;
            std::stringstream ss(line);
            std::string field;
            
            while (std::getline(ss, field, ',')) {
                fields.push_back(trim(field));
            }
            
            if (!fields.empty()) {
                result.push_back(std::move(fields));
            }
        }
    }
    
    file.close();
    return result;
}

bool CSVParser::writeFile(const std::string& filepath, 
                         const std::vector<std::vector<std::string>>& data) {
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) file << ",";
            file << row[i];
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

std::string CSVParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace trajectory_editor