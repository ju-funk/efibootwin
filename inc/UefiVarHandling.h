// SetBootNext.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "pch.h"


const DWORD LOAD_OPTION_ACTIVE = 0x00000001;


class UefiVarHandling
{
private:
    struct deleter
    {
    private:
        bool notdel;

    public:
        deleter() : notdel(false) {}

        void NotDel() { notdel = true; }

        void operator()(BYTE *p)
        {
            if (!notdel)
                free(p);
        }

    };

    class DynData
    {
    private:
        DWORD nSize;
        std::shared_ptr<BYTE> bmem;

    public:
        DynData(DWORD size = 4096);

        void Resize(DWORD size);

        DynData operator=(const DynData &oth);
        operator bool() {return nSize > 0;}

        DWORD GetSize(DWORD len = 1) { return nSize / len; }

        void *GetMem(DWORD offset = 0) { return bmem.get() + offset; }

        template<class T>
        DWORD SetMem(T Val, DWORD offset = 0) 
        { 
            *(T *)GetMem(offset) = Val;

            return sizeof(T); 
        }

        void SetError() {Resize(0); }
    };

    DWORD   nStatus;
    HANDLE  hAccessToken;


    //*******************************************************
    // Can be used on any device handle to obtain generic 
    // path/location information concerning the physical device 
    // or logical device. If the handle does not logically map 
    // to a physical device, the handle may not necessarily support 
    // the device path protocol. The device path describes the 
    // location of the device the handle is for. The size of the 
    // Device Path can be determined from the structures that make 
    // up the Device Path.
    //*******************************************************
    typedef struct _EFI_DEVICE_PATH_PROTOCOL 
    {
        UINT8 Type;
        UINT8 SubType;
        UINT8 Length[2];
        UINT8 Data[2];        // kann auch länger sein
    } EFI_DEVICE_PATH_PROTOCOL;


    //*******************************************************
    // Each load option variable contains an EFI_LOAD_OPTION 
    // descriptor that is a byte packed buffer of variable 
    // length fields
    //*******************************************************
    typedef struct _EFI_LOAD_OPTION 
    {
        UINT32 Attributes;
        UINT16 FilePathListLength;                   // length of the bytes
        WCHAR  Description[1];
        // EFI_DEVICE_PATH_PROTOCOL FilePathList[];
        // UINT8 OptionalData[];                     // OptionalData starts at  offset sizeof(UINT32) + sizeof(UINT16) + 
                                                     //                         StrSize(Description) + FilePathListLength
                                                     // The number of bytes in OptionalData can be computed by subtracting 
                                                     //                         the starting offset of OptionalData from 
                                                     //                         total size in bytes of the EFI_LOAD_OPTION.

    } EFI_LOAD_OPTION;          



    std::map<DWORD, twstring> mErrMsg;

    bool SetPrivileg();

    void SetStateStrings(std::function<void(DWORD dErr, twstring &sMsg)> UserMsgFunc);


    DWORD GetFirmVar(const twstring &VarName, const twstring &inGUID, DynData &Data, bool bEnums);

    bool SetFirmVar(const twstring &VarName, const twstring &inGUID, DynData &Data);

public:
    typedef struct _MEFI_DEVICE_PATH_PROTOCOL
    {
        UINT8 Type;
        UINT8 SubType;
        UINT8 Length[2];
        std::vector<UINT8> Data;
    } MEFI_DEVICE_PATH_PROTOCOL;


    typedef struct _MEFI_LOAD_OPTION
    {
        UINT32 Attributes;
        twstring Description;
        std::vector<MEFI_DEVICE_PATH_PROTOCOL> vFilePathList;
        std::vector<UINT8> OptData;

        bool IsActiv(void) { return ((Attributes & LOAD_OPTION_ACTIVE) == LOAD_OPTION_ACTIVE); }
    } MEFI_LOAD_OPTION;


    typedef std::vector<MEFI_LOAD_OPTION> tvMEFI_LOAD_OPTION;

    UefiVarHandling() : nStatus(NO_ERROR), hAccessToken(nullptr) {}
    ~UefiVarHandling();

    static wchar_t* GetHex(UINT16 i);

    bool Init(std::function<void(DWORD dErr, twstring &sMsg)> UserMsgFunc);
    
    DWORD GetState(bool bReset = true);
    void  SetState(DWORD state) {nStatus = state;}
    twstring GetStateString();



    DynData GetFirmEnvVar(const twstring &VarName);

    bool SetFirmEnvVar(const twstring &VarName, DynData &Data);

    int GetBootVariable(const twstring &VarName);
    
    bool SetBootVariable(const twstring &VarName, UINT16 Num);

    bool DeleteBootVariable(const twstring &VarName);
    
    tvMEFI_LOAD_OPTION EnumVariableData(const twstring &VarName, bool bAll);

    tvInt GetOrderVariable(const twstring &VarName);
    
    bool SetOrderVariable(const twstring &VarName, tvInt vInts);
};



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




