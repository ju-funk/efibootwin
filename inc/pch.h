//-----------------------------------------------
// Module name: pch.h
//-----------------------------------------------
// Description: Precompile Header, defines for all
//-----------------------------------------------


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
typedef std::vector<twstring> tvString;

const DWORD UVH_START_OWN_Error    = 0x00010000;
const DWORD UVH_Error_Var_NotFound = UVH_START_OWN_Error    + 1;
const DWORD UVH_Error_Size_Differ  = UVH_Error_Var_NotFound + 1;


#endif //PCH_H
