#include "smt.h"
#include <exception>
#include <regex>
#include <string>
#include <iostream>
#include <fstream>

bool operator==(const Node& lhs, const Node& rhs)
{
    return lhs.GetPos() == rhs.GetPos();
}

std::pair<bool, Node> SMT::CheckPinsOnDist(const std::pair<uint8_t, uint8_t>& pos, 
                                           uint8_t dist,
                                           const std::vector<Node>& pins_in_tree) const
{
    if (dist == 0)
        return {false, Node()};

    int yShift = dist,
        xShift = 0;
    auto begin = pins_in_tree.begin();
    auto end = pins_in_tree.end();
    for (; yShift >= 0; --yShift, ++xShift)
    {
        bool checkLeft = pos.second >= yShift;
        bool checkRight = (149 - pos.second) >= yShift;
        if ((149 - pos.first) >= xShift)
        {
            if (checkLeft)
            {
                uint8_t x = pos.first + xShift,
                        y = pos.second - yShift;
                if (grid_.IsPin(x, y) || grid_.IsPseudo(x, y))
                {
                    if (std::find_if(begin, end, [x, y](const auto& n) { return std::make_pair(x, y) == n.GetPos(); }) == end)
                        return {true, Node(x, y, grid_.GetType(x, y))};
                }
            }
            if (checkRight)
            {
                uint8_t x = pos.first + xShift,
                        y = pos.second + yShift;
                if (grid_.IsPin(x, y) || grid_.IsPseudo(x, y))
                {
                    if (std::find_if(begin, end, [x, y](const auto& n) { return std::make_pair(x, y) == n.GetPos(); }) == end)
                        return {true, Node(x, y, grid_.GetType(x, y))};
                }
            }
        }
        if (pos.first >= xShift)
        {
            if (checkLeft)
            {
                uint8_t x = pos.first - xShift,
                        y = pos.second - yShift;
                if (grid_.IsPin(x, y) || grid_.IsPseudo(x, y))
                {
                    if (std::find_if(begin, end, [x, y](const auto& n) { return std::make_pair(x, y) == n.GetPos(); }) == end)
                        return {true, Node(x, y, grid_.GetType(x, y))};
                }
            }
            if (checkRight)
            {
                uint8_t x = pos.first - xShift,
                        y = pos.second + yShift;
                if (grid_.IsPin(x, y) || grid_.IsPseudo(x, y))
                {
                    if (std::find_if(begin, end, [x, y](const auto& n) { return std::make_pair(x, y) == n.GetPos(); }) == end)
                        return {true, Node(x, y, grid_.GetType(x, y))};
                }
            }
        }
    }
    return {false, Node()};
}

std::pair<Node, Node> SMT::GetClosestPins(const std::vector<Node>& pins_in_tree) const
{
    uint8_t dist = 1;
    Node closest;
    bool found;
    while (dist < 150)
    {
        for (auto& pin : pins_in_tree)
        {
            std::tie(found, closest) = CheckPinsOnDist(pin.GetPos(), dist, pins_in_tree);
            if (found)
                return {pin, closest};
        }
        ++dist;
    }
    ASSERT(0 && "Closest pin not found");
    return {Node(), Node()};
}

void SMT::MakePath(const Node& pinFrom, const Node& pinTo)
{
    uint8_t xFrom, yFrom;
    std::tie(xFrom, yFrom) = pinFrom.GetPos();
    uint8_t xTo, yTo;
    std::tie(xTo, yTo) = pinTo.GetPos();
    if (yFrom != yTo)
    {
        std::pair<uint8_t,uint8_t> cur(xFrom, yFrom);
        auto step = [&yFrom](bool incr) { (incr ? ++yFrom : --yFrom); };
        bool incr = yFrom < yTo;
        for (step(incr); yFrom != yTo; step(incr))
        {
            std::pair<uint8_t, uint8_t> next(xFrom, yFrom);
            grid_.SetConnection(cur, next);
            cur = next;
        }
        grid_.SetConnection(cur, {xFrom, yFrom});
    }
    if (xFrom != xTo)
    {
        std::pair<uint8_t,uint8_t> cur(xFrom, yFrom);
        auto step = [&xFrom](bool incr) { (incr ? ++xFrom : --xFrom); };
        bool incr = xFrom < xTo;
        for (step(incr); xFrom != xTo; step(incr))
        {
            std::pair<uint8_t, uint8_t> next(xFrom, yFrom);
            grid_.SetConnection(cur, next);
            cur = next;
        }
        grid_.SetConnection(cur, {xFrom, yFrom});
    }
}

SMT::TreeInfo SMT::BuildMST()
{
    TreeInfo tree;
    std::vector<Node> pins_in_tree;
    pins_in_tree.reserve(grid_.GetNumPinsAll());
    pins_in_tree.push_back(*(grid_.GetPins().begin()));
    auto distance = [] (auto pos1, auto pos2) -> size_t
            {
                return (std::max(pos1.first, pos2.first) - std::min(pos1.first, pos2.first)) +
                       (std::max(pos1.second, pos2.second) - std::min(pos1.second, pos2.second));
            };
    while (pins_in_tree.size() != grid_.GetNumPinsAll())
    {
        //find two closest pins
        Node pinFrom, pinTo;
        std::tie(pinFrom, pinTo) = GetClosestPins(pins_in_tree);
        //add new pin
        pins_in_tree.push_back(pinTo);
        //save edge in tree
        tree.edges_.emplace_back(pinFrom, pinTo);
        tree.weight_ += distance(pinFrom.GetPos(), pinTo.GetPos());
    }
    return tree;
}

void SMT::BuildTree()
{
    //collect nodes for pseudo pins
    std::vector<Node> pseudo;
    auto& pins = grid_.GetPins();
    for (auto iter1 = pins.begin(); iter1 != pins.end(); ++iter1)
    {
        auto iter2 = iter1;
        ++iter2;
        for (; iter2 != pins.end(); ++iter2)
        {
            auto pos1 = (*iter1).GetPos();
            auto pos2 = (*iter2).GetPos();
            if (grid_.IsNone(pos1.first, pos2.second) && 
                    std::find(pseudo.begin(), pseudo.end(), Node(pos1.first, pos2.second, NodeType::PSEUDO)) == pseudo.end())
            {
                pseudo.emplace_back(pos1.first, pos2.second, NodeType::PSEUDO);
            }
            if (grid_.IsNone(pos2.first, pos1.second) &&
                    std::find(pseudo.begin(), pseudo.end(), Node(pos2.first, pos1.second, NodeType::PSEUDO)) == pseudo.end())
            {
                pseudo.emplace_back(pos2.first, pos1.second, NodeType::PSEUDO);
            }
        }
    }
    //build steiner tree
    tree_ = BuildMST();
    bool changed;
    do {
        changed = false;
        decltype(pseudo.begin()) chosenPinIter;
        for (auto iter = pseudo.begin(); iter != pseudo.end(); ++iter)
        {
            auto pos = (*iter).GetPos();
            grid_.AddPseudo((*iter).GetPos());
            auto cur_tree = std::move(BuildMST());
            if (cur_tree.weight_ < tree_.weight_)
            {
                tree_ = std::move(cur_tree);
                chosenPinIter = iter;
                changed = true;
            }
            grid_.RemovePseudo((*iter).GetPos());
        }
        if (changed)
        {
            grid_.AddPseudo((*chosenPinIter).GetPos());
            pseudo.erase(chosenPinIter);
        }
    } while (changed);
    for (const auto& edge : tree_.edges_)
        MakePath(edge.first, edge.second);
}

void SMT::DumpXML(const std::string& filename) const
{
    std::ofstream out;
    out.open(filename, std::ofstream::trunc);
    if (!out.is_open())
        throw bad_xml("cannot open output file");
    out << "<root>\n<grid min_x=\"0\" max_x=\"150\" min_y=\"0\" max_y=\"150\"/>\n<net>\n";
    //print pins, via and zero-length m_2 segments
    for (int x = 0; x < 150; ++x)
    {
        for (int y = 0; y < 150; ++y)
        {
            bool foundHorizontal = false,
                 foundVertical = false;
            if (x < 149)
                //foundVertical |= !grid_.IsFreeConnection(x, y, x + 1, y);
                foundHorizontal |= !grid_.IsFreeConnection(x, y, x + 1, y);
            if (x > 0)
                //foundVertical |= !grid_.IsFreeConnection(x, y, x - 1, y);
                foundHorizontal |= !grid_.IsFreeConnection(x, y, x - 1, y);
            if (y < 149)
                foundVertical |= !grid_.IsFreeConnection(x, y, x, y + 1);
            if (y > 0)
                foundVertical |= !grid_.IsFreeConnection(x, y, x, y - 1);
            //print m2_m3 via
            if (foundHorizontal && foundVertical)
            {
                out << "<point x=\"" << x << 
                           "\" y=\"" << y << "\" layer=\"m2_m3\" type=\"via\"/>\n";
            }
            bool isPin = grid_.IsPin(x, y);
            if (isPin)
            {
                out << "<point x=\"" << x << "\" y=\"" << y << "\" layer=\"pins\" type=\"pin\"/>\n";
                if (foundVertical || foundHorizontal)
                {
                    out << "<point x=\"" << x << 
                               "\" y=\"" << y <<  "\" layer=\"pins_m2\" type=\"via\"/>\n";
                    if (!foundHorizontal)
                    {
                        out << "<point x=\"" << x << 
                                   "\" y=\"" << y << "\" layer=\"m2_m3\" type=\"via\"/>\n";
                        out << "<segment x1=\"" << x <<
                                     "\" y1=\"" << y <<
                                     "\" x2=\"" << x <<
                                     "\" y2=\"" << y << "\" layer=\"m2\"/>\n";
                    }
                }
            }
        }
    }
    //print connections
    for (int k = 0; k < 150; ++k)
    {
        bool horizontal_line = false,
             vertical_line = false;
        std::pair<int, int> hor_start,
                                    hor_end,
                                    ver_start,
                                    ver_end;
        for (int m = 0; m < 149; ++m)
        {
            if (grid_.GetHorizontal(k, m))
            {
                hor_end = {k, m + 1};
                if (!horizontal_line)
                {
                    hor_start = {k, m};
                    horizontal_line = true;
                }
#if 0
                out << "<segment x1=\"" << k <<
                             "\" y1=\"" << m <<
                             "\" x2=\"" << k <<
                             "\" y2=\"" << m + 1 << "\" layer=\"m3\"/>\n";
#endif
            }
            else
            {
                if (horizontal_line)
                {
                    horizontal_line = false;
                    out << "<segment x1=\"" << hor_start.first <<
                                 "\" y1=\"" << hor_start.second <<
                                 "\" x2=\"" << hor_end.first <<
                                 "\" y2=\"" << hor_end.second << "\" layer=\"m3\"/>\n";

                }
            }
            if (grid_.GetVertical(k, m))
            {
                ver_end = {m + 1, k};
                if (!vertical_line)
                {
                    ver_start = {m, k};
                    vertical_line = true;
                }
#if 0
                out << "<segment x1=\"" << m <<
                             "\" y1=\"" << k <<
                             "\" x2=\"" << m + 1 <<
                             "\" y2=\"" << k << "\" layer=\"m2\"/>\n";
#endif
            }
            else
            {
                if (vertical_line)
                {
                    vertical_line = false;
                    out << "<segment x1=\"" << ver_start.first <<
                                 "\" y1=\"" << ver_start.second <<
                                 "\" x2=\"" << ver_end.first <<
                                 "\" y2=\"" << ver_end.second << "\" layer=\"m2\"/>\n";

                }
            }
        }
        if (horizontal_line)
        {
            out << "<segment x1=\"" << hor_start.first <<
                         "\" y1=\"" << hor_start.second <<
                         "\" x2=\"" << hor_end.first <<
                         "\" y2=\"" << hor_end.second << "\" layer=\"m3\"/>\n";
            
        }
        if (vertical_line)
        {
            vertical_line = false;
            out << "<segment x1=\"" << ver_start.first <<
                         "\" y1=\"" << ver_start.second <<
                         "\" x2=\"" << ver_end.first <<
                         "\" y2=\"" << ver_end.second << "\" layer=\"m2\"/>\n";
        }
    }
    out << "</net>\n</root>";
    out.close();
}

SMT::SMT(const std::string& filename)
{
    //open file
    std::ifstream input;
    input.open(filename, std::ios_base::in);
    if (!input.is_open())
        throw bad_xml("cannot open file");
    //read file
    input.seekg(0, input.end);
    size_t file_size = input.tellg();
    char* buf = new char[file_size];
    input.seekg(0, input.beg);
    input.read(buf,file_size);
    input.close();
    std::string str(buf, file_size);
    delete[] buf;

    std::smatch match;
    //get all points
    std::regex point_rx("<point.*/>");
    std::sregex_iterator next(str.begin(), str.end(), point_rx),
                          end;
    while (next != end)
    {
        std::string res = (*next).str();
        if (!std::regex_search(res, match, std::regex("x=\"([0-9]+)\" y=\"([0-9]+)\"")))
            throw bad_xml("no coordinates for pin");
        size_t x = std::stoul(match.str(1)),
               y = std::stoul(match.str(2));
        grid_.AddPin(x, y);
        next++;
    }
}
