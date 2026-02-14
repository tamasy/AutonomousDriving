#pragma once

#include <vector>
#include <string>

namespace trajectory_editor {

class CSVParser {
public:
    CSVParser();
    ~CSVParser();
    
    std::vector<std::vector<std::string>> parseFile(const std::string& filepath);
    bool writeFile(const std::string& filepath, 
                   const std::vector<std::vector<std::string>>& data);
    
    bool hasHeader() const { return has_header_; }
    
private:
    bool has_header_;
    std::string trim(const std::string& str);
};

} // namespace trajectory_editor