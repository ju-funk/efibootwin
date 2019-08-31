#ifndef PCH_H
#define PCH_H


#include <string>       
#include <vector>
#include <functional>
#include <map>
#include <unordered_map>

#include <windows.h>


typedef std::vector<int> tvInt;
typedef std::wstring     twstring;

const DWORD UVH_START_OWN_Error    = 0x00010000;
const DWORD UVH_Error_Var_NotFound = UVH_START_OWN_Error + 1;


#endif //PCH_H
