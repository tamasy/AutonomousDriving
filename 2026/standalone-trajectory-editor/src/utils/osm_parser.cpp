#include "osm_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

namespace trajectory_editor {

bool OSMParser::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    nodes_.clear();
    ways_.clear();
    relations_.clear();
    
    std::string line;
    std::string current_section = "";
    std::string multi_line_content = "";
    
    while (std::getline(file, line)) {
        // 空行やコメント行をスキップ
        if (line.empty() || line.find("<?xml") != std::string::npos || 
            line.find("<osm") != std::string::npos || line.find("<MetaInfo") != std::string::npos) {
            continue;
        }
        
        // ノード処理
        if (line.find("<node id=") != std::string::npos) {
            current_section = "node";
            multi_line_content = line;
            continue;
        }
        
        // ウェイ処理
        if (line.find("<way id=") != std::string::npos) {
            current_section = "way";
            multi_line_content = line;
            continue;
        }
        
        // リレーション処理
        if (line.find("<relation id=") != std::string::npos) {
            current_section = "relation";
            multi_line_content = line;
            continue;
        }
        
        // セクション内のコンテンツを蓄積
        if (!current_section.empty()) {
            multi_line_content += "\n" + line;
        }
        
        // セクション終了の検出と処理
        if (line.find("</node>") != std::string::npos) {
            parseNode(multi_line_content);
            current_section = "";
            multi_line_content = "";
        } else if (line.find("</way>") != std::string::npos) {
            parseWay(multi_line_content);
            current_section = "";
            multi_line_content = "";
        } else if (line.find("</relation>") != std::string::npos) {
            parseRelation(multi_line_content);
            current_section = "";
            multi_line_content = "";
        }
    }
    
    std::cout << "OSM file loaded: " << nodes_.size() << " nodes, " 
              << ways_.size() << " ways, " << relations_.size() << " relations" << std::endl;
    
    return true;
}

void OSMParser::parseNode(const std::string& content) {
    OSMNode node;
    
    // IDを抽出
    std::string id_str = extractAttribute(content, "id");
    if (id_str.empty()) return;
    node.id = std::stoi(id_str);
    
    // タグから座標を抽出
    std::string x_str = extractTagValue(content, "local_x");
    std::string y_str = extractTagValue(content, "local_y");
    std::string ele_str = extractTagValue(content, "ele");
    
    if (x_str.empty() || y_str.empty()) return;
    
    node.local_x = std::stod(x_str);
    node.local_y = std::stod(y_str);
    node.elevation = ele_str.empty() ? 0.0 : std::stod(ele_str);
    
    nodes_[node.id] = node;
}

void OSMParser::parseWay(const std::string& content) {
    OSMWay way;
    
    // IDを抽出
    std::string id_str = extractAttribute(content, "id");
    if (id_str.empty()) return;
    way.id = std::stoi(id_str);
    
    // ノード参照を抽出
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("<nd ref=") != std::string::npos) {
            std::string ref_str = extractAttribute(line, "ref");
            if (!ref_str.empty()) {
                way.node_refs.push_back(std::stoi(ref_str));
            }
        } else if (line.find("<tag k=") != std::string::npos) {
            // タグ抽出（将来の拡張用）
            std::string key = extractAttribute(line, "k");
            std::string value = extractAttribute(line, "v");
            if (!key.empty() && !value.empty()) {
                way.tags[key] = value;
            }
        }
    }
    
    if (!way.node_refs.empty()) {
        ways_[way.id] = way;
    }
}

void OSMParser::parseRelation(const std::string& content) {
    OSMRelation relation;
    
    // IDを抽出
    std::string id_str = extractAttribute(content, "id");
    if (id_str.empty()) return;
    relation.id = std::stoi(id_str);
    
    // メンバーとタグを抽出
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("<member") != std::string::npos) {
            std::string role = extractAttribute(line, "role");
            std::string ref_str = extractAttribute(line, "ref");
            if (!role.empty() && !ref_str.empty()) {
                relation.members.push_back({role, std::stoi(ref_str)});
            }
        } else if (line.find("<tag k=") != std::string::npos) {
            std::string key = extractAttribute(line, "k");
            std::string value = extractAttribute(line, "v");
            if (!key.empty() && !value.empty()) {
                relation.tags[key] = value;
            }
        }
    }
    
    relations_[relation.id] = relation;
}

std::string OSMParser::extractAttribute(const std::string& line, const std::string& attr) const {
    std::string pattern = attr + "=\"([^\"]+)\"";
    std::regex regex(pattern);
    std::smatch match;
    
    if (std::regex_search(line, match, regex)) {
        return match[1];
    }
    return "";
}

std::string OSMParser::extractTagValue(const std::string& content, const std::string& key) const {
    std::string pattern = "<tag k=\"" + key + "\" v=\"([^\"]+)\"";
    std::regex regex(pattern);
    std::smatch match;
    
    if (std::regex_search(content, match, regex)) {
        return match[1];
    }
    return "";
}

std::vector<std::pair<std::vector<OSMNode>, std::vector<OSMNode>>> 
OSMParser::extractTrackBoundaries() const {
    std::vector<std::pair<std::vector<OSMNode>, std::vector<OSMNode>>> boundaries;
    
    // laneletリレーションを探す
    for (const auto& [rel_id, relation] : relations_) {
        if (relation.tags.find("type") != relation.tags.end() && 
            relation.tags.at("type") == "lanelet") {
            
            std::vector<OSMNode> left_boundary, right_boundary;
            
            // left と right の境界線を取得
            for (const auto& [role, ref_id] : relation.members) {
                if (ways_.find(ref_id) != ways_.end()) {
                    const OSMWay& way = ways_.at(ref_id);
                    std::vector<OSMNode> boundary_nodes;
                    
                    // wayのノードを順番に取得
                    for (int node_id : way.node_refs) {
                        if (nodes_.find(node_id) != nodes_.end()) {
                            boundary_nodes.push_back(nodes_.at(node_id));
                        }
                    }
                    
                    if (role == "left") {
                        left_boundary = boundary_nodes;
                    } else if (role == "right") {
                        right_boundary = boundary_nodes;
                    }
                }
            }
            
            if (!left_boundary.empty() && !right_boundary.empty()) {
                boundaries.push_back({left_boundary, right_boundary});
            }
        }
    }
    
    std::cout << "Found " << boundaries.size() << " lane boundaries" << std::endl;
    return boundaries;
}

} // namespace trajectory_editor