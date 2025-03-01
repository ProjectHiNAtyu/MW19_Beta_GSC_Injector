#include "dllmain.h"
#include <sys/stat.h>
#include "xor.h"
#include <atlstr.h>
#include "skcrypt.hpp"
#include <future>
#include <vector>
#include <string>
#include <shlobj.h>
#include <locale>
#include <codecvt>

#include <windows.h>

#include <iostream>
#include <fstream>
#include <shlobj.h>
#include <filesystem>
//#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "common/utils/hook.hpp"
using namespace std;

#pragma region --------------- [ Enums ] ---------------

enum Language
{
    ENGLISH = 0,
    JAPANESE
};


enum GameTitle
{
    UnknownGame     = 0,

    MW2019_BNet     = 1,
    MW2019_Steam    = 2,
    MW2019_Donetsk  = 20,
    MW2019_138      = 21,
    MW2019_Beta     = 22,

    Vanguard_BNet   = 3,
    Vanguard_Steam  = 4,

    MW22_SP_BNet    = 5,
    MW22_SP_Steam   = 6,
    MW22_MP_BNet    = 7,
    MW22_MP_Steam   = 8,

    CoDHQ_BNet      = 9 ,
    CoDHQ_Steam     = 10 ,

    BO6_Steam       = 11 ,
    BO6_BNet        = 12 ,
    BO6_GP          = 13 ,

    MW23_MP_Steam   = 14 ,
    MW23_MP_BNet    = 15 ,
    MW23_MP_GP      = 16 ,
    MW23_SP_Steam   = 17 ,
    MW23_SP_BNet    = 18 ,
    MW23_SP_GP      = 19 ,
};



#pragma region --------------- [ Load_ScriptFile ] ---------------

enum DBStreamStart : __int32
{
    AtStart = 0x0,
    NotAtStart = 0x1,
};

struct ScriptFile
{
    const char* name; // correct
    int compressedLen; // correct
    int len;
    int bytecodeLen; // correct
    const char* buffer; // correct
    unsigned __int8* bytecode; // correct
};



#pragma endregion



#pragma endregion


#pragma region --------------- [ Public variable ] ---------------



struct GscOffsets
{
    uintptr_t DumpBase;

    uintptr_t GetActiveServerApplication;
    uintptr_t SvGameModeAppMP_ServerStart_Spawn;
    uintptr_t s_svMainMP_serverProcessControl_settings;

    uintptr_t Party_GetActiveParty;
    uintptr_t PartyHost_RequestStartMatch;
    uintptr_t PartyHost_GamestateChanged;
    uintptr_t FUN_7ff66ce717c0;

    uintptr_t xpartydisband;
    uintptr_t GScr_EndLobby;

    uintptr_t NetConstStrings_GetIndexPlusOneFromName;
    uintptr_t NetConstStrings_GetNameFromIndexPlusOne;

    uintptr_t Dvar_GetStringSafe;
    uintptr_t Dvar_SetBool_Internal;
    
    uintptr_t LUI_CoD_Shutdown;
    uintptr_t LUI_CoD_Init;
        
    uintptr_t xblive_privatematch;
    uintptr_t xblive_privatematch_solo;
    uintptr_t force_offline_menus;
    uintptr_t force_offline_enabled;
    uintptr_t lui_force_online_menus;
    uintptr_t onlinegame;
    uintptr_t lui_dev_features_enabled;
    uintptr_t online_auth_skip_auth;
    uintptr_t disable_warzone_private_match;
    uintptr_t systemlink;
    uintptr_t xblive_loggedin;
    
    uintptr_t Load_ScriptFile;
    uintptr_t DB_PatchMem_PushAsset;
    uintptr_t Load_Stream;
    uintptr_t DB_PushStreamPos;
    uintptr_t DB_PopStreamPos;
    uintptr_t DB_PatchMem_PopAsset;
    uintptr_t DB_ReadXFile;
    uintptr_t Load_ConstCharArray;
    uintptr_t Load_byteArray;
    uintptr_t varScriptFile;
    uintptr_t varConstChar;
    uintptr_t varbyte;
    uintptr_t AllocLoad_ConstChar;
    uintptr_t AllocLoad_byte;
    uintptr_t g_streamPosGlob_pos;
    uintptr_t Load_XString;
    uintptr_t varXString;
    uintptr_t varHashName;
    uintptr_t Load_HashName;

    uintptr_t playerCountOffset;
    uintptr_t inGameOffset;
};

GscOffsets _adr;
const auto ImageBase = (uintptr_t)GetModuleHandle(nullptr);
static GameTitle currentGame = GameTitle::UnknownGame;
std::string gscPath;

#pragma endregion

#pragma region --------------- [ Utility functions ] ---------------

void ReplaceAll(std::string& stringreplace, const std::string& origin, const std::string& dest)
{
    size_t pos = 0;
    size_t offset = 0;
    size_t len = origin.length();
    // 指定文字列が見つからなくなるまでループする
    while ((pos = stringreplace.find(origin, offset)) != std::string::npos) {
        stringreplace.replace(pos, len, dest);
        offset = pos + dest.length();
    }
}

inline bool file_exists(const char* name)
{
    struct stat buffer;
    return (stat(name, &buffer) == 0);
}

bool isSubStr(std::string str, std::string subStr)
{
    size_t pos = str.find(subStr);
    if (pos != std::string::npos)
    {
        return true;
    }
    return false;
}

void dump_gsc_script(std::ofstream& stream, ScriptFile* scriptfile)
{
    std::string buffer;
    buffer.append(scriptfile->name, strlen(scriptfile->name) + 1);

    buffer.append(reinterpret_cast<char*>(&scriptfile->compressedLen), 4);
    buffer.append(reinterpret_cast<char*>(&scriptfile->len), 4);
    buffer.append(reinterpret_cast<char*>(&scriptfile->bytecodeLen), 4);
    buffer.append(scriptfile->buffer, scriptfile->compressedLen);
    buffer.append((char*)scriptfile->bytecode, scriptfile->bytecodeLen);

    stream << buffer;
}


void NotepadLog(const char* fmt, ...)
{
    va_list ap;
    HWND notepad, edit;
    char buf[256];

    va_start(ap, fmt);
    vsprintf_s(buf, fmt, ap);
    va_end(ap);


    notepad                 = FindWindow(NULL, "Untitled - Notepad");
    if (!notepad) { notepad = FindWindow(NULL, ("*Untitled - Notepad")); }
    if (!notepad) { notepad = FindWindowW(NULL, L"無題 - メモ帳"); }
    if (!notepad) { notepad = FindWindowW(NULL, L"*無題 - メモ帳"); }
    if (!notepad) { notepad = FindWindowW(NULL, L"test.txt - メモ帳"); }
    if (!notepad) { notepad = FindWindowW(NULL, L"*test.txt - メモ帳"); }



    if (!notepad)
        return;

    edit = FindWindowEx(notepad, NULL, "Edit", NULL);
    SendMessageA(edit, EM_REPLACESEL, 0, (LPARAM)buf);
    //SendMessage(edit, WM_SETTEXT, 0, (LPARAM)_T(buf));
}

#pragma endregion


#pragma region --------------- [ MW19 Load_ScriptFile functions ] ---------------

inline utils::hook::detour MW19_MP_Load_ScriptFile;
void __fastcall Detour_MW19_MP_Load_ScriptFile(DBStreamStart streamStart)
{
    switch (currentGame)
    {
        case GameTitle::MW2019_BNet:    NotepadLog("[MW19_Bn] 'gsc_func' called. -> ");    break;
        case GameTitle::MW2019_Steam:   NotepadLog("[MW19_Stm] 'gsc_func' called. -> "); break;
        case GameTitle::MW2019_138:     NotepadLog("[MW2019_138] 'gsc_func' called. -> "); break;
        case GameTitle::MW2019_Beta:    NotepadLog("[MW2019_Beta] 'gsc_func' called. -> "); break;
    }

    // ========================================================================================== //
    // for Dump
    // ========================================================================================== //
    
//    MW19_MP_Load_ScriptFile.stub<void>(streamStart);
//
//	ScriptFile **varScriptFile	= reinterpret_cast<ScriptFile **>( ImageBase + ( _adr.varScriptFile - _adr.DumpBase ) );
//
//    ScriptFile* scriptfile = *varScriptFile;
//
//    std::string directory;
//
//    std::string scriptFileStr = "";
//    if (isSubStr(scriptfile->name, ".gsc"))
//    {
//        scriptFileStr = gscPath + "gsc_dump\\" + std::string(scriptfile->name) + "bin";
//    }
//    else
//    {
//        scriptFileStr = gscPath + "gsc_dump\\" + std::string(scriptfile->name) + ".gscbin";
//    }
//    
//    ReplaceAll(scriptFileStr, "/", "\\");
//    NotepadLog("     target dump path '%s' -> ", scriptFileStr.c_str());
//    
//    size_t lastSlash = scriptFileStr.find_last_of("\\");
//    if (lastSlash != std::string::npos && isSubStr(scriptFileStr, "\\"))
//    {
//        directory = scriptFileStr.substr(0, lastSlash);
//    
//        std::filesystem::create_directories(directory);
//    }
//    
//    std::ofstream gscbin_file(scriptFileStr, std::ios::out | std::ios::binary);
//    if (gscbin_file.is_open())
//    {
//        NotepadLog("gsc dumping!");
//        dump_gsc_script(gscbin_file, scriptfile);
//        gscbin_file.close();
//    }
//    NotepadLog("\n");


    // ========================================================================================== //
    // for Inject
    // ========================================================================================== //

    auto DB_PatchMem_PushAsset = reinterpret_cast<void(*)(size_t len, ScriptFile * script)>(ImageBase + (_adr.DB_PatchMem_PushAsset - _adr.DumpBase));
    auto Load_Stream = reinterpret_cast<void(*)(int streamStart, void* ptr, size_t size)>(ImageBase + (_adr.Load_Stream - _adr.DumpBase));
    auto DB_PushStreamPos = reinterpret_cast<void(*)(int param_1)>(ImageBase + (_adr.DB_PushStreamPos - _adr.DumpBase));
    auto Load_XString = reinterpret_cast<void(*)(int param_1)>(ImageBase + (_adr.Load_XString - _adr.DumpBase));
    auto DB_PopStreamPos = reinterpret_cast<void(*)(void)>(ImageBase + (_adr.DB_PopStreamPos - _adr.DumpBase));
    auto DB_PatchMem_PopAsset = reinterpret_cast<void(*)(void)>(ImageBase + (_adr.DB_PatchMem_PopAsset - _adr.DumpBase));
    auto DB_ReadXFile = reinterpret_cast<void(*)(void* ptr, size_t size)>(ImageBase + (_adr.DB_ReadXFile - _adr.DumpBase));
    auto Load_ConstCharArray = reinterpret_cast<void(*)(void* ptr, size_t size)>(ImageBase + (_adr.Load_ConstCharArray - _adr.DumpBase));
    auto Load_byteArray = reinterpret_cast<void(*)(void* ptr, size_t size)>(ImageBase + (_adr.Load_byteArray - _adr.DumpBase));
    ScriptFile** varScriptFile = reinterpret_cast<ScriptFile**>(ImageBase + (_adr.varScriptFile - _adr.DumpBase));
    char** varXString = reinterpret_cast<char**>(ImageBase + (_adr.varXString - _adr.DumpBase));
    char** varConstChar = reinterpret_cast<char**>(ImageBase + (_adr.varConstChar - _adr.DumpBase));
    char** varbyte = reinterpret_cast<char**>(ImageBase + (_adr.varbyte - _adr.DumpBase));
    char** AllocLoad_ConstChar = reinterpret_cast<char**>(ImageBase + (_adr.AllocLoad_ConstChar - _adr.DumpBase));
    char** AllocLoad_byte = reinterpret_cast<char**>(ImageBase + (_adr.AllocLoad_byte - _adr.DumpBase));
    char** g_streamPosGlob_pos = reinterpret_cast<char**>(ImageBase + (_adr.g_streamPosGlob_pos - _adr.DumpBase));

    char* backup;
    ScriptFile* scriptfile;


    DB_PatchMem_PushAsset(0x34, *varScriptFile);
    Load_Stream(streamStart, *varScriptFile, sizeof(ScriptFile));
    DB_PushStreamPos(5);

    char* xStringBackup = *varXString;
    *varXString = reinterpret_cast<char*>(*varScriptFile);
    Load_XString(1);
    *varXString = xStringBackup;
    DB_PushStreamPos(6);


    scriptfile = *varScriptFile;

    backup = *varConstChar;

    std::string filepath = gscPath + "script.gscbin";
    std::string scriptname = "";
    std::string fixpath = "";
    std::string directory;
    bool scriptgscbin = false;
    bool lastScript = false;


    if (scriptfile)
    {
        scriptname = std::string(scriptfile->name);
        NotepadLog("scriptfile '%s' ->", scriptfile->name);

        if (isSubStr(scriptfile->name, ".gsc"))
        {
            filepath = gscPath + scriptname + "bin";
        }
        else // numbered scriptfiles like "1892"
        {
            filepath = gscPath + scriptname + ".gscbin";
        }

        ReplaceAll(filepath, "/", "\\");
        NotepadLog("gsc path '%s' -> ", filepath.c_str());

        if (file_exists(filepath.c_str()))
        {
            NotepadLog("custom gscbin found!");
            scriptgscbin = true;
        }

        NotepadLog("\n");

        if (!strcmp(scriptfile->name, "scripts/mp/gametypes/war.gsc")) // trial war
        {
            lastScript = true;
        }

    }

    if (file_exists(filepath.c_str()) && scriptgscbin)
    {
        std::ifstream script;
        script.open(filepath, std::ios::binary | std::ios::ate);
        int size = (int)script.tellg();
        script.seekg(0, std::ios::beg);

        char* customScript = new char[size];
        script.read(customScript, size);
        script.seekg(0, std::ios::beg);

        while (script.get() != '\0'); // read past the name
        int vars[3] = { 0 };
        script.read((char*)vars, sizeof(int) * 3); //read header info

        if (scriptfile->buffer != NULL)
        {
            *varConstChar = *g_streamPosGlob_pos;
            scriptfile->buffer = *varConstChar;

            char* dummyMem = new char[scriptfile->compressedLen];
            DB_ReadXFile(dummyMem, scriptfile->compressedLen);
            delete[scriptfile->compressedLen] dummyMem;

            memmove(*g_streamPosGlob_pos, customScript + (int)script.tellg(), vars[0]);

            *g_streamPosGlob_pos = *g_streamPosGlob_pos + vars[0];
            scriptfile->compressedLen = vars[0];
        }
        *varConstChar = backup;
        scriptfile->len = vars[1];

        DB_PopStreamPos();
        DB_PushStreamPos(6);

        scriptfile = *varScriptFile;
        backup = *varbyte;

        if (scriptfile->bytecode != NULL)
        {
            *varbyte = *g_streamPosGlob_pos;
            scriptfile->bytecode = (unsigned char*)*varbyte;

            char* dummyMem = new char[scriptfile->bytecodeLen];
            DB_ReadXFile(dummyMem, scriptfile->bytecodeLen);
            delete[scriptfile->bytecodeLen] dummyMem;

            memmove(*g_streamPosGlob_pos, customScript + vars[0] + (int)script.tellg(), vars[2]);

            *g_streamPosGlob_pos = *g_streamPosGlob_pos + vars[2];
            scriptfile->bytecodeLen = vars[2];
        }
        *varbyte = backup;

        delete[size] customScript;
        script.close();

        NotepadLog("custom gscbin injected!! %s\n", scriptfile->name);
    }
    else
    {
        if (scriptfile->buffer != NULL)
        {
            *varConstChar = *g_streamPosGlob_pos;
            scriptfile->buffer = *varConstChar;
            Load_Stream(0, *varConstChar, scriptfile->compressedLen);
        }
        *varConstChar = backup;

        DB_PopStreamPos();
        DB_PushStreamPos(6);

        scriptfile = *varScriptFile;
        backup = *varbyte;
        if (scriptfile->bytecode != NULL)
        {
            *varbyte = *g_streamPosGlob_pos;
            scriptfile->bytecode = (unsigned char*)*varbyte;
            Load_Stream(0, *varbyte, scriptfile->bytecodeLen);
        }
        *varbyte = backup;
    }

    DB_PopStreamPos();
    DB_PopStreamPos();
    DB_PatchMem_PopAsset();


    if (lastScript)
    {
        MW19_MP_Load_ScriptFile.clear();
        NotepadLog("[* Notice] Success remove a hook for gsc_func\n");
    }
}

#pragma endregion


#pragma region --------------- [ MW19 Construction functions ] ---------------

int MW19_Construction( )
{
    TCHAR waFolderPath[MAX_PATH];
    SHGetSpecialFolderPath( NULL , waFolderPath , CSIDL_PERSONAL , 0 );
    gscPath = (std::string)waFolderPath + "\\MW19_BETA_GSC_CUSTOM\\";

    string titlename = "";
    switch (currentGame)
    {
        case GameTitle::MW2019_BNet:    titlename = "MW2019_BNet"; break;
        case GameTitle::MW2019_Steam:   titlename = "MW2019_Stm"; break;
        case GameTitle::MW2019_Donetsk: titlename = "MW2019_Donetsk"; break;
        case GameTitle::MW2019_138:     titlename = "MW2019_138"; break;
        case GameTitle::MW2019_Beta:    titlename = "MW2019_Beta"; break;
    }

    MW19_MP_Load_ScriptFile.create(ImageBase + (_adr.Load_ScriptFile - _adr.DumpBase), Detour_MW19_MP_Load_ScriptFile);
    NotepadLog("[%s] Success Enable the hook for gsc_func\n", titlename);


    return 0;
}

#pragma endregion


#pragma region --------------- [ AIO functions ] ---------------

bool GetAddressOffset( )
{
    switch ( currentGame )
    {
        
        case MW2019_BNet:
        case MW2019_Steam:
        case Vanguard_BNet:
        case MW22_MP_Steam:
        case MW23_MP_GP:
        case BO6_GP:
        case MW2019_Donetsk:
        case MW2019_138:
        case MW2019_Beta:
            NotepadLog( "[+ OK] " );
            break;

        default:
            NotepadLog( "[# Failed] Unsupported game for GetAddressOffset\n" );
            return false;
    }

    switch ( currentGame )
    {
        case MW2019_Beta:
        {
            NotepadLog( "<MW2019_Beta> " );
            _adr.DumpBase                       = 0x7FF658340000;

            _adr.Load_ScriptFile                = 0x7FF6585ADBB0;
            _adr.DB_PatchMem_PushAsset          = 0x7FF658539E10;
            _adr.Load_Stream                    = 0x7FF65892BA30;
            _adr.DB_PushStreamPos               = 0x7FF65892B620;
            _adr.Load_XString                   = 0x7FF6585819B0;
            _adr.DB_PopStreamPos                = 0x7FF65892B570;
            _adr.DB_PatchMem_PopAsset           = 0x7FF658539E00;
            _adr.DB_ReadXFile                   = 0x7FF658923070;
            _adr.Load_ConstCharArray            = 0x7FF65857FC50;
            _adr.Load_byteArray                 = 0x7FF658581E00;
            _adr.varScriptFile                  = 0x7FF65DA30740;
            _adr.varXString                     = 0x7FF65DA2ECB8;
            _adr.varConstChar                   = 0x7FF65DA2ECA8;
            _adr.varbyte                        = 0x7FF65DA2EAC8;
            _adr.AllocLoad_ConstChar            = 0x7FF65857DC70;
            _adr.AllocLoad_byte                 = 0x7FF65857DF70;
            _adr.g_streamPosGlob_pos            = 0x7FF6641DFC50;
        }
            break;
    }

    NotepadLog( "Successfully running GetAddressOffset( ).\n" );
    NotepadLog( "\n" );
    return true;
}
#pragma endregion


#pragma region --------------- [ Init & Base functions ] ---------------


DWORD WINAPI initthread(HMODULE hModule)
{
    /* ==========[ Modern Warfare 2019 ]==========
    */
    //currentGame = GameTitle::MW2019_BNet;
    //currentGame = GameTitle::MW2019_Steam;
    //currentGame = GameTitle::MW2019_Donetsk;
    currentGame = GameTitle::MW2019_Beta;
    GetAddressOffset( );
    MW19_Construction( );


	return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
    case DLL_PROCESS_ATTACH:
        initthread(module);
        break;

	case DLL_PROCESS_DETACH:

		break;
	}
	return TRUE;
}

#pragma endregion







/*
* 
[+ OK] <MW2019_Beta> Successfully running GetAddressOffset( ).

[MW2019_Beta] Success Enable the hook for gsc_func
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1212.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1214.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\16.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1622.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1633.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1773.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1774.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1775.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1776.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1777.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1778.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1780.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1781.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1782.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1784.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1785.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1786.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1787.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1789.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1799.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1800.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1801.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1804.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1805.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1806.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1808.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1809.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1810.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1812.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1813.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1814.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1815.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1816.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1817.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1818.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1819.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1820.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1821.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1822.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1823.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1824.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1825.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1826.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1827.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1828.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1829.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1830.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1831.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1832.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1833.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1834.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1835.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1836.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1837.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1838.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1839.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1840.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1841.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1842.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1843.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1844.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1845.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1846.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1847.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1848.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1849.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1850.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1851.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1852.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1853.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1854.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1855.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1856.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1857.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1858.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1859.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1860.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1861.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1862.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1863.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1864.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1865.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1866.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1867.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1868.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1869.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1870.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1871.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1872.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1874.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1875.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1876.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1877.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1878.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1879.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1880.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1881.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1882.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1883.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1884.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1885.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1886.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1887.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1888.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1889.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1890.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1891.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1892.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1893.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1894.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1896.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1897.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1898.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1899.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1900.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1901.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1902.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1903.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1904.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1905.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1906.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1907.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1908.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1909.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1910.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1911.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1912.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1913.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1914.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\1915.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2064.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2213.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2214.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2215.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2216.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2217.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2218.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2219.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2220.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2221.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2222.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2223.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2224.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2226.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2227.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2228.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2229.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2230.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2231.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2232.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2234.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2237.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2238.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2240.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2241.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2242.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2243.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2244.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2245.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2246.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2247.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2248.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2249.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2250.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2251.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2252.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2253.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2254.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2255.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2256.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2257.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2258.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2259.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2260.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2261.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2262.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2263.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2264.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2265.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2274.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2275.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2276.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2277.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2278.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2286.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2287.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2288.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2289.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2290.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2291.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2292.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2293.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2294.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2295.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2296.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2297.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2298.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2299.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2300.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2301.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2302.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2303.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2304.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2305.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2306.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2307.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2308.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2309.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2310.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2311.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2312.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2321.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2322.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2323.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2324.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2325.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2326.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2327.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2328.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2329.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2330.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2331.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2332.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2333.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2334.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2335.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2336.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2337.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2338.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2339.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2340.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2341.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2342.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2343.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2344.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2345.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2346.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2347.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2348.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2349.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2350.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2351.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2352.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2353.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2354.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2355.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2356.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2357.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2358.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2359.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2360.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2361.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2362.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2363.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2364.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2365.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2366.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2367.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2368.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2369.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2370.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2371.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2372.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2373.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2374.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2375.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2376.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2379.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2380.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2381.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2382.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2383.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2384.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2385.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2386.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2387.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2388.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2389.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2390.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2391.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2392.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2393.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2394.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2395.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2396.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2397.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2398.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2399.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2400.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2401.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2402.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2403.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2404.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2405.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2406.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2407.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2408.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2409.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2410.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2411.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2412.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2413.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2414.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2415.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2416.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2417.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2418.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2419.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2420.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2421.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2422.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2423.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2424.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2425.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2426.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2427.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2428.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2429.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2430.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\2431.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_arena.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_arm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_assault.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_ball.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_blitz.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_br.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_btm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_cmd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_conf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_cranked.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_ctf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_cyber.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_dd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_defcon.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_dm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_dom.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_front.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_grind.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_grnd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_gun.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_hq.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_hvt.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_infect.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_koth.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_lava.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_mtmc.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_mugger.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_payload.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_pill.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_rush.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_sd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_siege.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_sotf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_sotf_ffa.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_sr.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_tac_ops.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_tdef.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_tjugg.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_air.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_bhd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_blitz.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_conf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_dd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_hstg.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_sam.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_to_wmd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_vip.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\agents\gametype_war.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_arena.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_arm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_assault.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_ball.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_blitz.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_br.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_btm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_cmd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_common.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_conf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_cranked.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_ctf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_cyber.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_dd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_defcon.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_dm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_dom.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_front.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_grind.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_grnd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_gun.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_hq.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_hvt.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_infect.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_koth.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_lava.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_mtmc.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_mugger.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_payload.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_pill.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_rush.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_sd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_siege.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_sotf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_sotf_ffa.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_sr.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_tac_ops.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_tdef.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_tjugg.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_air.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_bhd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_blitz.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_conf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_dd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_hstg.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_sam.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_to_wmd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_vip.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\bots\bots_gametype_war.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\arena.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\arena_alt.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\arm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\assault.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\ball.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\blitz.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_adventure_quest.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_analytics.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_armor.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_assassination_quest.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_bank.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_c130.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_calloutmarkerping.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_circle.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_convoy.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_dom_quest.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_extract_chopper.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_functional_poi.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_gulag.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_helicopters.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_infils.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_killstreaks.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_mobile_armory.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_nuke.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_perks.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_pickups.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_plunder.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_quest_util.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_respawn.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_rewards.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_satellite_truck.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_scavenger_quest.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_scene_management.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_spectate.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_vehicles.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\br_weapons.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\bradley_spawner.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\btm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\cmd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\common.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\conf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\cranked.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\ctf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\cyber.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\dd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\dm.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\dom.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\exfil.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\front.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\grind.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\grnd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\gun.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\hq.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\hvt.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\infect.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\koth.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\lava.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\mtmc.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\mugger.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_ball.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_bombzone.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_capture.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_dogtag.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_dom.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_grindzone.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\obj_zonecapture.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\payload.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\pill.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\plunder.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\rugby.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\rush.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\sd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\siege.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\sotf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\sotf_ffa.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\sr.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\tac_ops.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\tdef.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\tjugg.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_air.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_bhd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_blitz.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_conf.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_dd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_hstg.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_sam.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\to_wmd.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\trial.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\vip.gscbin' -> gsc dumping!
[MW2019_Beta] 'gsc_func' called. ->      target dump path 'C:\Users\0726k\OneDrive\ドキュメント\MW19_BETA_GSC_CUSTOM\gsc_dump\scripts\mp\gametypes\war.gscbin' -> gsc dumping!


*/