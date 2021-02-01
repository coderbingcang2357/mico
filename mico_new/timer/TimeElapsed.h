#include <assert.h>
#include <sys/time.h>
#include <string>

class TimeElap
{
public:
    TimeElap() : isInit(false)
    {
    }

    void start()
    {
        isInit = true;
        gettimeofday(&starttime, 0);
    }

    long elaps()
    {
        assert(isInit);
        isInit = false;

        timeval cur;
        gettimeofday(&cur, 0);
        timeval el;
        timersub(&cur, &starttime, &el);
        return el.tv_sec * 1000 * 1000 + el.tv_usec;
    }

private:
    bool isInit;
    timeval starttime;
};

namespace TimeOfNow{
	void AddPrefixZero(std::string &str){
		if(str.size() == 1){
			str = "0" + str;
		}	
	}
	std::string GetTimeOfNow(){     
		// 基于当前系统的当前日期/时间  
		std::string res = "";       
		time_t now = time(0);
		tm *ltm = localtime(&now);

		res += std::to_string(1900+ltm->tm_year);
		res += "-";           
		std::string mon = std::to_string(1 + ltm->tm_mon);
		AddPrefixZero(mon);
		res += mon;
		res += "-";           
		std::string day = std::to_string(ltm->tm_mday); 
		AddPrefixZero(day);
		res += day;
		res += " ";
		std::string hour = std::to_string(ltm->tm_hour); 
		AddPrefixZero(hour);
		res += hour;
		res += ":";            
		std::string min = std::to_string(ltm->tm_min);  
		AddPrefixZero(min);
		res += min;
		res += ":";            
		std::string sec = std::to_string(ltm->tm_sec);  
		AddPrefixZero(sec);
		res += sec;
		return res;            
	}
}
