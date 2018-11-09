#include "dockerbase.h"
uint64_t getDockerTime(const std::string& timeStr)
{
    std::string times=timeStr.substr(0,10)+" "+timeStr.substr(11,8);
    uint64_t unixTime=0;
    unixTime=TimeestampStr(times.c_str());
    return unixTime;
}
uint64_t TimeestampStr(const char *nTimeStr)
{
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    sscanf(nTimeStr, "%d-%d-%d %d:%d:%d",&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
           &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon--;
    return (uint64_t)mktime(&tm);
}
std::string unixTime2Str(uint64_t unixtime)
{ 
    tm* tLocalTime = localtime((time_t *)&unixtime);   
    char szTime[60] = {'\0'};    
    strftime(szTime, 60, "%Y-%m-%d %H:%M:%S", tLocalTime);   
    std::string strTime = szTime;
    return strTime;    
}