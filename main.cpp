#include "smt.h"
#include <regex>
#include <cstdio>
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: ./tree <name>\n");
        return 0;
    }
    try {
        SMT smt(argv[1]);
        smt.BuildTree();
        std::string str(argv[1]);
        str = std::regex_replace(str, std::regex(".xml"), "_out.xml");
        smt.DumpXML(str);
    }
    catch (bad_xml& e)
    {
        printf("Exception caught! %s", e.what());
        return -1;
    }
}
