// STEP_mp3.cpp : DLL 用の初期化処理の定義を行います。
//

#include "stdafx.h"
#include "STEP_mp3.h"
#include "Id3tagv1.h"
#include "Id3tagv2.h"
#include "RMP.h"
#include "Mp3Info.h"
#include "STEPlugin.h"

#include "DlgFileRmpID3v2.h"
#include "DlgDefaultValue.h"
#include "DlgConvID3v2Version.h"

#include "STEP_mp3_ConvFormat.h"
#include "..\SuperTagEditor\INI\ini.h"
//設定の読み書き
//WritePrivateProfileString はファイルが存在しない場合や、
//元ファイルが ANSI だと ANSI で文字列を書き込む
//使い辛いので STEP 本体の INI 読み書きクラスを使い回す
//UTF8/UTF16/ANSI 対応

#define STEP_API
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma pack(push, 1)
struct  ID3TAG  {                       // <<< ID3 Tag のデータ形式 >>>
    BYTE    byData[128];                // 128バイトデータ
};

#define ID3_LEN_TRACK_NAME      30      // トラック名    (文字列…30BYTE)
#define ID3_LEN_ARTIST_NAME     30      // アーティスト名(文字列…30BYTE)
#define ID3_LEN_ALBUM_NAME      30      // アルバム名    (文字列…30BYTE)
#define ID3_LEN_COMMENT         30      // コメント      (文字列…30BYTE)
#define ID3_LEN_YEAR            4       // リリース年号  (文字列… 4BYTE)

struct  ID3TAG_V10  {                           // <<< ID3 Tag v1.0 のデータ形式 >>>
    char    sTAG[3];                            // "TAG"         (文字列… 3BYTE)
    char    sTrackName[ID3_LEN_TRACK_NAME];     // トラック名    (文字列…30BYTE)
    char    sArtistName[ID3_LEN_ARTIST_NAME];   // アーティスト名(文字列…30BYTE)
    char    sAlbumName[ID3_LEN_ALBUM_NAME];     // アルバム名    (文字列…30BYTE)
    char    sYear[4];                           // リリース年号  (文字列… 4BYTE)
    char    sComment[ID3_LEN_COMMENT];          // コメント      (文字列…30BYTE)
    BYTE    byGenre;                            // ジャンル番号  (数字…… 1BYTE)
};

struct  ID3TAG_V11  {                           // <<< ID3 Tag v1.1 のデータ形式 >>>
    char    sTAG[3];                            // "TAG"         (文字列… 3BYTE)
    char    sTrackName[ID3_LEN_TRACK_NAME];     // トラック名    (文字列…30BYTE)
    char    sArtistName[ID3_LEN_ARTIST_NAME];   // アーティスト名(文字列…30BYTE)
    char    sAlbumName[ID3_LEN_ALBUM_NAME];     // アルバム名    (文字列…30BYTE)
    char    sYear[4];                           // リリース年号  (文字列… 4BYTE)
    char    sComment[ID3_LEN_COMMENT-2];        // コメント      (文字列…30BYTE)
    char    cZero;                              // '\0'          (文字列… 1BYTE)
    BYTE    byTrackNumber;                      // トラック番号  (数字…… 1BYTE)
    BYTE    byGenre;                            // ジャンル番号  (数字…… 1BYTE)
};
#pragma pack(pop)

//
//  メモ!
//
//      この DLL が MFC DLL に対して動的にリンクされる場合、
//      MFC 内で呼び出されるこの DLL からエクスポートされた
//      どの関数も関数の最初に追加される AFX_MANAGE_STATE
//      マクロを含んでいなければなりません。
//
//      例:
//
//      extern "C" BOOL PASCAL EXPORT ExportedFunction()
//      {
//          AFX_MANAGE_STATE(AfxGetStaticModuleState());
//          // 通常関数の本体はこの位置にあります
//      }
//
//      このマクロが各関数に含まれていること、MFC 内の
//      どの呼び出しより優先することは非常に重要です。
//      これは関数内の最初のステートメントでなければな
//      らないことを意味します、コンストラクタが MFC
//      DLL 内への呼び出しを行う可能性があるので、オブ
//      ジェクト変数の宣言よりも前でなければなりません。
//
//      詳細については MFC テクニカル ノート 33 および
//      58 を参照してください。
//

/////////////////////////////////////////////////////////////////////////////
// CSTEP_mp3App

BEGIN_MESSAGE_MAP(CSTEP_mp3App, CWinApp)
    //{{AFX_MSG_MAP(CSTEP_mp3App)
        // メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
        //        この位置に生成されるコードを編集しないでください。
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSTEP_mp3App の構築

CSTEP_mp3App::CSTEP_mp3App()
{
    // TODO: この位置に構築用のコードを追加してください。
    // ここに InitInstance の中の重要な初期化処理をすべて記述してください。
}

/////////////////////////////////////////////////////////////////////////////
// 唯一の CSTEP_mp3App オブジェクト

CSTEP_mp3App theApp;

UINT nPluginID;
UINT nFileTypeMP3;
UINT nFileTypeMP3V1;
UINT nFileTypeMP3V11;
UINT nFileTypeID3V2;
UINT nFileTypeRMP;

enum    {SIF_CONV_ALL_FIELD, SIF_CONV_LENGTH_OK};

CString strINI;
// オプション設定

const bool bOptAutoConvID3v2 = true;extern const bool bOptAutoConvID3v2_default = true;
//bool    bOptAutoConvRMP;            extern const bool bOptAutoConvRMP_default = true;
int     nOptSIFieldConvType;        extern const int  nOptSIFieldConvType_default = SIF_CONV_ALL_FIELD;
bool    bOptID3v2ID3tagAutoWrite;   extern const bool bOptID3v2ID3tagAutoWrite_default = true;
bool    bOptRmpID3tagAutoWrite;     extern const bool bOptRmpID3tagAutoWrite_default = true;
bool    bOptID3v2GenreAddNumber;    extern const bool bOptID3v2GenreAddNumber_default = false;
bool    bOptChangeFileExt;          extern const bool bOptChangeFileExt_default = false;
bool    bOptID3v2GenreListSelect;   extern const bool bOptID3v2GenreListSelect_default = false; //他のプラグインに合わせたつもりだったが不具合報告・要望が多すぎるので初期値を STEP_M の頃に戻す(のをやっぱりやめた)(true だと STEP_M 以前と同じ)
bool    bOptRmpGenreListSelect;     extern const bool bOptRmpGenreListSelect_default = false;   //同上(今さら RMP を使う人はいないと思うが)
bool    bOptID3v2ID3tagAutoDelete;  extern const bool bOptID3v2ID3tagAutoDelete_default = true;
CString strOptSoftwareTag;          extern const TCHAR strOptSoftwareTag_default[] = _T("");

bool    bOptUnSync;                 extern const bool bOptUnSync_default = false;

int     nId3v2Encode;               extern const int  nId3v2Encode_default = 0; //ID3v2 文字エンコード(変更しない)
int     nId3v2Version;              extern const int  nId3v2Version_default = 0;//ID3v2 バージョン(変更しない)
int     nId3v2EncodeNew;            extern const int  nId3v2EncodeNew_default = 1; //新規作成時 ID3v2 文字エンコード(UTF16)
int     nId3v2VersionNew;           extern const int  nId3v2VersionNew_default = 1;//新規作成時 ID3v2 バージョン(v2.3)
int     nId3v2EncodeConv = nId3v2Encode_default;
int     nId3v2VersionConv = nId3v2Version_default;
bool    bOptUnSyncConv = bOptUnSync_default;

const bool    bAutoISO8859_1toUtf16 = true;
//↑added by Kobarin
//エンコードの指定が「変更なし」で更新前が「ISO_8859_1」のとき、UNICODE 固有文字
//使用時は UTF16 に変換する場合に true
//設定出来た方が良いかもしれないが、面倒なので常に有効
//エンコードの指定が明示的に「ISO_8859_1」となっている場合はそちらを優先

// コマンドID
//UINT nIDFileConvAutoID3;//標準MP3/ID3v2 形式に自動変換(廃止)
UINT nIDFileConvMP3;     //ID3v1 に変換
UINT nIDFileConvID3v2;   //ID3v2 に変換&ID3v2バージョン/文字エンコードの変換
UINT nIDFileConvRMP;     //RIFF MP3 形式に変換
UINT nIDDeleteID3;       //ID3v1/v2 を削除
//UINT nIDConvID3v2Version;//ID3v2バージョン/文字エンコードの変換(廃止)(ID3v2 に変換と統合)

bool ID3v1IsEmpty(FILE_INFO *pFileMP3)
{//ID3v1 が空の場合は true を返す
    if(GetTrackName(pFileMP3)[0]){
        return false;
    }
    if(GetArtistName(pFileMP3)[0]){
        return false;
    }
    if(GetAlbumName(pFileMP3)[0]){
        return false;
    }
    if(GetYear(pFileMP3)[0]){
        return false;
    }
    if(GetComment(pFileMP3)[0]){
        return false;
    }
    if(GetBGenre(pFileMP3) != 0xFF){
        return false;
    }
    if(GetBTrackNumber(pFileMP3) != 0xFF){
        return false;
    }
    return true;
}
#ifdef _UNICODE
static bool IsUnicodeStr(const WCHAR *str)
{//UNICODE 固有の(ANSI で表現出来ない)文字を使用している場合は true を返す
    int len_ansi = WideCharToMultiByte(CP_ACP, 0, str, -1, 0, 0, NULL, NULL);
    char *str_ansi = (char*)malloc(len_ansi);
    WideCharToMultiByte(CP_ACP, 0, str, -1, str_ansi, len_ansi, NULL, NULL);
    //UTF16 に戻して元の文字列と一致するか確認
    int len_utf16 = MultiByteToWideChar(CP_ACP, 0, str_ansi, -1, 0, 0);
    WCHAR *str_utf16 = (WCHAR*)malloc(len_utf16*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str_ansi, -1, str_utf16, len_utf16);
    bool bRet = (wcscmp(str, str_utf16) != 0);
    free(str_utf16);
    free(str_ansi);
    return bRet;
}
#endif
STEP_API LPCTSTR WINAPI STEPGetPluginInfo(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return _T("Version 1.02 Copyright (C) 2003-2006 haseta\r\n")
           _T("Version 1.04M Copyright (C) 2008-2010 Mimura\r\n")
           _T("Version 1.07 Copyright (C) 2016-2019 Kobarin\r\n")
           _T("MP3(ID3v1/ID3v2)/RIFF形式をサポートしています");
}

void AddConvMenu(HMENU hMenu) {
    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_SEPARATOR, 0, NULL);
    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDFileConvMP3, _T("ID3v1 に変換(MP3)"));
    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDFileConvRMP, _T("RIFF MP3 に変換(MP3)"));
    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDFileConvID3v2, _T("ID3v2 に変換/バージョン・文字エンコード変換(MP3)"));
//    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDConvID3v2Version, _T("ID3v2バージョン/文字エンコードの変換"));
}

void AddEditMenu(HMENU hMenu) {
    TCHAR szMenu[100];
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    bool bAddDeleteID3 = false;
    for (int iPos=0;iPos<GetMenuItemCount(hMenu);iPos++) {
        mii.dwTypeData = (LPTSTR)szMenu;
        mii.cch = sizeof(szMenu)/sizeof(TCHAR)-1;
        if (GetMenuItemInfo(hMenu, iPos, true, &mii)) {
            if (mii.fType == MFT_STRING) {
                if (_tcscmp(szMenu, _T("編集前の状態に戻す")) == 0) {
                    InsertMenu(hMenu, iPos, MF_BYPOSITION | MFT_STRING, nIDDeleteID3, _T("ID3v1/v2 を削除(MP3)"));
                    bAddDeleteID3 = true;
                    iPos++;
                }
            }
        }
    }

    if (!bAddDeleteID3) {
        InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDDeleteID3, _T("ID3v1/v2 を削除(MP3)"));
    }
}

extern "C" STEP_API bool WINAPI STEPInit(UINT pID, LPCTSTR szPluginFolder)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (Initialize() == false)  return false;
    nPluginID = pID;

    // INIファイルの読み込み
    strINI = szPluginFolder;
    strINI +=_T( "STEP_mp3.ini");
    CIniFile iniFile(strINI);
    //bOptAutoConvID3v2 = true;//iniFile.ReadInt(_T("RMP_ID3V2"), _T("AutoConvID3v2"), bOptAutoConvID3v2_default)!=0;
    //bOptAutoConvRMP = true;//iniFile.ReadInt(_T("RMP_ID3V2"), _T("AutoConvRMP"), bOptAutoConvRMP_default)!=0;
    nOptSIFieldConvType = iniFile.ReadInt(_T("RMP_ID3V2"), _T("SIFieldConvType"), nOptSIFieldConvType_default);
    bOptID3v2ID3tagAutoWrite = iniFile.ReadInt(_T("RMP_ID3V2"), _T("ID3v2ID3tagAutoWrite"), bOptID3v2ID3tagAutoWrite_default) != 0;
    bOptRmpID3tagAutoWrite = iniFile.ReadInt(_T("RMP_ID3V2"), _T("RmpID3tagAutoWrite"), bOptRmpID3tagAutoWrite_default) != 0;
    bOptID3v2GenreListSelect = iniFile.ReadInt(_T("RMP_ID3V2"), _T("ID3v2GenreListSelect"), bOptID3v2GenreListSelect_default) != 0;
    bOptRmpGenreListSelect = iniFile.ReadInt(_T("RMP_ID3V2"), _T("RmpGenreListSelect"), bOptRmpGenreListSelect_default) != 0;
    bOptID3v2ID3tagAutoDelete = iniFile.ReadInt(_T("RMP_ID3V2"), _T("ID3v2ID3tagAutoDelete"), bOptID3v2ID3tagAutoDelete_default) != 0;
    bOptID3v2GenreAddNumber = false;// 2005.08.23 iniFile.ReadInt("RMP_ID3V2", "ID3v2GenreAddNumber", bOptID3v2GenreAddNumber_default) != 0;
    bOptChangeFileExt = iniFile.ReadInt(_T("RMP_ID3V2"), _T("ChangeFileExt"), bOptChangeFileExt_default) != 0;
    iniFile.ReadStr(_T("OTHER"), _T("SoftwareTag"), __T(""), strOptSoftwareTag.GetBufferSetLength(255+1), 255);
    strOptSoftwareTag.ReleaseBuffer();

    bOptUnSync = iniFile.ReadInt(_T("OTHER"), _T("ID3v2UnSync"), bOptUnSync_default) != 0;

    nId3v2Encode = iniFile.ReadInt(_T("OTHER"), _T("ID3v2CharEncode"), nId3v2Encode_default);
    nId3v2Version = iniFile.ReadInt(_T("OTHER"), _T("ID3v2Version"), nId3v2Version_default);
    nId3v2EncodeNew = iniFile.ReadInt(_T("OTHER"), _T("ID3v2CharEncodeNew"), nId3v2EncodeNew_default);
    nId3v2VersionNew = iniFile.ReadInt(_T("OTHER"), _T("ID3v2VersionNew"), nId3v2VersionNew_default);

    //値の範囲チェック(ユーザーが手動でiniを書き換えた場合に念の為)
    switch(nId3v2Encode){
    //変更なし: ISO_8859_1: UTF-16:  UTF-8:
    case 0:         case 1: case 2: case 3: break;
    default: nId3v2Encode = nId3v2Encode_default; break;
    }
    switch (nId3v2Version) {
    //変更なし：  v2.2:   v2.3:   v2.4:
    case 0:     case 1: case 2: case 3: break;
    default: nId3v2Version = nId3v2Version_default; break;
    }
    switch (nId3v2EncodeNew) {
    //ISO_8859_1: UTF-16:  UTF-8:
    case 0:       case 1: case 2:break;
    default: nId3v2EncodeNew = nId3v2EncodeNew_default; break;
    }
    switch (nId3v2VersionNew) {
    //v2.2:   v2.3:   v2.4:
    case 0: case 1: case 2: break;
    default: nId3v2VersionNew = nId3v2VersionNew_default; break;
    }
    //ID3v2.2/2.3 で UTF8 を使わないように
    if (nId3v2Encode == 2 && nId3v2Version != 2) {
        nId3v2Encode = 1;//UTF16 にする
    }
    if (nId3v2EncodeNew == 2 && nId3v2VersionNew != 2) {
        nId3v2EncodeNew = 1;//UTF16 にする
    }
    //
    // サポートしている拡張子を登録
    HBITMAP hMP3Bitmap = LoadBitmap(theApp.m_hInstance, MAKEINTRESOURCE(IDB_BITMAP_MP3));
    HBITMAP hMP3V1Bitmap = LoadBitmap(theApp.m_hInstance, MAKEINTRESOURCE(IDB_BITMAP_MP3));
    HBITMAP hMP3V11Bitmap = LoadBitmap(theApp.m_hInstance, MAKEINTRESOURCE(IDB_BITMAP_MP3));
    HBITMAP hID3v2Bitmap = LoadBitmap(theApp.m_hInstance, MAKEINTRESOURCE(IDB_BITMAP_ID3V2));
    HBITMAP hRMPBitmap = LoadBitmap(theApp.m_hInstance, MAKEINTRESOURCE(IDB_BITMAP_RMP));
    nFileTypeMP3 = STEPRegisterExt(nPluginID, _T("mp3"), hMP3Bitmap);
    nFileTypeMP3V1 = STEPRegisterExt(nPluginID, _T("mp3"), hMP3V1Bitmap);
    nFileTypeMP3V11 = STEPRegisterExt(nPluginID, _T("mp3"), hMP3V11Bitmap);
    nFileTypeID3V2 = STEPRegisterExt(nPluginID, _T("mp3"), hID3v2Bitmap);
    nFileTypeRMP = STEPRegisterExt(nPluginID, _T("rmp"), hRMPBitmap);
    DeleteObject(hMP3Bitmap);
    DeleteObject(hMP3V1Bitmap);
    DeleteObject(hMP3V11Bitmap);
    DeleteObject(hID3v2Bitmap);
    DeleteObject(hRMPBitmap);

    // ツールバーへの登録
    COLORMAP map; // 背景色の変換
    map.from = RGB(192,192,192);
    map.to = GetSysColor(COLOR_3DFACE);

    nIDFileConvMP3 = STEPGetCommandID();
    STEPAddToolBarButton(CreateMappedBitmap(theApp.m_hInstance, IDB_FILE_CONV_MP3, 0, &map, 1), nIDFileConvMP3, _T("STEP_mp3_FILE_CONV_MP3"));
    STEPKeyAssign(nIDFileConvMP3, _T("ID3v1 に変換"), _T("STEP_mp3_KEY_FILE_CONV_MP3"));

    nIDFileConvRMP = STEPGetCommandID();
    STEPAddToolBarButton(CreateMappedBitmap(theApp.m_hInstance, IDB_FILE_CONV_RMP, 0, &map, 1), nIDFileConvRMP, _T("STEP_mp3_FILE_CONV_RMP"));
    STEPKeyAssign(nIDFileConvRMP, _T("RIFF MP3 に変換"), _T("STEP_mp3_KEY_FILE_CONV_RMP"));

    nIDFileConvID3v2 = STEPGetCommandID();
    STEPAddToolBarButton(CreateMappedBitmap(theApp.m_hInstance, IDB_FILE_CONV_ID3V2, 0, &map, 1), nIDFileConvID3v2, _T("STEP_mp3_FILE_CONV_ID3V2"));
    STEPKeyAssign(nIDFileConvID3v2, _T("ID3v2 に変換"), _T("STEP_mp3_KEY_FILE_CONV_ID3V2"));

    nIDDeleteID3 = STEPGetCommandID();
    STEPKeyAssign(nIDDeleteID3, _T("ID3v1/v2 を削除"), _T("STEP_mp3_KEY_DELETE_ID3"));

    //nIDConvID3v2Version = STEPGetCommandID();
    //STEPKeyAssign(nIDConvID3v2Version, _T("ID3v2バージョン/文字エンコードの変換"), _T("STEP_mp3_KEY_CONV_ID3V2_VERSION"));

    return true;
}

extern "C" STEP_API void WINAPI STEPFinalize() {
    Finalize();
}

extern "C" STEP_API UINT WINAPI STEPGetAPIVersion(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return STEP_API_VERSION;
}

extern "C" STEP_API LPCTSTR WINAPI STEPGetPluginName(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return _T("STEP_mp3");
}

extern "C" STEP_API bool WINAPI STEPSupportSIF(UINT nFormat) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nFormat == nFileTypeMP3 || nFormat == nFileTypeMP3V1 || nFormat == nFileTypeMP3V11) {
        if(bOptAutoConvID3v2){
            return true;
        }
        return false;
    }
    if (nFormat == nFileTypeID3V2) {
        return true;
    }
    if (nFormat == nFileTypeRMP) {
        return true;
    }
    return true;
}

extern "C" STEP_API bool WINAPI STEPSupportTrackNumberSIF(UINT nFormat) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nFormat == nFileTypeID3V2) {
        return true;
    }
    if ((nFormat == nFileTypeMP3  || nFormat == nFileTypeMP3V1 || nFormat == nFileTypeMP3V11) && bOptAutoConvID3v2) {
        return true;
    }
    return false;
}

extern "C" STEP_API bool WINAPI STEPSupportGenreSIF(UINT nFormat) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nFormat == nFileTypeID3V2) {
        return true;
    }
    if (nFormat == nFileTypeRMP) {
        return true;
    }
    if ((nFormat == nFileTypeMP3  || nFormat == nFileTypeMP3V1 || nFormat == nFileTypeMP3V11) && (bOptAutoConvID3v2 /*|| bOptAutoConvRMP*/)) {
        return true;
    }
    return false;
}

extern "C" STEP_API CONTROLTYPE WINAPI STEPGetControlType(UINT nFormat, COLUMNTYPE nColumn, bool isEditSIF)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    switch (nColumn) {
    case COLUMN_TRACK_NAME:
    case COLUMN_ARTIST_NAME:
    case COLUMN_ALBUM_NAME:
    case COLUMN_TRACK_NUMBER:
    case COLUMN_YEAR:
        if (!isEditSIF && TYPE_IS_MP3(nFormat) && bOptID3v2ID3tagAutoWrite) {
            return _NULL;
        }
        if (!isEditSIF && TYPE_IS_RMP(nFormat) && bOptRmpID3tagAutoWrite) {
            return _NULL;
        }
        if (!isEditSIF && TYPE_IS_MP3(nFormat) && bOptID3v2ID3tagAutoDelete) {
            return _NULL;
        }
        return _EDIT;
    case COLUMN_GENRE:
        if (!isEditSIF && TYPE_IS_MP3(nFormat) && bOptID3v2ID3tagAutoWrite) {
            return _NULL;
        }
        if (!isEditSIF && TYPE_IS_RMP(nFormat) && bOptRmpID3tagAutoWrite) {
            return _NULL;
        }
        if (!isEditSIF && TYPE_IS_MP3(nFormat) && bOptID3v2ID3tagAutoDelete) {
            return _NULL;
        }
        if (TYPE_IS_MP3V2(nFormat) && isEditSIF) {
            if (bOptID3v2GenreListSelect) {
                return _CBOX;
            } else {
                return _EDIT;
            }
        }
        if (TYPE_IS_RMP(nFormat) && isEditSIF) {
            if (bOptRmpGenreListSelect) {
                return _CBOX;
            } else {
                return _EDIT;
            }
        }
        return _CBOX;
    case COLUMN_SOFTWARE:
    case COLUMN_COPYRIGHT:
        if (isEditSIF) {
            return _EDIT;
        } else {
            return _NULL;
        }
    case COLUMN_COMMENT:
        if (!isEditSIF && TYPE_IS_MP3(nFormat) && bOptID3v2ID3tagAutoWrite) {
            return _NULL;
        }
        if (!isEditSIF && TYPE_IS_MP3(nFormat) && bOptID3v2ID3tagAutoDelete) {
            return _NULL;
        }
        if (!isEditSIF && TYPE_IS_RMP(nFormat) && bOptRmpID3tagAutoWrite) {
            return _NULL;
        }
        if ((isEditSIF && TYPE_IS_MP3V1(nFormat) && bOptAutoConvID3v2) || (isEditSIF && TYPE_IS_MP3V2(nFormat))) {
            return _MEDIT;
        } else {
            return _EDIT;
        }
    case COLUMN_TRACK_TOTAL: //ID3v2 専用
    case COLUMN_DISC_NUMBER:
    case COLUMN_DISC_TOTAL:
    case COLUMN_WRITER:
    case COLUMN_COMPOSER:
    case COLUMN_ALBM_ARTIST:
    case COLUMN_ORIG_ARTIST:
    case COLUMN_URL:
    case COLUMN_ENCODEST:
        if (!isEditSIF) {
            return _NULL;
        }
        if(TYPE_IS_RMP(nFormat)){
            return _NULL;
        }
        return _EDIT;
    case COLUMN_SOURCE:     //RIFF MP3 専用
    case COLUMN_KEYWORD:
    case COLUMN_TECHNICIAN:
    case COLUMN_LYRIC:
    case COLUMN_COMMISSION:
        if (!isEditSIF) {
            return _NULL;
        }
        if (!TYPE_IS_RMP(nFormat)){
            return _EDIT;
        }
        return _NULL;
    case COLUMN_ENGINEER:
        if (!isEditSIF) {
            return _NULL;
        }
        return _EDIT;
    }
    return _NULL;
}

extern "C" STEP_API UINT WINAPI STEPGetColumnMax(UINT nFormat, COLUMNTYPE nColumn, bool isEditSIF) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!isEditSIF) {
        switch (nColumn) {
        case COLUMN_TRACK_NAME:     return ID3_LEN_TRACK_NAME;
        case COLUMN_ARTIST_NAME:    return ID3_LEN_ARTIST_NAME;
        case COLUMN_ALBUM_NAME:     return ID3_LEN_ALBUM_NAME;
        case COLUMN_TRACK_NUMBER:   return 3;
        case COLUMN_YEAR:           return ID3_LEN_YEAR;
        case COLUMN_GENRE:          return 128; // ID3v1の場合、ジャンル名称と入力できる最大長は違うので
        case COLUMN_COMMENT:        return ID3_LEN_COMMENT;
        default:                    return 0;
        }
    }
    if (((nFormat == nFileTypeMP3 || nFormat == nFileTypeMP3V1 || nFormat == nFileTypeMP3V11) && bOptAutoConvID3v2) || nFormat == nFileTypeID3V2) {
        switch (nColumn) {
        case COLUMN_TRACK_NAME:     // トラック名
        case COLUMN_ARTIST_NAME:    // アーティスト名
        case COLUMN_ALBUM_NAME:     // アルバム名
        case COLUMN_YEAR:           // リリース年号
        case COLUMN_GENRE:          // ジャンル番号
        case COLUMN_COPYRIGHT:      // 著作権
        case COLUMN_SOFTWARE:       // ソフトウェア
        case COLUMN_WRITER:         // 作詞
        case COLUMN_COMPOSER:       // 作曲
        case COLUMN_ALBM_ARTIST:    // Albm.アーティスト
        case COLUMN_ORIG_ARTIST:    // Orig.アーティスト
        case COLUMN_URL:            // URL
        case COLUMN_ENCODEST:       // エンコードした人
        case COLUMN_ENGINEER:       // エンジニア（出版）
        case COLUMN_COMMENT:        // コメント
            return 2048; /* 2003.06.20 増やした */
        case COLUMN_TRACK_NUMBER:   // トラック番号
        case COLUMN_TRACK_TOTAL:    // トラック数
        case COLUMN_DISC_NUMBER:    // ディスク番号
        case COLUMN_DISC_TOTAL:     // ディスク数
            return 32;
        }
    } else {
        switch (nColumn) {
        case COLUMN_TRACK_NUMBER:   // トラック番号
        case COLUMN_TRACK_TOTAL:    // トラック数
        case COLUMN_DISC_NUMBER:    // ディスク番号
        case COLUMN_DISC_TOTAL:     // ディスク数
            return 32;
        case COLUMN_TRACK_NAME:     // トラック名
        case COLUMN_ARTIST_NAME:    // アーティスト名
        case COLUMN_ALBUM_NAME:     // アルバム名
        case COLUMN_YEAR:           // リリース年号
        case COLUMN_GENRE:          // ジャンル番号
        case COLUMN_COMMENT:        // コメント
        case COLUMN_COPYRIGHT:      // 著作権
        case COLUMN_ENGINEER:       // エンジニア(製作者)
        case COLUMN_SOURCE:         // ソース
        case COLUMN_SOFTWARE:       // ソフトウェア
        case COLUMN_KEYWORD:        // キーワード
        case COLUMN_TECHNICIAN:     // 技術者
        case COLUMN_LYRIC:          // 歌詞
            return 2048;
        }
    }
    return 0;
}

// ID3 Tag v1.0/v1.1 かどうかを調べる
static  bool    IsID3Tag(const ID3TAG *data) {
    return((((const ID3TAG_V10 *)(data))->sTAG[0] == 'T' &&
            ((const ID3TAG_V10 *)(data))->sTAG[1] == 'A' &&
            ((const ID3TAG_V10 *)(data))->sTAG[2] == 'G') ? true : false);
}
// ID3 Tag v1.0 かどうかを調べる
static  bool    IsID3Tag10(const ID3TAG *data) {
    return(((IsID3Tag(data) && (((const ID3TAG_V11 *)data)->cZero != '\0') || ((const ID3TAG_V11 *)data)->byTrackNumber == '\0')) ? true : false);
}
// ID3 Tag v1.1 かどうかを調べる
static  bool    IsID3Tag11(const ID3TAG *data) {
    return(((IsID3Tag(data) && (((const ID3TAG_V11 *)data)->cZero == '\0') && ((const ID3TAG_V11 *)data)->byTrackNumber != '\0'))? true : false);
}

void StringCopyN(TCHAR *sDest, const TCHAR *sSrc, int nLen);//sDest のサイズは (nLen+1) 以上あることが前提

void StringCopyN(TCHAR *sDest, const TCHAR *sSrc, int nLen)
{//長さ nLen だけ sDest にコピー(ID3v2=>ID3v1, RIFF=>ID3v1用)
 //sDest のサイズは nLen+1 以上あることが前提
 //nLen は ANSI 換算の長さであるため、UNICODE 版では nLen 文字コピーではないことに注意
 //例えば sSrc が半角文字だけで構成されている場合は nLen 文字のコピーとなるが、
 //sSrc が全角文字だけで構成されている場合は nLen/2 文字のコピーとなる
 //UNICODE 版では sSrc に ANSI で表現出来ない文字を含むと ? や代替文字に置換されることに注意
#ifndef _UNICODE
    _mbsncpy_s((unsigned char*)sDest, (nLen+1), (unsigned char*)sSrc, _TRUNCATE);
    //↑_mbsncpy_s でマルチバイトの２バイト目が切れることはない筈だが念のためチェック
    int i = 0;
    while(sDest[i]){
        if(IsDBCSLeadByte((BYTE)sDest[i])){
            if(!sDest[i+1]){//文字列が切れている
                sDest[i] = 0;
                break;
            }
            i += 2;
        }
        else{
            i++;
        }
    }
#else
    CHAR* str_ansi = (CHAR*)malloc(static_cast<size_t>(nLen) + 1);
    //nLen によってマルチバイトの２バイト目で切れる場合、
    //切れる文字(マルチバイト1文字目)に 0 が入らない
    ZeroMemory(str_ansi, nLen+1);//↑なのでメモリをゼロクリアしておく(どこで切れるか分からないので)
    int len_ansi = WideCharToMultiByte(CP_ACP, 0, sSrc, -1, 0, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, sSrc, -1, str_ansi, nLen, NULL, NULL);
    str_ansi[nLen] = 0;
    //マルチバイトの２バイト目で切れているかどうか確認
    //(OS によって挙動が異なるかもしれないので)
    int i = 0;
    while(str_ansi[i]){
        if(IsDBCSLeadByte((BYTE)str_ansi[i])){
            if(!str_ansi[i+1]){//文字列が切れている
                str_ansi[i] = 0;
                break;
            }
            i += 2;
        }
        else{
            i++;
        }
    }
    MultiByteToWideChar(CP_ACP, 0, str_ansi, -1, sDest, nLen);
    free(str_ansi);
    sDest[nLen] = 0;
#endif
}

void setFileType(CId3tagv2& id3v2, FILE_INFO *pFileMP3)
{
    CString strFileTypeName;
    switch(id3v2.GetVer()){
    case 0x0200:
        strFileTypeName = _T("MP3(ID3v2.2");
        break;
    case 0x0300:
        strFileTypeName = _T("MP3(ID3v2.3");
        break;
    case 0x0400:
        strFileTypeName = _T("MP3(ID3v2.4");
        break;
    default:
        strFileTypeName = _T("MP3(ID3v2.?");
        break;
    }
    if (GetFormat(pFileMP3) == nFileTypeMP3V1) {
        strFileTypeName = strFileTypeName + _T("+v1.0");
    }
    if (GetFormat(pFileMP3) == nFileTypeMP3V11) {
        strFileTypeName = strFileTypeName + _T("+v1.1");
    }
    strFileTypeName = strFileTypeName + _T(")");
    if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_ISO_8859_1) {
    } else if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_16) {
        strFileTypeName = strFileTypeName + _T(",UTF16");
    } else if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_16BE) {
        strFileTypeName = strFileTypeName + _T(",UTF16BE");
    } else if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_8) {
        strFileTypeName = strFileTypeName + _T(",UTF8");
    }
    if (id3v2.GetUnSynchronization()) {
        strFileTypeName = strFileTypeName + _T(",US");
    }
    SetFileTypeName(pFileMP3, strFileTypeName);
    SetFormat(pFileMP3, nFileTypeID3V2);        // ファイル形式：MP3(ID3v2)
}

bool ReadTagID3(LPCTSTR sFileName, FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CId3tagv1 id3v1;
    id3v1.Load(sFileName);
    if(id3v1.IsEnable()){
        SetTrackName(pFileMP3, id3v1.GetTitle());  //トラック名
        SetArtistName(pFileMP3, id3v1.GetArtist());//アーティスト名
        SetAlbumName(pFileMP3, id3v1.GetAlbum());  //アルバム名
        SetYear(pFileMP3, id3v1.GetYear());        //リリース年号
        SetComment(pFileMP3, id3v1.GetComment());  //コメント
        CString strTrackNo = id3v1.GetTrackNo();
        SetTrackNumber(pFileMP3, strTrackNo);
        if(!strTrackNo.IsEmpty()){
            SetFormat(pFileMP3, nFileTypeMP3V11);   // ファイル形式：MP3V1.1
            SetFileTypeName(pFileMP3, _T("MP3(ID3v1.1)"));
        } else {
            SetFormat(pFileMP3, nFileTypeMP3V1);    // ファイル形式：MP3V1.0
            SetFileTypeName(pFileMP3, _T("MP3(ID3v1.0)"));
        }
        SetGenre(pFileMP3, id3v1.GetGenre());
        // ジャンル番号
        //SetBGenre(pFileMP3, pTag->byGenre);
        //SetGenre(pFileMP3, STEPGetGenreNameSIF(pTag->byGenre));
    }
    else {
    //この関数が呼ばれた時点でファイルオープンは成功している筈なので
    //id3tag の有無に関わらず「ファイル形式：MP3」とする
        SetFileTypeName(pFileMP3, _T("MP3"));
        SetFormat(pFileMP3, nFileTypeMP3);  // ファイル形式：MP3
    }
    return true;
}

bool ReadTagID3v2(LPCTSTR sFileName, FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // ID3v2 タグの取得
    CId3tagv2   id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    if (id3v2.Load(sFileName) != ERROR_SUCCESS// 読み込み失敗
    ||  id3v2.IsEnable() == FALSE           // ID3v2 ではない
    ||  !TRUE/*id3v2.IsSafeVer()*/) {               // 未対応のバージョン
        // 読み込み失敗
        if (id3v2.GetVer() > 0x0000 && !TRUE/*id3v2.IsSafeVer()*/) {
            return(true);
        }
        return(false);
    }

    // *** タグ情報の設定 ***
    SetTrackNameSI(pFileMP3, id3v2.GetTitle());     // タイトル
    SetArtistNameSI(pFileMP3, id3v2.GetArtist());   // アーティスト名
    SetAlbumNameSI(pFileMP3, id3v2.GetAlbum());     // アルバム名
    SetYearSI(pFileMP3, id3v2.GetYear());           // リリース
    SetCommentSI(pFileMP3, id3v2.GetComment());     // コメント
    SetGenreSI(pFileMP3, id3v2.GetGenre());         // ジャンル名
    {//トラック番号
     // x/y 形式になっていれば y の方をトラック数として処理
        CString strTrackNo = id3v2.GetTrackNo();
        TCHAR *trk_number = strTrackNo.GetBuffer();
        TCHAR *trk_total = _tcschr(trk_number, _T('/'));
        if(trk_total){
            *trk_total++ = 0;
        }
        SetTrackNumberSI(pFileMP3, trk_number);
        if(trk_total){
            SetTrackTotalSI(pFileMP3, trk_total);
        }
        strTrackNo.ReleaseBuffer();
    }
    {//ディスク番号
     //x/y 形式になっていれば y の方をディスク数として処理
        CString strDiscNo = id3v2.GetDiscNo();
        TCHAR *disc_number = strDiscNo.GetBuffer();
        TCHAR *disc_total = _tcschr(disc_number, _T('/'));
        if(disc_total){
            *disc_total++ = 0;
        }
        SetDiscNumberSI(pFileMP3, disc_number);
        if(disc_total){
            SetDiscTotalSI(pFileMP3, disc_total);
        }
        strDiscNo.ReleaseBuffer();
    }
    SetSoftwareSI(pFileMP3, id3v2.GetEncoder());        // エンコーダ
    SetCopyrightSI(pFileMP3, id3v2.GetCopyright());     // 著作権
    SetWriterSI(pFileMP3,id3v2.GetWriter());            // 作詞
    SetComposerSI(pFileMP3, id3v2.GetComposer());       // 作曲
    SetAlbumArtistSI(pFileMP3, id3v2.GetAlbumArtist()); // Albm. アーティスト
    SetOrigArtistSI(pFileMP3, id3v2.GetOrigArtist());   // Orig.アーティスト
    SetURLSI(pFileMP3, id3v2.GetUrl());                 // URL
    SetEncodest(pFileMP3, id3v2.GetEncodedBy());        // エンコードした人
    SetEngineerSI(pFileMP3,id3v2.GetEngineer());        // エンジニア（出版）

//  SetFileTypeName(pFileMP3, "MP3(ID3v2)");
    setFileType(id3v2, pFileMP3);
    return(true);
}

bool ReadTagSIF(LPCTSTR sFileName, FILE_INFO *pFileMP3)
{
    CRMP rmp/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    if (rmp.Load(GetFullPath(pFileMP3))  != ERROR_SUCCESS   // 読み込み失敗
        ||  rmp.IsEnable() == FALSE) {
        // 読み込み失敗
        return(false);
    }

    // 読み込み成功
    // SI フィールド
    SetTrackNameSI(pFileMP3,    rmp.GetNAM());  // 曲名
    SetArtistNameSI(pFileMP3,   rmp.GetART());  // アーティスト名
    SetAlbumNameSI(pFileMP3,    rmp.GetPRD());  // 製品名
    SetCommentSI(pFileMP3,      rmp.GetCMT());  // コメント文字列
    SetYearSI(pFileMP3,         rmp.GetCRD());  // リリース年号
    SetGenreSI(pFileMP3,        rmp.GetGNR());  // ジャンル名
    SetCopyrightSI(pFileMP3,    rmp.GetCOP());  // 著作権
    SetEngineerSI(pFileMP3,     rmp.GetENG());  // エンジニア
    SetSourceSI(pFileMP3,       rmp.GetSRC());  // ソース
    SetSoftwareSI(pFileMP3,     rmp.GetSFT());  // ソフトウェア
    SetKeywordSI(pFileMP3,      rmp.GetKEY());  // キーワード
    SetTechnicianSI(pFileMP3,   rmp.GetTCH());  // 技術者
    SetLyricSI(pFileMP3,        rmp.GetLYC());  // 歌詞
    SetCommissionSI(pFileMP3,   rmp.GetCMS());  // コミッション
    CString strFileTypeName = _T("RIFF MP3");
    if (GetFormat(pFileMP3) == nFileTypeMP3V1) {
        strFileTypeName = strFileTypeName + _T("+ID3v1.0");
    }
    if (GetFormat(pFileMP3) == nFileTypeMP3V11) {
        strFileTypeName = strFileTypeName + _T("+ID3v1.1");
    }
    SetFormat(pFileMP3, nFileTypeRMP);      // ファイル形式：RIFF MP3
    SetFileTypeName(pFileMP3, strFileTypeName);

    //if (bOptRmpID3tagAutoWrite)
    //    SetGenreSI(pFileMP3, GetGenre(pFileMP3));

    return(true);
}

bool LoadFileMP3(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // === ID3v1.0/1.1 形式ファイル ===
    BYTE Head[4] = {0};
    CString strFileName = GetFullPath(pFileMP3);
    FILE *fp;
    _tfopen_s(&fp, strFileName, _T("rb"));
    if(!fp){
        CString strMsg;
        strMsg.Format(_T("%s がオープンできませんでした"), strFileName);
        MessageBox(NULL, strMsg, _T("ファイルのオープン失敗"), MB_ICONSTOP|MB_OK|MB_TOPMOST);
        return false;
    }
    fread(Head, 1, 4, fp);
    fclose(fp);
    ReadTagID3(strFileName, pFileMP3);

    if (memcmp(Head, "ID3", 3) == 0) {
        // === ID3v2 形式ファイル ===
        if (bOptAutoConvID3v2/*bOptID3v2ID3tagAutoWrite*/) {
            SetGenreSI(pFileMP3, GetGenre(pFileMP3));
            SetTrackNumberSI(pFileMP3, GetTrackNumber(pFileMP3));
        }
        if (ReadTagID3v2(strFileName, pFileMP3) == false) {
            MessageBox(NULL, _T("ID3v2 タグの読み込みに失敗しました"), GetFullPath(pFileMP3), MB_ICONSTOP|MB_OK|MB_TOPMOST);
            return false;
        }
    }
    else if (memcmp(Head, "RIFF", 4) == 0) {
        // === RIFF MP3 形式ファイル(SIフィールドの読み込み) ===
        if (ReadTagSIF(strFileName, pFileMP3) == false) {
            MessageBox(NULL, _T("RIFF MP3 タグの読み込みに失敗しました"), GetFullPath(pFileMP3), MB_ICONSTOP|MB_OK|MB_TOPMOST);
            return(false);
        }
    }
    else{// === ID3v1.0/1.1 のみ ===
        if (bOptAutoConvID3v2 /*|| bOptAutoConvRMP*/) {
            if(GetBGenre(pFileMP3) != 0xFF){
                SetGenreSI(pFileMP3, GetGenre(pFileMP3));
            }
            else{
                SetGenreSI(pFileMP3, _T(""));
            }
            SetTrackNumberSI(pFileMP3, GetTrackNumber(pFileMP3));
        }
        SetTrackNameSI(pFileMP3, GetTrackName(pFileMP3));
        SetArtistNameSI(pFileMP3, GetArtistName(pFileMP3));
        SetAlbumNameSI(pFileMP3, GetAlbumName(pFileMP3));
        SetCommentSI(pFileMP3, GetComment(pFileMP3));
        SetYearSI(pFileMP3, GetYear(pFileMP3));
    }

    // mp3infpにより取得
    //GetValues_mp3infp(pFileMP3);
    CMp3Info mp3info;
    if(mp3info.Load(GetFullPath(pFileMP3))){
        SetAudioFormat(pFileMP3, mp3info.GetFormatString());
        CString strTimeString = mp3info.GetTimeString();
        TCHAR *time = strTimeString.GetBuffer();
        //szTime = "xx:xx (xxxsec) のようになっている
        TCHAR *pszSec = _tcschr(time, _T('('));
        if(pszSec){
            pszSec++;
            TCHAR *end;
            int sec = _tcstol(pszSec, &end, 10);
            SetPlayTime(pFileMP3, sec);
        }
        strTimeString.ReleaseBuffer();
    }

    return true;
}

STEP_API UINT WINAPI STEPLoad(FILE_INFO *pFileMP3, LPCTSTR szExt)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (_tcsicmp(szExt, _T("mp3")) == 0 || _tcsicmp(szExt, _T("rmp")) == 0) {
        if (LoadFileMP3(pFileMP3) == false) {
            return STEP_ERROR;
        }
        if (GetFormat(pFileMP3) != FILE_FORMAT_UNKNOWN) {
            return STEP_SUCCESS;
        }
    }
    return STEP_UNKNOWN_FORMAT;
}

bool WriteTagID3(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CId3tagv1 id3v1;

    id3v1.SetTitle(GetTrackName(pFileMP3));  //トラック名
    id3v1.SetArtist(GetArtistName(pFileMP3));//アーティスト名
    id3v1.SetAlbum(GetAlbumName(pFileMP3));  //アルバム名
    id3v1.SetYear(GetYear(pFileMP3));        //リリース年号
    id3v1.SetComment(GetComment(pFileMP3));  //コメント
    BYTE byTrackNumber = GetBTrackNumber(pFileMP3);
    if(byTrackNumber != 0xFF){
        id3v1.SetTrackNo(byTrackNumber);
        // 再設定
        CString strTrackNumber;
        strTrackNumber.Format(_T("%d"), byTrackNumber);
        SetTrackNumber(pFileMP3, strTrackNumber);
        SetFormat(pFileMP3, nFileTypeMP3V11);   // ファイル形式：MP3V1.1
        SetFileTypeName(pFileMP3, _T("MP3(ID3v1.1)"));
    }
    else{
        // 再設定
        SetFormat(pFileMP3, nFileTypeMP3V1);    // ファイル形式：MP3V1.0
        SetFileTypeName(pFileMP3, _T("MP3(ID3v1.0)"));
    }
    // ジャンル
    BYTE byGenre = GetBGenre(pFileMP3);
    id3v1.SetGenre(byGenre);
    SetGenre(pFileMP3, STEPGetGenreNameSIF(byGenre));
    if(id3v1.Save(GetFullPath(pFileMP3)) != 0){
        CString strMsg;
        strMsg.Format(_T("%s がオープンできませんでした"), GetFullPath(pFileMP3));
        MessageBox(NULL, strMsg, _T("ファイルのオープン失敗"), MB_ICONSTOP|MB_OK|MB_TOPMOST);
        return false;
    }
    return true;
}

bool ConvID3tagToSIField(FILE_INFO *pFileMP3)
{
    SetModifyFlag(pFileMP3, true);                      // 変更されたフラグをセット
    SetTrackNameSI(pFileMP3, GetTrackName(pFileMP3));   // 曲名
    SetArtistNameSI(pFileMP3, GetArtistName(pFileMP3)); // アーティスト名
    SetAlbumNameSI(pFileMP3, GetAlbumName(pFileMP3));   // 製品名
    SetCommentSI(pFileMP3, GetComment(pFileMP3));       // コメント文字列
    SetYearSI(pFileMP3, GetYear(pFileMP3));             // リリース年号

#define LIMIT_TEXT_LENGTH(strID3, nLen) {           \
    TCHAR   sWorkBuffer[nLen+1];                    \
    StringCopyN(sWorkBuffer, GetValue(pFileMP3, strID3), nLen);         \
    sWorkBuffer[nLen] = '\0';                       \
    SetValue(pFileMP3, strID3, sWorkBuffer);            \
}
    // ID3 tag の文字数を調整(自動変換の為の対処)
    LIMIT_TEXT_LENGTH(FIELD_TRACK_NAME      , ID3_LEN_TRACK_NAME);
    LIMIT_TEXT_LENGTH(FIELD_ARTIST_NAME     , ID3_LEN_ARTIST_NAME);
    LIMIT_TEXT_LENGTH(FIELD_ALBUM_NAME      , ID3_LEN_ALBUM_NAME);
    LIMIT_TEXT_LENGTH(FIELD_YEAR            , ID3_LEN_YEAR);
    if (GetBTrackNumber(pFileMP3) != (BYTE)0xff) {
        LIMIT_TEXT_LENGTH(FIELD_COMMENT, ID3_LEN_COMMENT-2);
    } else {
        LIMIT_TEXT_LENGTH(FIELD_COMMENT, ID3_LEN_COMMENT);
    }
#undef LIMIT_TEXT_LENGTH
    // ソフトウェアの設定
    UINT nFormat = GetFormat(pFileMP3);
    if (!(nFormat == nFileTypeMP3 || nFormat == nFileTypeMP3V1 || nFormat == nFileTypeMP3V11)) {
        if (_tcslen(GetSoftwareSI(pFileMP3)) == 0) {
            SetSoftwareSI(pFileMP3, strOptSoftwareTag);
        }
    }

    if (_tcslen(GetGenreSI(pFileMP3)) == 0 || false /* 常に再設定→未設定時のみ */) {
        //SetGenreSI(pFileMP3, STEPGetGenreNameSIF(GetBGenre(pFileMP3)));
        SetGenreSI(pFileMP3, GetGenre(pFileMP3));
    }
    if (_tcslen(GetTrackNumberSI(pFileMP3)) == 0) { /* 未設定時のみ */
        SetTrackNumberSI(pFileMP3, GetTrackNumber(pFileMP3));
    }
    return true;
}

static void COPY_FIELD(FILE_INFO *pFileMP3, FIELDTYPE fieldType, const CString& strSIF, int len)
{// コピー対象のフィールドかどうかをチェックして、必要な場合だけコピーします
 //UNIOCDE 版では len が文字数で単純な長さ比較ではうまくいかないので、
 //StringCopyN で ANSI 換算 での len 文字コピーした場合の文字列で必要か否かを判断する
    TCHAR* sBuffer = (TCHAR*)malloc((static_cast<unsigned long long>(len) + 1) * sizeof(TCHAR));
    StringCopyN(sBuffer, strSIF, len);//長さ len までコピー
    if (nOptSIFieldConvType == SIF_CONV_ALL_FIELD || //常にコピー
        _tcscmp(sBuffer, strSIF) == 0) {             //長さ len までコピーしたものと同じならコピー
        SetValue(pFileMP3, fieldType, sBuffer);
    }
    free(sBuffer);
}

void ConvSIFieldToID3tag(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    //TCHAR   sBuffer[256+1];

    //if (GetFormat(pFileMP3) != nFileTypeID3V2 && GetFormat(pFileMP3) != nFileTypeRMP) {
    //    return;
    //}

    SetModifyFlag(pFileMP3, TRUE);              // 変更されたフラグをセット

/*
// コピー対象のフィールドかどうかをチェックして、必要な場合だけコピーします
#define COPY_FIELD(strID3, strSIF, len) {           \
    if (nOptSIFieldConvType == SIF_CONV_ALL_FIELD   \
    || _tcslen(strSIF) <= (len)) {                  \
        StringCopyN(sBuffer, strSIF, len);          \
        sBuffer[len] = '\0';                        \
        SetValue(pFileMP3, strID3, sBuffer);        \
    }                                               \
}
*/
    //↑関数化
    // 曲名
    COPY_FIELD(pFileMP3, FIELD_TRACK_NAME, GetTrackNameSI(pFileMP3), ID3_LEN_TRACK_NAME);
    // アーティスト名
    COPY_FIELD(pFileMP3, FIELD_ARTIST_NAME, GetArtistNameSI(pFileMP3), ID3_LEN_ARTIST_NAME);
    // アルバム名
    COPY_FIELD(pFileMP3, FIELD_ALBUM_NAME, GetAlbumNameSI(pFileMP3), ID3_LEN_ALBUM_NAME);
    // トラック番号 //コメント設定より先に処理する
    if (_tcslen(GetTrackNumberSI(pFileMP3)) == 0) {
        if (GetFormat(pFileMP3) == nFileTypeID3V2) {
            SetBTrackNumber(pFileMP3, (BYTE)0xff);
        }
    } else {
        SetBTrackNumber(pFileMP3, (BYTE)STEPGetIntegerTrackNumber(GetTrackNumberSI(pFileMP3)));
    }
    // コメント文字列
    if (GetBTrackNumber(pFileMP3) == (BYTE)0xff) {
        // ID3 v1.0
        COPY_FIELD(pFileMP3, FIELD_COMMENT, GetCommentSI(pFileMP3), ID3_LEN_COMMENT);
    } else {
        // ID3 v1.1
        COPY_FIELD(pFileMP3, FIELD_COMMENT, GetCommentSI(pFileMP3), ID3_LEN_COMMENT-2);
    }
    // リリース年号
    COPY_FIELD(pFileMP3, FIELD_YEAR, GetYearSI(pFileMP3), ID3_LEN_YEAR);
    // ジャンル
    SetBGenre(pFileMP3, STEPGetGenreCode(GetGenreSI(pFileMP3)));
    SetGenre(pFileMP3, STEPGetGenreNameSIF(GetBGenre(pFileMP3)));
//#undef COPY_FIELD
//#undef COPY_FIELD2

    return;
}


bool WriteTagID3v2(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // ID3v2 タグの取得
    CId3tagv2   id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    if (id3v2.Load(GetFullPath(pFileMP3))  != ERROR_SUCCESS // 読み込み失敗
    ||  id3v2.IsEnable() == FALSE           // ID3v2 ではない
    ||  !TRUE/*id3v2.IsSafeVer()*/) {               // 未対応のバージョン
        // 読み込み失敗
        return(false);
    }

    switch (nId3v2Version) {
    case 1: // v2.2
        id3v2.SetVer(0x0200);
        break;
    case 2: // v2.3
        id3v2.SetVer(0x0300);
        break;
    case 3: // v2.4
        id3v2.SetVer(0x0400);
        break;
    default:
        break;
    }
    switch (nId3v2Encode) {
    case 1:
        id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_ISO_8859_1);
        break;
    case 2:
        id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16);
        break;
    case 3:
        id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_8);
        break;
    //case 4:
    //  id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16BE);
    //  break;
    default:
        break;
    }
    // ID3tag の自動更新
//  if (bOptID3v2ID3tagAutoWrite) {
//      ConvSIFieldToID3tag(pFileMP3);
//  }
    // ソフトウェアの設定
    if (_tcslen(GetSoftwareSI(pFileMP3)) == 0) {
        SetSoftwareSI(pFileMP3, strOptSoftwareTag);
    }

#ifdef _UNICODE
    if(nId3v2Encode == 0 && //「ID3v2 の保存形式」-「文字エンコード」-「変更しない」
        id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_ISO_8859_1 && //更新前が ISO_8859_1
        bAutoISO8859_1toUtf16){
    //by Kobarin
    //変更しない設定でも、更新前の文字エンコードが ISO_8859_1 の場合は更新後の方で UNICODE 固有
    //の文字を使用しているかどうかをチェックし、使用している場合は UTF16 で書き込む
        if(IsUnicodeStr(GetTrackNameSI(pFileMP3)) ||
           IsUnicodeStr(GetArtistNameSI(pFileMP3)) ||
           IsUnicodeStr(GetAlbumNameSI(pFileMP3)) ||
           IsUnicodeStr(GetSoftwareSI(pFileMP3)) ||
           IsUnicodeStr(GetTrackNumberSI(pFileMP3)) ||
           IsUnicodeStr(GetTrackTotalSI(pFileMP3)) ||
           IsUnicodeStr(GetDiscNumberSI(pFileMP3)) ||
           IsUnicodeStr(GetDiscTotalSI(pFileMP3)) ||
           IsUnicodeStr(GetCommentSI(pFileMP3)) ||
           IsUnicodeStr(GetCopyrightSI(pFileMP3)) ||
           IsUnicodeStr(GetWriterSI(pFileMP3)) ||
           IsUnicodeStr(GetComposerSI(pFileMP3)) ||
           IsUnicodeStr(GetAlbumArtistSI(pFileMP3)) ||
           IsUnicodeStr(GetOrigArtistSI(pFileMP3)) ||
           IsUnicodeStr(GetURLSI(pFileMP3)) ||
           IsUnicodeStr(GetEncodest(pFileMP3)) ||
           IsUnicodeStr(GetEngineerSI(pFileMP3)) ||
           IsUnicodeStr(GetGenreSI(pFileMP3)) ||
           0){
            id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16);
        }
    }
#endif

    // *** タグ情報の設定 ***
    id3v2.SetTitle(GetTrackNameSI(pFileMP3));           // タイトル
    id3v2.SetArtist(GetArtistNameSI(pFileMP3));         // アーティスト名
    id3v2.SetAlbum(GetAlbumNameSI(pFileMP3));           // アルバム名
    id3v2.SetYear(GetYearSI(pFileMP3));                 // リリース
    id3v2.SetEncoder(GetSoftwareSI(pFileMP3));          // エンコーダ
    {//トラック番号/トラック数
     //トラック数用の ID が ID3v2 にはないので "トラック番号/トラック数" の
     //ような文字列に変換してトラック数に書き込む
        CString strTrackNumber = GetTrackNumberSI(pFileMP3);
        CString strTrackTotal = GetTrackTotalSI(pFileMP3);
        if(!strTrackNumber.IsEmpty() && !strTrackTotal.IsEmpty()){
            strTrackNumber = strTrackNumber + _T("/") + strTrackTotal;
        }
        id3v2.SetTrackNo(strTrackNumber);
    }
    {//ディスク番号/ディスク数
     //ディスク数用の ID が ID3v2 にはないので "ディスク番号/ディスク数" の
     //ような文字列に変換してディスク数に書き込む
        CString strDiscNumber = GetDiscNumberSI(pFileMP3);
        CString strDiscTotal = GetDiscTotalSI(pFileMP3);
        if(!strDiscNumber.IsEmpty() && !strDiscTotal.IsEmpty()){
            strDiscNumber = strDiscNumber + _T("/") + strDiscTotal;
        }
        id3v2.SetDiscNo(strDiscNumber);
    }
    id3v2.SetComment(GetCommentSI(pFileMP3));           // コメント
    id3v2.SetCopyright(GetCopyrightSI(pFileMP3));       // 著作権
    id3v2.SetWriter(GetWriterSI(pFileMP3));             // 作詞
    id3v2.SetComposer(GetComposerSI(pFileMP3));         // 作曲
    id3v2.SetAlbumArtist(GetAlbumArtistSI(pFileMP3));   // Albm. アーティスト
    id3v2.SetOrigArtist(GetOrigArtistSI(pFileMP3));     // Orig.アーティスト
    id3v2.SetUrl(GetURLSI(pFileMP3));                   // URL
    id3v2.SetEncodedBy(GetEncodest(pFileMP3));          // エンコードした人
    id3v2.SetEngineer(GetEngineerSI(pFileMP3));         // エンジニア（出版）
    // ジャンル名
    CString strGenre;
    BYTE    byGenre;
    byGenre = STEPGetGenreCode(GetGenreSI(pFileMP3));
    if (!_tcslen(GetGenreSI(pFileMP3)) == 0) {
        if (byGenre == (BYTE)0xff || STEPIsUserGenre(GetGenreSI(pFileMP3)) || bOptID3v2GenreAddNumber == false) strGenre.Format(_T("%s"), GetGenreSI(pFileMP3));
        else                       strGenre.Format(_T("(%d)%s"), STEPGetGenreCode(GetGenreSI(pFileMP3)), GetGenreSI(pFileMP3));
    }
    id3v2.SetGenre(strGenre/* 2005.08.23 del , bOptID3v2GenreAddNumber*/);

    id3v2.SetUnSynchronization(bOptUnSync);//フレーム非同期化
    // *** タグ情報を更新する ***
    bool result = id3v2.Save(GetFullPath(pFileMP3)) == ERROR_SUCCESS;
    if (result) {
        setFileType(id3v2, pFileMP3);
    }
    return result;
}

bool WriteTagSIF(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (_tcslen(GetSoftwareSI(pFileMP3)) == 0) {
        SetSoftwareSI(pFileMP3, strOptSoftwareTag);
    }

    CRMP rmp/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    if (rmp.Load(GetFullPath(pFileMP3))  != ERROR_SUCCESS   // 読み込み失敗
        ||  rmp.IsEnable() == FALSE) {
        // 読み込み失敗
        return(false);
    }

    bool isNeedID3 = false;
    if (_tcslen(GetTrackName(pFileMP3)) > 0)    isNeedID3 = true;
    if (_tcslen(GetArtistName(pFileMP3))> 0)    isNeedID3 = true;
    if (_tcslen(GetAlbumName(pFileMP3)) > 0)    isNeedID3 = true;
    if (_tcslen(GetComment(pFileMP3))   > 0)    isNeedID3 = true;
    if (_tcslen(GetYear(pFileMP3))      > 0)    isNeedID3 = true;
    if (_tcslen(GetGenre(pFileMP3))     > 0)    isNeedID3 = true;
    if (_tcslen(GetTrackNumber(pFileMP3))> 0)   isNeedID3 = true;
    if (isNeedID3 && !rmp.HasId3tag()) {
        rmp.SetHasId3tag(TRUE);
    }

    // SI フィールドを更新
    rmp.SetNAM(GetTrackNameSI(pFileMP3));   // 曲名
    rmp.SetART(GetArtistNameSI(pFileMP3));  // アーティスト名
    rmp.SetPRD(GetAlbumNameSI(pFileMP3));   // 製品名
    rmp.SetCMT(GetCommentSI(pFileMP3));     // コメント文字列
    rmp.SetCRD(GetYearSI(pFileMP3));        // リリース年号
    rmp.SetGNR(GetGenreSI(pFileMP3));       // ジャンル名
    rmp.SetCOP(GetCopyrightSI(pFileMP3));   // 著作権
    rmp.SetENG(GetEngineerSI(pFileMP3));    // エンジニア
    rmp.SetSRC(GetSourceSI(pFileMP3));      // ソース
    rmp.SetSFT(GetSoftwareSI(pFileMP3));    // ソフトウェア
    rmp.SetKEY(GetKeywordSI(pFileMP3));     // キーワード
    rmp.SetTCH(GetTechnicianSI(pFileMP3));  // 技術者
    rmp.SetLYC(GetLyricSI(pFileMP3));       // 歌詞
    rmp.SetCMS(GetCommissionSI(pFileMP3));  // コミッション
    // SI フィールドを更新
    bool result = rmp.Save(GetFullPath(pFileMP3)) == ERROR_SUCCESS ? true : false;
    if (result) {
        rmp.Release();
        if (isNeedID3) {
            result = WriteTagID3(pFileMP3);
        }
        SetFormat(pFileMP3, nFileTypeRMP);      // ファイル形式：RIFF MP3
        SetFileTypeName(pFileMP3, _T("RIFF MP3"));
    }
    return result;
}

static bool CheckLimitId3v1(const TCHAR *str, int nLimit, bool bCheckUnicode)
{//by Kobarin
 //id3v1 の制限内なら true を、制限を超えていたら false を返す
 //文字列 str を ANSI に変換して長さが nLimit を超えたら true を返す
 //bCheckUnicode == true の場合は UNICODE 固有文字を使っているかどうかも調べる
 //UNICODE 版では str が UNICODE 固有文字を使用している場合も false を返す
#ifndef _UNICODE
    return (strlen(str) <= nLimit);//長さの確認だけで OK(str は元々 ANSI なので)
#else
    int len_ansi = WideCharToMultiByte(CP_ACP, 0, str, -1, 0, 0, NULL, NULL);
    char *str_ansi = (char*)malloc(len_ansi);
    WideCharToMultiByte(CP_ACP, 0, str, -1, str_ansi, len_ansi, NULL, NULL);
    bool bRet = (strlen(str_ansi) <= nLimit);
    if(bRet && bCheckUnicode){//長さは OK
    //UTF16 に戻して元の文字列と一致するか確認
        int len_utf16 = MultiByteToWideChar(CP_ACP, 0, str_ansi, -1, 0, 0);
        WCHAR *str_utf16 = (WCHAR*)malloc(len_utf16*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str_ansi, -1, str_utf16, len_utf16);
        if(wcscmp(str, str_utf16) != 0){//元の文字列と不一致
            bRet = false;//UNICODE 固有文字を使用している
        }
        free(str_utf16);
    }
    free(str_ansi);
    return bRet;
#endif
}

bool IsCreateID3v2(FILE_INFO *pFileMP3)
{//ID3v2 に自動変換すべきかどうかの判別
 //長さに加えて UNICODE 固有文字を使っているかの判別も行うようにした
    if (!CheckLimitId3v1(GetTrackName(pFileMP3), ID3_LEN_TRACK_NAME, true)) return true;
    if (!CheckLimitId3v1(GetArtistName(pFileMP3), ID3_LEN_ARTIST_NAME, true))   return true;
    if (!CheckLimitId3v1(GetAlbumName(pFileMP3), ID3_LEN_ALBUM_NAME, true)) return true;
    if (!CheckLimitId3v1(GetComment(pFileMP3), ID3_LEN_COMMENT-2, true))        return true;
    if (!CheckLimitId3v1(GetYear(pFileMP3), ID3_LEN_YEAR, true))        return true;

    if (STEPIsUserGenre(GetGenreSI(pFileMP3)))                  return true;
    if (CString(GetComment(pFileMP3)).Find('\n') > -1)          return true;
    if (!STEPIsNumeric(GetTrackNumberSI(pFileMP3)))             return true;
    /*
    if (_tcslen(GetCopyrightSI(pFileMP3)) > 0)                  return true;    // 著作権
    if (_tcslen(GetComposerSI(pFileMP3)) > 0)                   return true;    // 作曲
    if (_tcslen(GetOrigArtistSI(pFileMP3)) > 0)                 return true;    // Orig.アーティスト
    if (_tcslen(GetURLSI(pFileMP3)) > 0)                            return true;    // URL
    if (_tcslen(GetEncodest(pFileMP3)) > 0)                     return true;    // エンコードした人
    if (_tcslen(GetSoftwareSI(pFileMP3)) > 0
        && _tcscmp(GetSoftwareSI(pFileMP3), strOptSoftwareTag) != 0)        return true;    // ソフトウェア
    */
    return false;
}

bool IsCreateRMP(FILE_INFO *pFileMP3)
{//RMP(RIFF MP3)に変換すべきかどうかの判別
 //長さが ID3v1 の制限を超えてるかどうかだけで判別
    //UNICODE の場合、長さのチェックは _tcslen では不十分(マルチバイト文字も1文字として数えるので)
    //UNICODE 固有の文字を使用しているかどうかは調べない(どうせ UNICODE で書き込めないので)
    if (!CheckLimitId3v1(GetTrackName(pFileMP3), ID3_LEN_TRACK_NAME, false))    return true;
    if (!CheckLimitId3v1(GetArtistName(pFileMP3), ID3_LEN_ARTIST_NAME, false))  return true;
    if (!CheckLimitId3v1(GetAlbumName(pFileMP3), ID3_LEN_ALBUM_NAME, false))    return true;
    if (!CheckLimitId3v1(GetComment(pFileMP3), ID3_LEN_COMMENT-2, false))       return true;
    if (!CheckLimitId3v1(GetYear(pFileMP3), ID3_LEN_YEAR, false))       return true;

    if (STEPIsUserGenre(GetGenreSI(pFileMP3)))                  return true;
    return false;
}

bool IsCreateID3v2SI(FILE_INFO *pFileMP3, bool bID3v1Only = FALSE)
{//SIF の方に入力されたタグ情報から ID3v2 に自動変換すべきかどうかの判別？
    if (!CheckLimitId3v1(GetTrackNameSI(pFileMP3), ID3_LEN_TRACK_NAME, true))   return true;
    if (!CheckLimitId3v1(GetArtistNameSI(pFileMP3), ID3_LEN_ARTIST_NAME, true)) return true;
    if (!CheckLimitId3v1(GetAlbumNameSI(pFileMP3), ID3_LEN_ALBUM_NAME, true))   return true;
    if (!CheckLimitId3v1(GetCommentSI(pFileMP3), ID3_LEN_COMMENT-2, true))      return true;
    if (!CheckLimitId3v1(GetYearSI(pFileMP3), ID3_LEN_YEAR, true))      return true;

    if (STEPIsUserGenre(GetGenreSI(pFileMP3)))                      return true;
    if (CString(GetCommentSI(pFileMP3)).Find(_T('\n')) > -1)            return true;
    if (!STEPIsNumeric(GetTrackNumberSI(pFileMP3)))                 return true;
    //if (!STEPIsNumeric(GetDiscNumberSI(pFileMP3)))                  return true;//数値かどうかに関わらず Disc番号は ID3v1 では書き込めないような…・。
    if (bID3v1Only == TRUE) return false;
    if (_tcslen(GetTrackTotalSI(pFileMP3)) > 0)                     return true;    // トラック数
    if (_tcslen(GetDiscNumberSI(pFileMP3)) > 0)                     return true;    // ディスク番号
    if (_tcslen(GetDiscTotalSI(pFileMP3)) > 0)                      return true;    // ディスク数
    if (_tcslen(GetCopyrightSI(pFileMP3)) > 0)                      return true;    // 著作権
    if (_tcslen(GetComposerSI(pFileMP3)) > 0)                       return true;    // 作曲
    if (_tcslen(GetOrigArtistSI(pFileMP3)) > 0)                     return true;    // Orig.アーティスト
    if (_tcslen(GetAlbumArtistSI(pFileMP3)) > 0)                        return true;    // Albm.アーティスト
    if (_tcslen(GetWriterSI(pFileMP3)) > 0)                         return true;    // 作詞者
    if (_tcslen(GetURLSI(pFileMP3)) > 0)                                return true;    // URL
    if (_tcslen(GetEncodest(pFileMP3)) > 0)                         return true;    // エンコードした人
    if (_tcslen(GetEngineerSI(pFileMP3)) > 0)                       return true;    // エンジニア
    if (_tcslen(GetSoftwareSI(pFileMP3)) > 0
        && _tcscmp(GetSoftwareSI(pFileMP3), strOptSoftwareTag) != 0)        return true;    // ソフトウェア
    return false;
}

/*
bool IsMatchID3SI(FILE_INFO* pFileMP3)
{
    if (CString(GetTrackName(pFileMP3)) != GetTrackNameSI(pFileMP3))        return false;
    if (CString(GetArtistName(pFileMP3)) != GetArtistNameSI(pFileMP3))      return false;
    if (CString(GetAlbumName(pFileMP3)) != GetAlbumNameSI(pFileMP3))        return false;
    if (CString(GetComment(pFileMP3)) != GetCommentSI(pFileMP3))            return false;
    if (CString(GetYear(pFileMP3)) != GetYearSI(pFileMP3))                  return false;
    if (CString(GetGenre(pFileMP3)) != GetGenreSI(pFileMP3))                return false;
    if (CString(GetTrackNumber(pFileMP3)) != GetTrackNumberSI(pFileMP3))    return false;
    return true;
}*/

void copySIField(FILE_INFO *pFileMP3)
{
    if (GetFormat(pFileMP3) != nFileTypeRMP) {
        CString strTrackNumber = GetTrackNumberSI(pFileMP3);
        if (strTrackNumber.IsEmpty()) {
            SetBTrackNumber(pFileMP3, 0xff);
        } else {
            SetBTrackNumber(pFileMP3, (BYTE)STEPGetIntegerTrackNumber(strTrackNumber));
        }
    }

    CString strGenre = STEPGetGenreNameSIF(STEPGetGenreCode(GetGenreSI(pFileMP3)));
    SetGenre(pFileMP3, strGenre);
    SetBGenre(pFileMP3, STEPGetGenreCode(GetGenreSI(pFileMP3)));
}

STEP_API UINT WINAPI STEPSave(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    UINT nFormat = GetFormat(pFileMP3);

    CString strTrack = GetTrackNameSI(pFileMP3);

    if (nFormat == nFileTypeMP3 || nFormat == nFileTypeMP3V1 || nFormat == nFileTypeMP3V11) {
        // MP3 => ID3v2 自動フォーマット変換処理
        copySIField(pFileMP3);
        if (MakeTagID3v2(GetFullPath(pFileMP3), NULL) == false){
            return STEP_ERROR;
        }
        else {
            nFormat = nFileTypeID3V2;
            SetFormat(pFileMP3, nFileTypeID3V2);
        }
    }
    if (nFormat == nFileTypeID3V2) {
        // ID3tag の自動更新
        if (bOptID3v2ID3tagAutoWrite) {
            ConvSIFieldToID3tag(pFileMP3);
        }
        if (bOptID3v2ID3tagAutoDelete) {
            SetFormat(pFileMP3, nFileTypeID3V2);
        }
        else if (WriteTagID3(pFileMP3) == false) { // FileType設定のためID3v2の前に書き換え
            return STEP_ERROR;
        }
        if (WriteTagID3v2(pFileMP3) == false) {
            return STEP_ERROR;
        }
        if (bOptID3v2ID3tagAutoDelete) {
            // ID3 tagを削除
            DeleteTagID3v1(GetFullPath(pFileMP3), NULL);
            STEPInitDataID3(pFileMP3);
        }
        else if(ID3v1IsEmpty(pFileMP3)){//ID3v1 が空の場合は削除
            DeleteTagID3v1(GetFullPath(pFileMP3), NULL);
        }
        //LoadFileMP3(pFileMP3);//追加(更新後の実際の文字列データが表示に反映されるようにする)
        return STEP_SUCCESS;
    }
    if (nFormat == nFileTypeRMP) {
        // ID3tag の自動更新
        if (bOptRmpID3tagAutoWrite) {
            ConvSIFieldToID3tag(pFileMP3);
        }
        if (WriteTagSIF(pFileMP3) == false) {
            return STEP_ERROR;
        }
        if(ID3v1IsEmpty(pFileMP3)){//ID3v1 が空の場合は削除
            DeleteTagID3v1(GetFullPath(pFileMP3), NULL);
        }
        //LoadFileMP3(pFileMP3);//追加(更新後の実際の文字列データが表示に反映されるようにする)
        return STEP_SUCCESS;
    }
    return STEP_UNKNOWN_FORMAT;
}

STEP_API void WINAPI STEPShowOptionDialog(HWND hWnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CPropertySheet page;
    CDlgFileRmpID3v2 dlg1;
    CDlgDefaultValue dlg2;
    page.SetTitle(CString(STEPGetPluginName()) + _T(" オプション設定"));
    dlg1.m_bRmpID3tagAutoWrite = bOptRmpID3tagAutoWrite;
    dlg1.m_bAutoConvRMP = true;//bOptAutoConvRMP;
    dlg1.m_nSIFieldConvertType = nOptSIFieldConvType;
    dlg1.m_bRmpGenreListSelect = bOptRmpGenreListSelect;
    dlg1.m_bChangeFileExt = bOptChangeFileExt;
    dlg1.m_bAutoConvID3v2 = bOptAutoConvID3v2;
    dlg1.m_bID3v2ID3tagAutoWrite = bOptID3v2ID3tagAutoWrite;
    dlg1.m_bID3v2GenreListSelect = bOptID3v2GenreListSelect;
    dlg1.m_bID3v2GenreAddNumber = bOptID3v2GenreAddNumber;
    dlg1.m_bID3v2Id3tagAutoDelete = bOptID3v2ID3tagAutoDelete;
    page.AddPage(&dlg1);
    dlg2.m_strSoftwareTag = strOptSoftwareTag;
    dlg2.m_bID3v2UnSync = bOptUnSync;
    dlg2.m_nId3v2Encode = nId3v2Encode;
    dlg2.m_nId3v2Version = nId3v2Version;
    dlg2.m_nId3v2EncodeNew = nId3v2EncodeNew;
    dlg2.m_nId3v2VersionNew = nId3v2VersionNew;

    page.AddPage(&dlg2);
    if (page.DoModal() == IDOK) {
        bOptRmpID3tagAutoWrite = dlg1.m_bRmpID3tagAutoWrite ? true : false;
        //bOptAutoConvRMP = dlg1.m_bAutoConvRMP ? true : false;
        nOptSIFieldConvType = dlg1.m_nSIFieldConvertType;
        bOptRmpGenreListSelect = dlg1.m_bRmpGenreListSelect ? true : false;
        bOptChangeFileExt = dlg1.m_bChangeFileExt ? true : false;
        //bOptAutoConvID3v2 = dlg1.m_bAutoConvID3v2 ? true : false;
        bOptID3v2ID3tagAutoWrite = dlg1.m_bID3v2ID3tagAutoWrite ? true : false;
        bOptID3v2GenreListSelect = dlg1.m_bID3v2GenreListSelect ? true : false;
        bOptID3v2GenreAddNumber = dlg1.m_bID3v2GenreAddNumber ? true : false;
        bOptID3v2ID3tagAutoDelete = dlg1.m_bID3v2Id3tagAutoDelete ? true : false;
        strOptSoftwareTag = dlg2.m_strSoftwareTag;
        bOptUnSync = dlg2.m_bID3v2UnSync ? true : false;
        nId3v2Encode = dlg2.m_nId3v2Encode;
        nId3v2Version = dlg2.m_nId3v2Version;
        nId3v2EncodeNew = dlg2.m_nId3v2EncodeNew;
        nId3v2VersionNew = dlg2.m_nId3v2VersionNew;
        //INIに保存
        //WritePrivateProfileString はファイルが存在しない場合や、
        //元ファイルが ANSI だと ANSI で文字列を書き込む
        CIniFile iniFile(strINI);
        //iniFile.WriteInt(_T("RMP_ID3V2"), _T("AutoConvID3v2"), bOptAutoConvID3v2);//廃止
        //iniFile.WriteInt(_T("RMP_ID3V2"), _T("AutoConvRMP"), bOptAutoConvRMP);   //廃止
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("SIFieldConvType"), nOptSIFieldConvType == SIF_CONV_ALL_FIELD);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("ID3v2ID3tagAutoWrite"), bOptID3v2ID3tagAutoWrite);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("RmpID3tagAutoWrite"), bOptRmpID3tagAutoWrite);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("ID3v2GenreListSelect"), bOptID3v2GenreListSelect);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("RmpGenreListSelect"), bOptRmpGenreListSelect);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("ID3v2ID3tagAutoDelete"), bOptID3v2ID3tagAutoDelete);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("ID3v2GenreAddNumber"), bOptID3v2GenreAddNumber);
        iniFile.WriteInt(_T("RMP_ID3V2"), _T("ChangeFileExt"), bOptChangeFileExt);
        //OTHER
        iniFile.WriteStr(_T("OTHER"), _T("SoftwareTag"), strOptSoftwareTag);
        iniFile.WriteInt(_T("OTHER"), _T("ID3v2UnSync"), bOptUnSync);
        iniFile.WriteInt(_T("OTHER"), _T("ID3v2CharEncode"), nId3v2Encode);
        iniFile.WriteInt(_T("OTHER"), _T("ID3v2Version"), nId3v2Version);
        iniFile.WriteInt(_T("OTHER"), _T("ID3v2CharEncodeNew"), nId3v2EncodeNew);
        iniFile.WriteInt(_T("OTHER"), _T("ID3v2VersionNew"), nId3v2VersionNew);

        iniFile.Flush();//保存実行(デストラクタで Flush は呼ばれない)

        STEPUpdateCellInfo();
    }
}

STEP_API LPCTSTR WINAPI STEPGetToolTipText(UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nID == nIDFileConvMP3) {
        return _T("ID3v1 に変換(非推奨)");
    }
    if (nID == nIDFileConvID3v2) {
        return _T("ID3v2 に変換");
    }
    if (nID == nIDFileConvRMP) {
        return _T("RIFF MP3 に変換(非推奨)");
    }
    if (nID == nIDDeleteID3) {
        return _T("ID3v1/v2 を削除");
    }
    //if (nID == nIDConvID3v2Version) {
    //    return _T("ID3v2バージョンの変換");
    //}
    return NULL;
}

STEP_API LPCTSTR WINAPI STEPGetStatusMessage(UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    //if (nID == nIDFileConvAutoID3) {
    //    return _T("タグの入力桁数/内容によって標準MP3形式またはID3v2形式のいずれかに変換します");
    //}
    if (nID == nIDFileConvMP3) {
        return _T("選択中の MP3 ファイルを ID3v1 に変換します。ID3v1 への変換はお勧めしません。通常は ID3v2 の使用をお勧めします。");
    }
    if (nID == nIDFileConvID3v2) {
        return _T("選択中の MP3 ファイルを ID3v2 に変換します。ID3 のバージョンと文字エンコード、フレーム非同期化の有無を指定出来ます。");
    }
    if (nID == nIDFileConvRMP) {
        return _T("選択中の MP3 ファイルを RIFF MP3 に変換します。RIFF MP3 への変換はお勧めしません。通常は ID3v2 の使用をお勧めします。");
    }
    if (nID == nIDDeleteID3) {
        return _T("選択中の MP3 ファイルから ID3v1/v2 を削除します");
    }
//    if (nID == nIDConvID3v2Version) {
//        return _T("ID3v2バージョン/文字エンコードの変換");
//    }
    return NULL;
}

STEP_API bool WINAPI STEPOnUpdateCommand(UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    //if (nID == nIDFileConvAutoID3) {
    //    if (STEPGetSelectedItem() < 0) return false;
    //    else return true;
    //}
    if (nID == nIDFileConvMP3) {
        if (STEPGetSelectedItem() < 0) return false;
        else return true;
    }
    if (nID == nIDFileConvID3v2) {
        if (STEPGetSelectedItem() < 0) return false;
        else return true;
    }
    if (nID == nIDFileConvRMP) {
        if (STEPGetSelectedItem() < 0) return false;
        else return true;
    }
    if (nID == nIDDeleteID3) {
        if (STEPGetSelectedItem() < 0) return false;
        else return true;
    }
//    if (nID == nIDConvID3v2Version) {
//        if (STEPGetSelectedItem() < 0) return false;
//        else return true;
//    }
    return false;
}

STEP_API bool WINAPI STEPOnCommand(UINT nID, HWND hWnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());//STEP 本体の関数を呼び出し前に解除しなければならない
    /*if (nID == nIDFileConvAutoID3) {
        static const TCHAR sMessage[] = _T("選択されているファイルを 標準MP3 形式 または ID3v2 形式に変換します\n\n")
                                        _T("変換を実行してもよろしいですか？");
        if (MessageBox(hWnd, sMessage, _T("標準MP3 形式/ID3v2 形式に変換"), MB_YESNO|MB_TOPMOST) == IDYES) {
            STEPProcessSelectedFilesForUpdate(_T("標準MP3形式/ID3v2 形式に変換中....."), ConvFileFormatAuto);
        }
        return true;
    }*/
    if (nID == nIDFileConvMP3) {
        static const TCHAR sMessage[] = _T("選択中の MP3 ファイルを ID3v1 に変換します。\n")
                                        _T("ID3v1 の使用は推奨しません。\n")
                                        _T("通常は ID3v2 の使用をお勧めします。\n\n")
                                        _T("ID3v1 には文字列長と扱える文字に制限があるため、変換後に\r\n")
                                        _T("情報の一部を失うことがあります。\n\n")
                                        _T("変換を実行してもよろしいですか？");
        if (MessageBox(hWnd, sMessage, _T("ID3v1 に変換"), MB_YESNO|MB_TOPMOST|MB_ICONWARNING) == IDYES) {
            STEPProcessSelectedFilesForUpdate(_T("ID3v1 に変換中....."), ConvFileFormatID3v1);
        }
        return true;
    }
    if (nID == nIDFileConvRMP) {
        static  const TCHAR sMessage[] = _T("選択中の MP3 ファイルを RIFF MP3 に変換します。\n")
                                         _T("RIFF MP3 の使用は推奨しません。\n")
                                         _T("通常は ID3v2 の使用をお勧めします。\n\n")
                                         _T("RIFF MP3 には扱える文字に制限があるため、変換後に\r\n")
                                         _T("情報の一部を失うことがあります。\r\n")
                                         _T("また対応ソフトが少ない為、ソフトによってはタグ情報を\r\n")
                                         _T("取得出来ないことがあります。\n\n")
                                         _T("変換を実行してもよろしいですか？");
        if (MessageBox(hWnd, sMessage, _T("RIFF MP3 に変換"), MB_YESNO|MB_TOPMOST|MB_ICONWARNING) == IDYES) {
            STEPProcessSelectedFilesForUpdate(_T("RIFF MP3 に変換中....."), ConvFileFormatRMP);
        }
        return true;
    }
    if (nID == nIDDeleteID3) {
        {
        static const TCHAR sMessage[] = _T("選択中の MP3 ファイルから ID3v1 を削除します\n\n")
                                        _T("実行してもよろしいですか？\r\n")
                                        _T("ID3v2 の削除はこのダイアログを閉じた後に確認します");
        int ret = MessageBox(hWnd, sMessage, _T("ID3v1 削除"), MB_YESNOCANCEL|MB_TOPMOST);
        if(ret == IDCANCEL){
            return true;
        }
        if(ret == IDYES) {
            STEPProcessSelectedFilesForUpdate(_T("ID3v1 を削除中....."), DeleteId3tagv1);
        }
        }
        {
        static const TCHAR sMessage[] = _T("選択中の MP3 ファイルから ID3v2 を削除します\n\n")
                                        _T("実行してもよろしいですか？\r\n")
                                        _T("RIFF MP3 のタグを削除するには、一度 ID3v1 か ID3v2 に変換して下さい\r\n");
        if (MessageBox(hWnd, sMessage, _T("ID3v2 削除"), MB_YESNO | MB_TOPMOST) == IDYES) {
            STEPProcessSelectedFilesForUpdate(_T("ID3v2 を削除中....."), DeleteId3tagv2);
        }
        }
        return true;
    }
    if (nID == nIDFileConvID3v2) {
        int ret;
        int nOldId3v2Encode = nId3v2Encode;
        int nOldId3v2Version = nId3v2Version;
        bool bOldOptUnSync = bOptUnSync;
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());//ダイアログを表示する間だけ有効
            CDlgConvID3v2Version dlg;
            dlg.m_nId3v2Encode = nId3v2Encode;
            dlg.m_nId3v2Version = nId3v2Version;
            dlg.m_bID3v2UnSync = bOptUnSync;
            ret = dlg.DoModal();
            if (ret == IDOK){
                nId3v2EncodeConv = dlg.m_nId3v2Encode;
                nId3v2VersionConv = dlg.m_nId3v2Version;
                bOptUnSyncConv = dlg.m_bID3v2UnSync;
            }
        }
        if (ret == IDOK){
            STEPProcessSelectedFilesForUpdate(_T("ID3v2 に変換 & ID3v2 バージョン/文字エンコードを変換中....."), ConvID3v2Version);
            nId3v2Encode = nOldId3v2Encode;
            nId3v2Version = nOldId3v2Version;
            bOptUnSync = bOldOptUnSync;
        }
        return true;
    }
    return false;
}

STEP_API void WINAPI STEPOnLoadMenu(HMENU hMenu, UINT nType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    switch (nType) {
    case MENU_SELECT_FOLDER:
    case MENU_FILE_RANGE:
    case MENU_FILE_EDIT_OK:
        AddConvMenu(hMenu);
        AddEditMenu(hMenu);
        break;
    }
}

STEP_API void WINAPI STEPOnLoadMainMenu()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // メニューへの追加
    HMENU hMenu = STEPGetMenu(MENU_CONV);
    AddConvMenu(hMenu);
    hMenu = STEPGetMenu(MENU_EDIT);
    AddEditMenu(hMenu);
}

STEP_API bool WINAPI STEPOnConvSiFieldToId3tag(FILE_INFO* pFileInfo)
{
    ConvSIFieldToID3tag(pFileInfo);
    return false;
}
