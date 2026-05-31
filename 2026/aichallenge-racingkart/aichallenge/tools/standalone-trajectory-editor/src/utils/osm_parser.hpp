#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace trajectory_editor {

struct OSMNode {
    int id;
    double local_x;
    double local_y;
    double elevation;
};

struct OSMWay {
    int id;
    std::vector<int> node_refs;
    std::map<std::string, std::string> tags;
};

struct OSMRelation {
    int id;
    std::vector<std::pair<std::string, int>> members; // (role, ref_id)
    std::map<std::string, std::string> tags;
};

class OSMParser {
public:
    bool loadFromFile(const std::string& filename);
    
    // データアクセス
    const std::unordered_map<int, OSMNode>& getNodes() const { return nodes_; }
    const std::unordered_map<int, OSMWay>& getWays() const { return ways_; }
    const std::unordered_map<int, OSMRelation>& getRelations() const { return relations_; }
    
    // 境界線抽出
    std::vector<std::pair<std::vector<OSMNode>, std::vector<OSMNode>>> 
    extractTrackBoundaries() const;

private:
    std::unordered_map<int, OSMNode> nodes_;
    std::unordered_map<int, OSMWay> ways_;
    std::unordered_map<int, OSMRelation> relations_;
    
    void parseNode(const std::string& line);
    void parseWay(const std::string& line);
    void parseRelation(const std::string& line);
    std::string extractAttribute(const std::string& line, const std::string& attr) const;
    std::string extractTagValue(const std::string& line, const std::string& key) const;
};

} // namespace trajectory_editor