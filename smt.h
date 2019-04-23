#ifndef SMT_
#define SMT_

#include "grid.h"

#include <vector>

class bad_xml final : public std::exception {
    private:
        const char* what_;
    public:
        explicit bad_xml(const char* what): what_(what) {}
        const char* what() const noexcept override
        {
            return what_;
        }
};

class SMT final {
    //private members
    private:
        struct TreeInfo final {
            size_t weight_ = 0;
            std::vector<std::pair<Node, Node>> edges_;
        };
        Grid grid_;
        TreeInfo tree_;
        uint8_t max_x = 0,
                min_x = 150,
                max_y = 0,
                min_y = 150;
    //private methods
    private:
        std::pair<bool, Node> CheckPinsOnDist(const std::pair<uint8_t, uint8_t>& pos, 
                                              uint8_t dist,
                                              const std::vector<Node>& tree) const;
        TreeInfo BuildMST();
        std::pair<Node, Node> GetClosestPins(const std::vector<Node>& tree) const;
        void MakePath(const Node& pinFrom, const Node& pinTo);
    //public methods
    public:
        explicit SMT(const std::string& filename);
        void BuildTree();
        void DumpXML(const std::string& filename) const;
};

#endif
