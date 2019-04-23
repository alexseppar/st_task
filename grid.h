#ifndef GRID_H
#define GRID_H

#ifdef DEBUG

#include <cassert>
#define ASSERT(x) assert(x)

#else

#define ASSERT(x) 

#endif

#include <utility>
#include <algorithm>
#include <bitset>
#include <list>

enum NodeType {
    NONE,
    PIN,
    PSEUDO
};

class Node final {
    private:
        uint8_t i_ = 0,
                j_ = 0;
        NodeType type_ = NodeType::NONE;
    public:
        Node() {}
        Node(uint8_t i, uint8_t j, NodeType type):
        i_(i), j_(j), type_(type)
        {}
        std::pair<uint8_t, uint8_t> GetPos() const
        {
            return {i_, j_};
        }
        NodeType GetType() const
        {
            return type_;
        }
};

class Grid final {
    private:
        //horizontal connections
        std::bitset<149> horizontal_[150];
        //vertical connections
        std::bitset<149> vertical_[150];
        //nodes that carry pins
        NodeType nodes_[150][150]{};
        std::list<Node> pins_;
    public:
        const std::list<Node>& GetPins() const
        {
            return pins_;
        }
        bool IsPin(uint8_t i, uint8_t j) const
        {
            return nodes_[i][j] == NodeType::PIN;
        }
        bool IsPseudo(uint8_t i, uint8_t j) const
        {
            return nodes_[i][j] == NodeType::PSEUDO;
        }
        bool IsNone(uint8_t i, uint8_t j) const
        {
            return nodes_[i][j] == NodeType::NONE;
        }
        NodeType GetType(uint8_t i, uint8_t j) const
        {
            return nodes_[i][j];
        }
        bool IsFreeConnection(uint8_t i1, uint8_t j1, uint8_t i2, uint8_t j2) const
        {
            if (i1 == i2)
            {
                uint8_t j_min = std::min(j1, j2);
                //check that nodes are adjacent
                ASSERT(std::max(j1, j2) - j_min == 1);
                return !horizontal_[i1].test(j_min);
            }
            else if (j1 == j2)
            {
                uint8_t i_min = std::min(i1, i2);
                //check that nodes are adjacent
                ASSERT(std::max(i1, i2) - i_min == 1);
                return !vertical_[j1].test(i_min);
            }
            else
                ASSERT(0 && "Checking connection of not adjacent nodes");
            return false;
        }
        bool IsFreeConnection(const std::pair<uint8_t, uint8_t>& pin1,
                              const std::pair<uint8_t, uint8_t>& pin2)
        {
            return IsFreeConnection(pin1.first, pin1.second, pin2.first, pin2.second);
        }
        bool GetVertical(uint8_t i, uint8_t j) const
        {
            return vertical_[i].test(j);
        }
        bool GetHorizontal(uint8_t i, uint8_t j) const
        {
            return horizontal_[i].test(j);
        }
        void SetConnection(uint8_t i1, uint8_t j1, uint8_t i2, uint8_t j2)
        {
            if (i1 == i2)
            {
                uint8_t j_min = std::min(j1, j2);
                //check that nodes are adjacent
                ASSERT(std::max(j1, j2) - j_min == 1);
                horizontal_[i1].set(j_min);
            }
            else if (j1 == j2)
            {
                uint8_t i_min = std::min(i1, i2);
                //check that nodes are adjacent
                ASSERT(std::max(i1, i2) - i_min == 1);
                vertical_[j1].set(i_min);
            }
            else
                ASSERT(0 && "Setting connection of not adjacent nodes");
        }
        void SetConnection(const std::pair<uint8_t, uint8_t>& pin1, 
                           const std::pair<uint8_t, uint8_t>& pin2)
        {
            SetConnection(pin1.first, pin1.second, pin2.first, pin2.second);
        }
        void AddPin(uint8_t i, uint8_t j)
        {
            nodes_[i][j] = NodeType::PIN;
            pins_.emplace_back(i, j, NodeType::PIN);
        }
        void AddPseudo(uint8_t i, uint8_t j)
        {
            ASSERT(nodes_[i][j] == NodeType::NONE);
            nodes_[i][j] = NodeType::PSEUDO;
            pins_.emplace_back(i, j, NodeType::PSEUDO);
        }
        void AddPseudo(const std::pair<uint8_t, uint8_t>& pos)
        {
            AddPseudo(pos.first, pos.second);
        }
        void RemovePseudo(uint8_t i, uint8_t j)
        {
            ASSERT(nodes_[i][j] == NodeType::PSEUDO);
            nodes_[i][j] = NodeType::NONE;
            pins_.remove_if( [i, j](const Node& n) 
                                { return std::make_pair(i, j) == n.GetPos(); });
        }
        void RemovePseudo(const std::pair<uint8_t, uint8_t>& pos)
        {
            RemovePseudo(pos.first, pos.second);
        }
        size_t GetNumPinsAll() const
        {
            return pins_.size();
        }
};

#endif
