// SetBootNext.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "..\inc\pch.h"
#include "..\inc\UefiVarHandling.h"




UefiVarHandling::DynData::DynData(DWORD size /*= 4096*/)
{   
    if (size > 0)
    {
        BYTE *p = (BYTE *) malloc(size);
        bmem.reset(p, deleter());
        nSize   = size;
    }
}

void UefiVarHandling::DynData::Resize(DWORD size)
{
    BYTE *p = (BYTE *) realloc(bmem.get(), size);
    std::get_deleter<deleter>(bmem)->NotDel();
    bmem.reset(p, deleter());
    nSize   = size;
}

UefiVarHandling::DynData UefiVarHandling::DynData::operator=(const DynData &oth)
{
    nSize   = oth.nSize;
    bmem    = oth.bmem;

    return *this;
}


UefiVarHandling::~UefiVarHandling()
{
    if(hAccessToken)
        CloseHandle(hAccessToken);
}

bool UefiVarHandling::SetPrivileg()
{
    TOKEN_PRIVILEGES tpTokenPrivilege;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hAccessToken))
    {
        nStatus = GetLastError();

        return false;
    }
    /************************************************************************\
     *     * Get LUID of SeTakeOwnershipPrivilege privilege     *
    \************************************************************************/

    if (!LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tpTokenPrivilege.Privileges[0].Luid))
    {
        // wprintf(L"\nThe above error means you need to use User Manager (menu item");
        // wprintf(L"\n  Policies\\UserRights) to turn on the 'Take ownership of...' ");
        // wprintf(L"\n  privilege, log off, log back on");

        nStatus = GetLastError();
        CloseHandle(hAccessToken);
        hAccessToken = nullptr;

        return false;
    }
    /************************************************************************\
     *     * Enable the SeTakeOwnershipPrivilege privilege using the LUID just
     *     * obtained     *
    \************************************************************************/

    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hAccessToken,
        FALSE,  // Do not disable all                            
        &tpTokenPrivilege,
        0,      // buffer length for previous info
        NULL,   // Ignore previous info  
        NULL))  // Ignore return length      
    {
        nStatus = GetLastError();
        CloseHandle(hAccessToken);
        hAccessToken = nullptr;

        return false;
    }


    return true;
}


twstring UefiVarHandling::GetStateString()
{
    twstring err = mErrMsg[nStatus];

    if (err.empty())
    {
        const DWORD mx = 30;
        wchar_t num[mx];
        _itow_s(nStatus, num, mx, 10);

        LPWSTR pBuf = nullptr;
        DWORD  ret  = 0;
        if(nStatus < UVH_START_OWN_Error)
            ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, nStatus, 0, (LPWSTR)&pBuf, 10, nullptr);
        if (ret > 0)
        {
            err = pBuf;
            LocalFree(pBuf);

            err.insert(0, L") ");
            err = twstring(L"(NOF ") + num + err;
        }
        else
        {
            err = L"Error Message number not Found : ";
            err += num;
        }
    }

    return err;
}


bool UefiVarHandling::Init(std::function<void (DWORD dErr, twstring &sMsg)> UserMsgFunc) 
{ 
    SetStateStrings(UserMsgFunc);

    return SetPrivileg(); 
}



void UefiVarHandling::SetStateStrings(std::function<void (DWORD dErr, twstring &sMsg)> UserMsgFunc)
{
    DWORD Err[] = {NO_ERROR, ERROR_NOACCESS, STATUS_INVALID_PARAMETER, ERROR_INVALID_FUNCTION, ERROR_PRIVILEGE_NOT_HELD, ERROR_NOT_ALL_ASSIGNED,
                   ERROR_ENVVAR_NOT_FOUND,
                   UVH_Error_Var_NotFound };


    for (DWORD err : Err)
    {
        LPWSTR pBuf = nullptr;
        DWORD ret = 0;
        if (err < UVH_START_OWN_Error)
            ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, 0, (LPWSTR)&pBuf, 10, nullptr);

        twstring sErr;
        if (ret > 0)
        {
            sErr = pBuf;
            LocalFree(pBuf);
        }

        UserMsgFunc(err, sErr);

        mErrMsg[err] = sErr;
    }


}

DWORD UefiVarHandling::GetState(bool bReset /*= true*/)
{
    DWORD state = nStatus;
    if (bReset)
    {
        SetLastError(NO_ERROR);
        nStatus = NO_ERROR;
    }
    return state;
}


wchar_t* UefiVarHandling::GetHex(UINT16 i)
{
    const int max = 20;
    static wchar_t num[max];

    swprintf_s(num, max, L"%04x", i);

    return num;
}



DWORD UefiVarHandling::GetFirmVar(const twstring &VarName, const twstring &inGUID, DynData &Data, bool bEnums)
{
    bool    loop;
    DWORD  length = 0;
    do
    {
        loop = false;
        length = GetFirmwareEnvironmentVariable(VarName.c_str(), inGUID.c_str(), Data.GetMem(), Data.GetSize());
        if(length == 0)
            nStatus = GetLastError();
        else
            nStatus = NO_ERROR;

        switch (nStatus)
        {
            case NO_ERROR:
                if(bEnums)
                    Data.Resize(length);
                break;

            case ERROR_NOACCESS:           // Guid is unknown
            case STATUS_INVALID_PARAMETER: // wrong parameter wrong varname?
            case ERROR_INVALID_FUNCTION:   // it is not uefi
            case ERROR_PRIVILEGE_NOT_HELD:
                Data.SetError();
                break;

            case ERROR_ENVVAR_NOT_FOUND:   // BootXXXX not define
                if(bEnums)
                    Data.SetError();
                break;

            case ERROR_INSUFFICIENT_BUFFER:
                Data.Resize(Data.GetSize() * 2);
                loop = true;
                break;
        }
    } while (loop);

    return length;
}

bool UefiVarHandling::SetFirmVar(const twstring &VarName, const twstring &inGUID, DynData &Data)
{
    bool ret = false;
    BOOL ok = SetFirmwareEnvironmentVariable(VarName.c_str(), inGUID.c_str(), Data.GetMem(), Data.GetSize());
    if(!ok)
        nStatus = GetLastError();
    else
        nStatus = NO_ERROR;


    switch (nStatus)
    {
    case NO_ERROR:
        ret = true;
        break;

    case ERROR_NOACCESS:           // Guid is unknown
    case STATUS_INVALID_PARAMETER: // wrong parameter wrong varname?
    case ERROR_INVALID_FUNCTION:   // it is not uefi
    case ERROR_PRIVILEGE_NOT_HELD:
    case ERROR_INSUFFICIENT_BUFFER:
        break;
    }

    return ret;
}

UefiVarHandling::DynData UefiVarHandling::GetFirmEnvVar(const twstring &VarName)
{
    DynData Data;

    GetFirmVar(VarName, L"{8be4df61-93ca-11d2-aa0d-00e098032b8c}", Data, true);

    return Data;
}

bool UefiVarHandling::SetFirmEnvVar(const twstring &VarName, DynData &Data)
{
    return SetFirmVar(VarName, L"{8be4df61-93ca-11d2-aa0d-00e098032b8c}", Data);
}


int UefiVarHandling::GetBootVariable(const twstring &VarName)
{
    DynData Data(0);

    Data = GetFirmEnvVar(VarName);

    if (Data)
        return  *(UINT16 *) Data.GetMem();

    return -1;
}


bool UefiVarHandling::SetBootVariable(const twstring &VarName, UINT16 Num)
{
    DynData Data(2);

    Data.SetMem(Num);

    return SetFirmEnvVar(VarName, Data);
}

bool UefiVarHandling::DeleteBootVariable(const twstring &VarName)
{
    DynData Data(0);

    return SetFirmEnvVar(VarName, Data);
}



tvInt UefiVarHandling::GetOrderVariable(const twstring &VarName)
{
    DynData Data(0);
    tvInt bo;
    const DWORD i16 = sizeof(INT16);


    Data = GetFirmEnvVar(VarName);

    if (Data)
    {
        for(DWORD i = 0; i < Data.GetSize(i16); ++i) 
            bo.push_back(*(INT16 *) Data.GetMem(i16 * i));
    }

    return  bo;
}


bool UefiVarHandling::SetOrderVariable(const twstring &VarName, tvInt vInts)
{
    DynData Data((DWORD)vInts.size() * sizeof(UINT16));
    DWORD inx = 0;

    for(int i : vInts)                                           
        inx += Data.SetMem(((UINT16) i), inx);
        

    return SetFirmEnvVar(VarName, Data);
}



UefiVarHandling::tvMEFI_LOAD_OPTION UefiVarHandling::EnumVariableData(const twstring &VarName, bool bAll)
{
    DynData Data;
    tvMEFI_LOAD_OPTION strs;

    for (UINT16 i = 0; i < 0xffff; ++i)
    {
        twstring vn(VarName);
        vn += GetHex(i);

        if (GetFirmVar(vn.c_str(), L"{8be4df61-93ca-11d2-aa0d-00e098032b8c}", Data, false) > 0)
        {
            EFI_LOAD_OPTION *elo = (EFI_LOAD_OPTION *)Data.GetMem();
            MEFI_LOAD_OPTION melo;
            melo.Description = elo->Description;
            melo.Attributes  = elo->Attributes;
            strs.push_back(melo);
        }
        else if(!bAll)
            break;
        
        if(Data.GetSize() == 0)
            break;
    }

    if(strs.size() > 0)
        GetState();      // erase the last error, we getting always 

    return strs;
}




    


/*


ExGetFirmwareEnvironmentVariable routine
The ExGetFirmwareEnvironmentVariable routine gets the value of the specified system firmware environment variable.

Syntax

NTSTATUS ExGetFirmwareEnvironmentVariable(
    _In_      PUNICODE_STRING VariableName,
    _In_      LPGUID          VendorGuid,
    _Out_opt_ PVOID           Value,
    _Inout_   PULONG          ValueLength,
    _Out_opt_ PULONG          Attributes
);

Parameters
VariableName[in]
A pointer to a UNICODE_STRING structure that contains the name of the specified environment variable.

VendorGuid[in]
A pointer to a GUID that identifies the vendor associated with the specified environment variable.Environment variables are grouped into namespaces based on their vendor GUIDs.Some hardware platforms might not support vendor GUIDs.On these platforms, all variables are grouped into one, common namespace, and the VendorGuid parameter is ignored.

Value[out, optional]
A pointer to a caller - allocated buffer to which the routine writes the value of the specified environment variable.

ValueLength[in, out]
A pointer to a location that contains the buffer size.On entry, the location pointed to by this parameter contains the size, in bytes, of the caller - supplied Value buffer.Before exiting, the routine writes to this location the size, in bytes, of the variable value.If the routine returns STATUS_SUCCESS, the *ValueLength output value is the number of bytes of data written to the Value buffer.If the routine returns STATUS_BUFFER_TOO_SMALL, *ValueLength is the required buffer size.

Attributes[out, optional]
A pointer to a location to which the routine writes the attributes of the specified environment variable.This parameter is optional and can be set to NULL if the caller does not need the attributes.For more information, see Remarks.

Return value
ExGetFirmwareEnvironmentVariable returns STATUS_SUCCESS if it is successful.Possible return values include the following error status codes.

Return code	Description
STATUS_INSUFFICIENT_RESOURCES
Available system resources are insufficient to complete the requested operation.

STATUS_BUFFER_TOO_SMALL
The Value buffer is too small.

STATUS_VARIABLE_NOT_FOUND
The requested variable does not exist.

STATUS_INVALID_PARAMETER
One of the parameters is not valid.

STATUS_NOT_IMPLEMENTED
This routine is not supported on this platform.

STATUS_UNSUCCESSFUL
The firmware returned an unrecognized error.








Sets the value of the specified firmware environment variable.

Syntax
C++

Kopieren
BOOL SetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID  pValue,
    DWORD  nSize
);
Parameters
lpName

The name of the firmware environment variable.The pointer must not be NULL.

lpGuid

The GUID that represents the namespace of the firmware environment variable.The GUID must be a string in the format "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".If the system does not support GUID - based namespaces, this parameter is ignored.

pValue

A pointer to the new value for the firmware environment variable.

nSize

The size of the pBuffer buffer, in bytes.If this parameter is zero, the firmware environment variable is deleted.

Return Value
If the function succeeds, the return value is a nonzero value.

If the function fails, the return value is zero.To get extended error information, call GetLastError.Possible error codes include ERROR_INVALID_FUNCTION.










ret = 118
0x00000032F52FE1D0 01 00 00 00 62 00 55 00 62 00 75 00 6e 00 74 00 75 00 00 00 04 01 ....b.U.b.u.n.t.u.....
0x00000032F52FE1E6 2a 00 01 00 00 00 00 08 00 00 00 00 00 00 00 f0 02 00 00 00 00 00 *..............ð......
0x00000032F52FE1FC 22 db ae a1 90 dc 72 45 97 05 ef 14 9b 75 88 af 02 02 04 04 34 00 "Û®¡.ÜrE—.ï..uˆ¯....4.
0x00000032F52FE212 5c 00 45 00 46 00 49 00 5c 00 75 00 62 00 75 00 6e 00 74 00 75 00 \.E.F.I.\.u.b.u.n.t.u.
0x00000032F52FE228 5c 00 67 00 72 00 75 00 62 00 78 00 36 00 35 00 2e 00 65 00 66 00 \.g.r.u.b.x.6.5...e.f.
0x00000032F52FE23E 69 00 00 00 7f ff 04 00 cc cc cc cc cc cc cc cc cc cc cc cc cc cc i....ÿ..ÌÌÌÌÌÌÌÌÌÌÌÌÌÌ


ret = 124
0x000000D86995DFC0 01 00 00 00 62 00 47 00 72 00 75 00 62 00 20 00 4d 00 65 00 6e 00 ....b.G.r.u.b. .M.e.n.
0x000000D86995DFD6 75 00 00 00 04 01 2a 00 01 00 00 00 00 08 00 00 00 00 00 00 00 f0 u.....*..............ð
0x000000D86995DFEC 02 00 00 00 00 00 22 db ae a1 90 dc 72 45 97 05 ef 14 9b 75 88 af ......"Û®¡.ÜrE—.ï..uˆ¯
0x000000D86995E002 02 02 04 04 34 00 5c 00 45 00 46 00 49 00 5c 00 75 00 62 00 75 00 ....4.\.E.F.I.\.u.b.u.
0x000000D86995E018 6e 00 74 00 75 00 5c 00 67 00 72 00 75 00 62 00 78 00 36 00 34 00 n.t.u.\.g.r.u.b.x.6.4.
0x000000D86995E02E 2e 00 65 00 66 00 69 00 00 00 7f ff 04 00 cc cc cc cc cc cc cc cc ..e.f.i....ÿ..ÌÌÌÌÌÌÌÌ

ret = 270
0x0000008A982FE690 01 00 00 00 74 00 57 00 69 00 6e 00 31 00 30 00 00 00 04 01 2a 00 ....t.W.i.n.1.0.....*.
0x0000008A982FE6A6 01 00 00 00 00 08 00 00 00 00 00 00 00 f0 02 00 00 00 00 00 22 db .............ð......"Û
0x0000008A982FE6BC ae a1 90 dc 72 45 97 05 ef 14 9b 75 88 af 02 02 04 04 46 00 5c 00 ®¡.ÜrE—.ï..uˆ¯....F.\.
0x0000008A982FE6D2 45 00 46 00 49 00 5c 00 4d 00 69 00 63 00 72 00 6f 00 73 00 6f 00 E.F.I.\.M.i.c.r.o.s.o.
0x0000008A982FE6E8 66 00 74 00 5c 00 42 00 6f 00 6f 00 74 00 5c 00 62 00 6f 00 6f 00 f.t.\.B.o.o.t.\.b.o.o.
0x0000008A982FE6FE 74 00 6d 00 67 00 66 00 77 00 2e 00 65 00 66 00 69 00 00 00 7f ff t.m.g.f.w...e.f.i....ÿ
0x0000008A982FE714 04 00 57 49 4e 44 4f 57 53 00 01 00 00 00 88 00 00 00 78 00 00 00 ..WINDOWS.....ˆ...x...
0x0000008A982FE72A 42 00 43 00 44 00 4f 00 42 00 4a 00 45 00 43 00 54 00 3d 00 7b 00 B.C.D.O.B.J.E.C.T.=.{.
0x0000008A982FE740 39 00 64 00 65 00 61 00 38 00 36 00 32 00 63 00 2d 00 35 00 63 00 9.d.e.a.8.6.2.c.-.5.c.
0x0000008A982FE756 64 00 64 00 2d 00 34 00 65 00 37 00 30 00 2d 00 61 00 63 00 63 00 d.d.-.4.e.7.0.-.a.c.c.
0x0000008A982FE76C 31 00 2d 00 66 00 33 00 32 00 62 00 33 00 34 00 34 00 64 00 34 00 1.-.f.3.2.b.3.4.4.d.4.
0x0000008A982FE782 37 00 39 00 35 00 7d 00 00 00 31 00 01 00 00 00 10 00 00 00 04 00 7.9.5.}...1...........
0x0000008A982FE798 00 00 7f ff 04 00 cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc ...ÿ..ÌÌÌÌÌÌÌÌÌÌÌÌÌÌÌÌ



*/




