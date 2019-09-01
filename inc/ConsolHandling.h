//-----------------------------------------------
// Module name: ConsolHandling
//-----------------------------------------------
// Description: This class handle the give calling
//				Parameter
//-----------------------------------------------

#include "pch.h"

#include "UefiVarHandling.h"


class ConsolHandling
{

public:

    ConsolHandling();

private:
    enum eState {eUnknown, eOnly, eOneStr, eOneInt, eMoreInt};  // eOnly = no more param | eOneString = next param must be String | eOneInt = next param must be a Initeger


    class ParamStruct;
    typedef std::vector<ParamStruct> tvParamStruct;
    typedef std::map<wchar_t, size_t> tmCommands;
    class ParamStruct
    {
    public:
         typedef int (ConsolHandling ::* tfunc) (ParamStruct &ps);

        ParamStruct(tfunc Cmd, eState State, twstring Descr);

        ParamStruct(tfunc Cmd = &ConsolHandling::Unknow) : func(Cmd), iState(eUnknown) {}


        void  SetParam(twstring param) { wStr = param; }
        void  SetParam(int param) { viIdx.push_back(param); }
        twstring  GetParam(void) { return wStr; }
        tvInt GetvParam(void) { return viIdx; }
        int  GetParam(int iIdx); 
        eState GetState(void) {return iState;}
        twstring  GetDesc(void) { return wDescrip; }

        int Call(void);

        static ConsolHandling *parent;

    private:
        twstring     wStr;
        tvInt        viIdx;
        tfunc        func;

        twstring     wDescrip;
        eState       iState;      


    };  // end  _ParamStruct


private:

    tvParamStruct  mParam;
    tvParamStruct  vParamStruct;
    tmCommands     mCommands;
    twstring       sName;
    UefiVarHandling ufh;

    void DefineParams(void);

public:
    bool Init(void);
    void ScanArgs(int argc, wchar_t *argv[]);
    int ExcuteCmd(void);
    void MapErrString(DWORD dErr, twstring &sMsg);


private:
    int Help(ParamStruct &ps);

    int Unknow(ParamStruct &ps);

    int Wrong(ParamStruct &ps);


    int GetBootVariable(const twstring &VarName, bool bView = true);

    int SetBootVariable(const twstring &VarName, INT16 value, bool bView = true);

    int DelBootVariable(const twstring &VarName);

    int GetBootNext(ParamStruct &ps);

    int SetBootNexti(ParamStruct &ps);

    int SetBootNextn(ParamStruct &ps);

    int DelBootNext(ParamStruct &ps);

    int GetBootNextView(void);

    int GetBootCurr(ParamStruct &ps);

    twstring GetOrderStr(tvInt vInt);

    int GetOrder(const twstring &VarName, bool bView = true);

    int SetBootOrder(const twstring &VarName, tvInt vInts);

    int GetOrderName(const twstring &VarName);

    int GetBootOrder(ParamStruct &ps);

    int GetDrivOrder(ParamStruct &ps);

    int GetDrivOrderView(void);

    int SetBootOrder(ParamStruct &ps);

    int SetDrivOrder(ParamStruct &ps);

    int GetBootOrNam(ParamStruct &ps);

    int GetDrivOrNam(ParamStruct &ps);

    int DelBootOrder(ParamStruct &ps);

    int DelDrivOrder(ParamStruct &ps);

    int GetTimeout(ParamStruct &ps);

    int GetTimeoutView(void);

    int SetTimeout(ParamStruct &ps);

    int DelTimeout(ParamStruct &ps);

    int EnumsVariable(const twstring &VarName, bool bAll, bool bView);

    int ListBoots(ParamStruct &ps);

    int ListBootsA(ParamStruct &ps);

    int ListDrivers(ParamStruct &ps);

    int ListDriversView(void);

    int ListDriversA(ParamStruct &ps);
};

