#include <iostream>
#include "formatsql.h"

int main()
{
    std::string v = 
        FormatSql("insert into runningscene (sceneid, phone, startdate, startuser, runnerserverid, scenedata) "
                                " values (%llu, \"%s\", now(), %llu, %d, \"%s\")",
                                1,
                                "1",
                                2,
                                2,
                                "abc");
    std::cout << v << std::endl;
    return 0;
}
