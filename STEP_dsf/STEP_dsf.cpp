// STEP_dsf.cpp : DLL の初期化ルーチンを定義します。
//

#include "stdafx.h"
#include "STEP_dsf.h"
#include "IniFile.h"
#include "Id3tagv2_for_dsf.h"
#include "STEPlugin.h"

#include "DlgConvID3v2Version.h"
#include "DlgSetup.h"
#include "DlgSetupOther.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DSF_FORMAT_COLUMN_DISP_MODE 0

BOOL GetPointerToMetadata(FILE_INFO *pFileMP3, UINT64 *pPointerToMetadata, BYTE pID3V2Head[4] = NULL);
BOOL ReadDsfHeader(FILE_INFO *pFileMP3, PSTDSFHEADER pStDsdHeader);
void MakeAudioFormat(PSTDSFHEADER pStDsdHeader, TCHAR *pcFormat, size_t sizeOfBuffer, int nDispType = 0);
bool LoadFileDSF(FILE_INFO *pFileMP3);
bool ReadTagID3v2(LPCTSTR sFileName, FILE_INFO *pFileMP3);
void setFileType(CId3tagv2& id3v2, FILE_INFO *pFileMP3);
bool WriteFileDSF(FILE_INFO *pFileMP3);
bool MakeTagID3v2(const TCHAR *sFileName, HWND hWnd);
bool WINAPI DeleteId3V2tag(FILE_INFO* pFileInfo, int nProcFlag, HWND hWnd);
bool DeleteTagID3v2(const TCHAR *sFileName, HWND hWnd);
bool WINAPI ConvID3v2Version(FILE_INFO* pFileInfo, int nProcFlag, HWND hWnd);

//
//TODO: この DLL が MFC DLL に対して動的にリンクされる場合、
//        MFC 内で呼び出されるこの DLL からエクスポートされたどの関数も
//        関数の最初に追加される AFX_MANAGE_STATE マクロを
//        持たなければなりません。
//
//        例:
//
//        extern "C" BOOL PASCAL EXPORT ExportedFunction()
//        {
//            AFX_MANAGE_STATE(AfxGetStaticModuleState());
//            // 通常関数の本体はこの位置にあります
//        }
//
//        このマクロが各関数に含まれていること、MFC 内の
//        どの呼び出しより優先することは非常に重要です。
//        it は、次の範囲内で最初のステートメントとして表示されるべきです
//        らないことを意味します、コンストラクターが MFC
//        DLL 内への呼び出しを行う可能性があるので、オブ
//        ジェクト変数の宣言よりも前でなければなりません。
//
//        詳細については MFC テクニカル ノート 33 および
//        58 を参照してください。
//

// CSTEPdsfApp

BEGIN_MESSAGE_MAP(CSTEPdsfApp, CWinApp)
END_MESSAGE_MAP()


// CSTEPdsfApp の構築

CSTEPdsfApp::CSTEPdsfApp()
{
    // TODO: この位置に構築用コードを追加してください。
    // ここに InitInstance 中の重要な初期化処理をすべて記述してください。
}


// 唯一の CSTEPdsfApp オブジェクト

CSTEPdsfApp theApp;

UINT nPluginID;
UINT nFileTypeDSF;

CIniFile IniFile;
CString strINI;
//bool    bOptNotUnSyncAlways;        extern const bool bOptNotUnSyncAlways_default = true;
//bool    bOptUnSyncNew;              extern const bool bOptUnSyncNew_default = true;
static int     nAudioFormatDispType;       extern const int nAudioFormatDispType_default = 0;        // オーディオフォーマット表示タイプ ※フォーマットカラムの表示パターン
static bool    bOptID3v2GenreListSelect;   extern const bool bOptID3v2GenreListSelect_default = false;

static int     nId3v2Encode;               extern const int  nId3v2Encode_default = 0; //ID3v2 文字エンコード(変更しない)
static int     nId3v2Version;              extern const int  nId3v2Version_default = 0;//ID3v2 バージョン(変更しない)
static int     nId3v2EncodeNew;            extern const int  nId3v2EncodeNew_default = 1; //新規作成時 ID3v2 文字エンコード(UTF16)
static int     nId3v2VersionNew;           extern const int  nId3v2VersionNew_default = 1;//新規作成時 ID3v2 バージョン(v2.3)
static int     nId3v2EncodeConv;
static int     nId3v2VersionConv;

static bool    bAutoISO8859_1toUtf16 = true;

// コマンドID
UINT nIDDeleteID3;
UINT nIDConvID3v2Version;


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

// CSTEPdsfApp の初期化

BOOL CSTEPdsfApp::InitInstance()
{
    CWinApp::InitInstance();

    return TRUE;
}

void AddConvMenu(HMENU hMenu) {
    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_SEPARATOR, 0, NULL);
    InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDConvID3v2Version, _T("ID3v2バージョン/文字エンコードの変換(DSF)"));
}

void AddEditMenu(HMENU hMenu) {
    TCHAR szMenu[100];
    MENUITEMINFO mii = { 0 };
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    bool bAddDeleteID3 = false;
    for (int iPos = 0; iPos < GetMenuItemCount(hMenu); iPos++) {
        mii.dwTypeData = (LPTSTR)szMenu;
        mii.cch = sizeof(szMenu) / sizeof(TCHAR) - 1;
        if (GetMenuItemInfo(hMenu, iPos, true, &mii)) {
            if (mii.fType == MFT_STRING) {
                if (_tcscmp(szMenu, _T("編集前の状態に戻す")) == 0) {
                    InsertMenu(hMenu, iPos, MF_BYPOSITION | MFT_STRING, nIDDeleteID3, _T("ID3v2タグを削除(DSF)"));
                    bAddDeleteID3 = true;
                    iPos++;
                }
            }
        }
    }

    if (!bAddDeleteID3) {
        InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDDeleteID3, _T("ID3v2タグを削除(DSF)"));
    }
}

extern "C" STEP_API bool WINAPI STEPInit(UINT pID, LPCTSTR szPluginFolder)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (Initialize() == false) return false;
    nPluginID = pID;

    // INIファイルの読み込み
    strINI = szPluginFolder;
    strINI += "STEP_dsf.ini";
    IniFile.SetIniFile(strINI);
    BOOL b;
    if (IniFile.GetPrivateProfile(_T("DSF"), _T("GenreListSelect"), -1, &b) == -1){
        bOptID3v2GenreListSelect = bOptID3v2GenreListSelect_default;
        IniFile.WritePrivateProfile(_T("DSF"), _T("GenreListSelect"), bOptID3v2GenreListSelect);
    }
    else{
        bOptID3v2GenreListSelect = (b != 0);
    }

    // オーディオフォーマット表示タイプ 0～3の4タイプ
    if (IniFile.GetPrivateProfile(_T("DSF"), _T("FormatDispType"), -1, (INT*)&nAudioFormatDispType) == -1){
        nAudioFormatDispType =  nAudioFormatDispType_default;
        IniFile.WritePrivateProfile(_T("DSF"), _T("FormatDispType"), nAudioFormatDispType);
    }
    if(nAudioFormatDispType < 0){
        nAudioFormatDispType = 0;
        IniFile.WritePrivateProfile(_T("DSF"), _T("FormatDispType"), nAudioFormatDispType);
    }
    if(nAudioFormatDispType > 3){
        nAudioFormatDispType = 3;
        IniFile.WritePrivateProfile(_T("DSF"), _T("FormatDispType"), nAudioFormatDispType);
    }

#if 0
    if (IniFile.GetPrivateProfile(_T("OTHER"), _T("ID3v2NotUnSync"), -1, (INT*)&bOptNotUnSyncAlways) == -1) {
        bOptNotUnSyncAlways = bOptNotUnSyncAlways_default;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2NotUnSync"), bOptNotUnSyncAlways);
    }
    if (IniFile.GetPrivateProfile(_T("OTHER"), _T("ID3v2UnSyncNew"), -1, (INT*)&bOptUnSyncNew) == -1) {
        bOptUnSyncNew = bOptUnSyncNew_default;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2UnSyncNew"), bOptUnSyncNew);
    }
#else
    // ID3v2非同期化が全く不要ならこっちを有効にしてビルドする事。
//    bOptNotUnSyncAlways = bOptNotUnSyncAlways_default;
//    bOptUnSyncNew = bOptUnSyncNew_default;
#endif
    if (IniFile.GetPrivateProfile(_T("OTHER"), _T("ID3v2CharEncode"), -1, (INT*)&nId3v2Encode) == -1){
        nId3v2Encode = nId3v2Encode_default;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2CharEncode"), nId3v2Encode);
    }
    if (IniFile.GetPrivateProfile(_T("OTHER"), _T("ID3v2Version"), -1, (INT*)&nId3v2Version) == -1){
        nId3v2Version = nId3v2Version_default;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2Version"), nId3v2Version);
    }
    if (IniFile.GetPrivateProfile(_T("OTHER"), _T("ID3v2CharEncodeNew"), -1, (INT*)&nId3v2EncodeNew) == -1){
        nId3v2EncodeNew = nId3v2EncodeNew_default;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2CharEncodeNew"), nId3v2EncodeNew);
    }
    if (IniFile.GetPrivateProfile(_T("OTHER"), _T("ID3v2VersionNew"), -1, (INT*)&nId3v2VersionNew) == -1){
        nId3v2VersionNew = nId3v2VersionNew_default;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2VersionNew"), nId3v2VersionNew);
    }

    //値の範囲チェック(ユーザーが手動でiniを書き換えた場合に念の為)
    switch (nId3v2Encode) {
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
    if(nId3v2Encode == 2 && nId3v2Version != 2){
        nId3v2Encode = 1;//UTF16 にする
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2CharEncode"), nId3v2Encode);
    }
    if(nId3v2EncodeNew == 2 && nId3v2VersionNew != 2){//UTF8
        nId3v2EncodeNew = 1;//UTF16 にする
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2CharEncodeNew"), nId3v2EncodeNew);
    }

    // サポートしている拡張子を登録
    HBITMAP hFLACBitmap = LoadBitmap(theApp.m_hInstance, MAKEINTRESOURCE(IDB_BITMAP_DSF));
    nFileTypeDSF = STEPRegisterExt(nPluginID, _T("dsf"), hFLACBitmap);
    DeleteObject(hFLACBitmap);

    nIDDeleteID3 = STEPGetCommandID();
    STEPKeyAssign(nIDDeleteID3, _T("ID3v2タグを削除(DSF)"), _T("STEP_dsf_KEY_DELETE_ID3"));

    nIDConvID3v2Version = STEPGetCommandID();
    STEPKeyAssign(nIDConvID3v2Version, _T("ID3v2バージョン/文字エンコードの変換(DSF)"), _T("STEP_dsf_KEY_CONV_ID3V2_VERSION"));

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
    return _T("STEP_dsf");
}

STEP_API LPCTSTR WINAPI STEPGetPluginInfo(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return _T("Version 1.01 Copyright (C) 2019-2019 DJ TOYO\r\n")
           _T("DSF(ID3v2)形式をサポートしています");
}

STEP_API bool WINAPI STEPSupportSIF(UINT nFormat) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return true;
}

STEP_API bool WINAPI STEPSupportTrackNumberSIF(UINT nFormat) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return true;
}


extern "C" STEP_API CONTROLTYPE WINAPI STEPGetControlType(UINT nFormat, COLUMNTYPE nColumn, bool isEditSIF)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!isEditSIF) return _NULL;
    switch (nColumn) {
    case COLUMN_TRACK_NAME:
    case COLUMN_ARTIST_NAME:
    case COLUMN_ALBUM_NAME:
    case COLUMN_ALBM_ARTIST:
    case COLUMN_TRACK_NUMBER:
    case COLUMN_TRACK_TOTAL:
    case COLUMN_DISC_NUMBER:
    case COLUMN_DISC_TOTAL:
    case COLUMN_YEAR:
    case COLUMN_COMPOSER:
    case COLUMN_ORIG_ARTIST:
    case COLUMN_SOFTWARE:
    case COLUMN_WRITER:
    case COLUMN_COPYRIGHT:
    case COLUMN_URL:
    case COLUMN_ENCODEST:
        return _EDIT;
    case COLUMN_COMMENT:
        return _MEDIT;
    case COLUMN_GENRE:
        if (bOptID3v2GenreListSelect) {
            return _CBOX;
        } else {
            return _EDIT;
        }
    }
    return _NULL;
}

extern "C" STEP_API UINT WINAPI STEPGetColumnMax(UINT nFormat, COLUMNTYPE nColumn, bool isEditSIF) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
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
        return 3;
    }
    return 0;
}

STEP_API UINT WINAPI STEPLoad(FILE_INFO *pFileMP3, LPCTSTR szExt)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (_tcsicmp(szExt, _T("dsf")) == 0) {
        if (LoadFileDSF(pFileMP3) == false) {
            CString strMsg;
            strMsg.Format(_T("%s の読み込みに失敗しました"), GetFullPath(pFileMP3));
            MessageBox(NULL, strMsg, _T("DSFファイルの読み込み失敗"), MB_ICONSTOP|MB_OK|MB_TOPMOST);
            return STEP_ERROR;
        } else {
            return STEP_SUCCESS;
        }
    }
    return STEP_UNKNOWN_FORMAT;
}

STEP_API UINT WINAPI STEPSave(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    UINT nFormat = GetFormat(pFileMP3);

    if (nFormat == nFileTypeDSF) {
        if (WriteFileDSF(pFileMP3) == false) {
            CString strMsg;
            strMsg.Format(_T("%s の書き込みに失敗しました"), GetFullPath(pFileMP3));
            MessageBox(NULL, strMsg, _T("DSFファイルの書き込み失敗"), MB_ICONSTOP|MB_OK|MB_TOPMOST);
            return STEP_ERROR;
        }
        return STEP_SUCCESS;
    }
    return STEP_UNKNOWN_FORMAT;
}

STEP_API void WINAPI STEPShowOptionDialog(HWND hWnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CDlgSetup dlg1;
    CDlgSetupOther dlg2;
    CPropertySheet page;

    dlg1.m_ddxnFormatType = nAudioFormatDispType;
    dlg1.m_bGenreListSelect = bOptID3v2GenreListSelect;
    page.AddPage(&dlg1);

    dlg2.m_nId3v2Encode = nId3v2Encode;
    dlg2.m_nId3v2Version = nId3v2Version;
    dlg2.m_nId3v2EncodeNew = nId3v2EncodeNew;
    dlg2.m_nId3v2VersionNew = nId3v2VersionNew;
    page.AddPage(&dlg2);

    page.SetTitle(CString(STEPGetPluginName()) + _T(" オプション設定"));
    if (page.DoModal() == IDOK) {
        nAudioFormatDispType = dlg1.m_ddxnFormatType;
        IniFile.WritePrivateProfile(_T("DSF"), _T("FormatDispType"), nAudioFormatDispType);
        bOptID3v2GenreListSelect = dlg1.m_bGenreListSelect ? true : false;
        IniFile.WritePrivateProfile(_T("DSF"), _T("GenreListSelect"), bOptID3v2GenreListSelect);

        nId3v2Encode = dlg2.m_nId3v2Encode;
        nId3v2Version = dlg2.m_nId3v2Version;
        nId3v2EncodeNew = dlg2.m_nId3v2EncodeNew;
        nId3v2VersionNew = dlg2.m_nId3v2VersionNew;
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2CharEncode"), nId3v2Encode);
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2Version"), nId3v2Version);
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2CharEncodeNew"), nId3v2EncodeNew);
        IniFile.WritePrivateProfile(_T("OTHER"), _T("ID3v2VersionNew"), nId3v2VersionNew);
    }
}

STEP_API LPCTSTR WINAPI STEPGetToolTipText(UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nID == nIDDeleteID3) {
        return _T("ID3v2タグを削除(DSF)");
    }
    if (nID == nIDConvID3v2Version) {
        return _T("ID3v2バージョンの変換(DSF)");
    }
    return NULL;
}

STEP_API LPCTSTR WINAPI STEPGetStatusMessage(UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nID == nIDDeleteID3) {
        return _T("選択されているDSFファイルのID3v2タグを削除します");
    }
    if (nID == nIDConvID3v2Version) {
        return _T("ID3v2バージョン/文字エンコードの変換(DSF)");
    }
    return NULL;
}

STEP_API bool WINAPI STEPOnUpdateCommand(UINT nID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (nID == nIDDeleteID3) {
        if (STEPGetSelectedItem() < 0) return false;
        else return true;
    }
    if (nID == nIDConvID3v2Version) {
        if (STEPGetSelectedItem() < 0) return false;
        else return true;
    }
    return false;
}

STEP_API bool WINAPI STEPOnCommand(UINT nID, HWND hWnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());//STEP 本体の関数を呼び出し前に解除しなければならない
    if (nID == nIDDeleteID3) {
        static  const TCHAR *sMessage = _T("選択されているファイルのID3v2タグを削除します\n\n")
                                        _T("実行してもよろしいですか？");
        if (MessageBox(hWnd, sMessage, _T("ID3v2タグ 削除(DSF)"), MB_YESNO|MB_TOPMOST) == IDYES) {
            STEPProcessSelectedFilesForUpdate(_T("ID3v2タグ を削除中....."), DeleteId3V2tag);
        }
        return true;
    }
    if (nID == nIDConvID3v2Version) {
        int ret;
        int nOldId3v2Encode = nId3v2Encode;
        int nOldId3v2Version = nId3v2Version;
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());//ダイアログを表示する間だけ有効
            CDlgConvID3v2Version dlg;
            dlg.m_nId3v2Encode = nId3v2Encode;
            dlg.m_nId3v2Version = nId3v2Version;
            ret = dlg.DoModal();
            if (ret == IDOK) {
                nId3v2EncodeConv = dlg.m_nId3v2Encode;
                nId3v2VersionConv = dlg.m_nId3v2Version;
            }
        }
        if (ret == IDOK) {
            //dlg.DoModal() == IDOK) {
            //nId3v2EncodeConv = dlg.m_nId3v2Encode;
            //nId3v2VersionConv = dlg.m_nId3v2Version;
            STEPProcessSelectedFilesForUpdate(_T("ID3v2 バージョン/文字エンコードを変換中....."), ConvID3v2Version);
            nId3v2Encode = nOldId3v2Encode;
            nId3v2Version = nOldId3v2Version;
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

BOOL GetPointerToMetadata(FILE_INFO *pFileMP3, UINT64 *pPointerToMetadata, BYTE pID3V2Head[4])
{
    UINT64 nMetaPos;
    *pPointerToMetadata = 0;

    CString strFileName = GetFullPath(pFileMP3);
    FILE *fp = _tfopen(strFileName, _T("rb"));
    if (!fp) {
        CString strMsg;
        strMsg.Format(_T("%s がオープンできませんでした"), strFileName);
        MessageBox(NULL, strMsg, _T("ファイルのオープン失敗"), MB_ICONSTOP | MB_OK | MB_TOPMOST);
        return FALSE;
    }

    // ファイルサイズ取得
    struct _stat64 Stat;
    _fstat64(_fileno(fp), &Stat);

    // Pointer To Metadata Chunkまでシーク
    _fseeki64(fp, 20, SEEK_SET);
    fread(&nMetaPos, 1, 8, fp);

    // メタデータの位置がファイルサイズを超えてたらタグ無しで扱う ※救済処置
    if(nMetaPos >= Stat.st_size){
        nMetaPos = 0;
    }

    // Pointer To Metadataが設定されているならID3V2Head位置までシーク
    if(nMetaPos > 0){
        _fseeki64(fp, nMetaPos, SEEK_SET);

        if(pID3V2Head != NULL){
            fread(pID3V2Head, 1, 4, fp);
        }
    } else {
        if (pID3V2Head != NULL) {
            pID3V2Head[0] = 0x00;
            pID3V2Head[1] = 0x00;
            pID3V2Head[2] = 0x00;
            pID3V2Head[3] = 0x00;
        }
    }

    fclose(fp);

    *pPointerToMetadata = nMetaPos;

    return TRUE;
}

BOOL ReadDsfHeader(FILE_INFO *pFileMP3, PSTDSFHEADER pStDsdHeader)
{
    memset(pStDsdHeader, NULL, sizeof(STDSFHEADER));

    CString strFileName = GetFullPath(pFileMP3);
    FILE *fp = _tfopen(strFileName, _T("rb"));
    if (!fp) {
        CString strMsg;
        strMsg.Format(_T("%s がオープンできませんでした"), strFileName);
        MessageBox(NULL, strMsg, _T("ファイルのオープン失敗"), MB_ICONSTOP | MB_OK | MB_TOPMOST);
        return FALSE;
    }

    _fseeki64(fp, 0, SEEK_SET);
    fread(pStDsdHeader, 1, sizeof(STDSFHEADER), fp);
    fclose(fp);

    // DSD Chunk Header Check
    if (memcmp(pStDsdHeader->stDsdChunk.cHeader, "DSD ", 4) != 0) {
        return FALSE;
    }
    // FMT Chunk Header Check
    if (memcmp(pStDsdHeader->stFmtChunk.cHeader, "fmt ", 4) != 0) {
        return FALSE;
    }

    return TRUE;
}

void MakeAudioFormat(PSTDSFHEADER pStDsdHeader, TCHAR *pcFormat, size_t sizeOfBuffer, int nDispType)
{
    double dSamplingFrequencyMHz;
    double dSamplingFrequencyFloorMHz;
    ULONG ulIntegerTmp;
    ULONG ulSamplingFrequency;
    ULONG ulDsdStanderdFormatChk[2];    // [0]:441KHz系チェック [1]:48KHz系チェック
    ULONG ulDsdTime;
    ULONG ulChannelNum;
    TCHAR format[256];

    memset(format,NULL,sizeof(format));

    // チャンネル数取得
    ulChannelNum = pStDsdHeader->stFmtChunk.ulChannelNum;

    // サンプリング周波数取得
    ulSamplingFrequency = pStDsdHeader->stFmtChunk.ulSamplingFrequency;

    ulDsdStanderdFormatChk[0] = ulSamplingFrequency % (44100 * 64); // DSD 2.8MHz(44.1KHz)系チェック 余りがなければ2822400Hzの倍数
    ulDsdStanderdFormatChk[1] = ulSamplingFrequency % (48000 * 64); // DSD 3.0MHz(48.0KHz)系チェック 余りがなければ3072000Hzの倍数

    // Hz → MHz計算
    dSamplingFrequencyMHz = (double)(ulSamplingFrequency / 1000000.0);

    // 小数点2位以下切り捨てMHz計算
    ulIntegerTmp = dSamplingFrequencyMHz * 10;
    dSamplingFrequencyFloorMHz = ulIntegerTmp / 10.0;

    // 標準(44.1KHz/48KHzの倍数)DSDサンプリング周波数？
    if (ulDsdStanderdFormatChk[0] == 0 || ulDsdStanderdFormatChk[1] == 0){
        // 44.1KHz/48KHz系のDSDサンプリング周波数のフォーマットテキストを作成
        // DSD倍数を計算
        if(ulDsdStanderdFormatChk[0] == 0){
            // 44.1KHz系の倍数
            ulDsdTime = ulSamplingFrequency / 44100;
        } else {
            // 48.0KHz系の倍数
            ulDsdTime = ulSamplingFrequency / 48000;
        }
        switch(nDispType){
            case 0:                        // フォーマットタイプ1 例"DSD64 / 2.8MHz, 2ch"
                _tcscpy_s(format, _T("DSD%lu / %0.1lfMHz, %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, ulDsdTime, dSamplingFrequencyFloorMHz, ulChannelNum);
                break;
            case 1:                        // フォーマットタイプ2 例"DSD64 / 2.822MHz, 2ch"
                _tcscpy_s(format, _T("DSD%lu / %0.3lfMHz, %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, ulDsdTime, dSamplingFrequencyMHz, ulChannelNum);
                break;
            case 2:                        // フォーマットタイプ3 例"DSD64 / 2.8MHz(2822400Hz), 2ch"
                _tcscpy_s(format, _T("DSD%lu / %0.1lfMHz(%luHz), %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, ulDsdTime, dSamplingFrequencyFloorMHz, ulSamplingFrequency, ulChannelNum);
                break;
            case 3:                        // フォーマットタイプ4 例"DSD64 / 2.822MHz(2822400Hz), 2ch"
                _tcscpy_s(format, _T("DSD%lu / %0.3lfMHz(%luHz), %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, ulDsdTime, dSamplingFrequencyMHz, ulSamplingFrequency, ulChannelNum);
                break;
            default:                    // 該当なし何も作らない
                break;
        }
    } else {
        // 44.1KHz/48KHz系以外のDSDサンプリング周波数のフォーマットテキストを作成 ※先頭の「DSD 64」を作らない
        switch(nDispType){
            case 0:                        // フォーマットタイプ1 例"DSD3.0MHz, 2ch"
                _tcscpy_s(format, _T("DSD%0.1lfMHz, %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, dSamplingFrequencyFloorMHz, ulChannelNum);
                break;
            case 1:                        // フォーマットタイプ2 例"DSD3.000MHz, 2ch"
                _tcscpy_s(format, _T("DSD%0.3lfMHz, %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, dSamplingFrequencyMHz, ulChannelNum);
                break;
            case 2:                        // フォーマットタイプ3 例"DSD3.0MHz(3000000Hz), 2ch"
                _tcscpy_s(format, _T("DSD%0.1lfMHz(%luHz), %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, dSamplingFrequencyFloorMHz, ulSamplingFrequency, ulChannelNum);
                break;
            case 3:                        // フォーマットタイプ4 例"DSD3.000MHz(3000000Hz), 2ch"
                _tcscpy_s(format, _T("DSD%0.3lfMHz(%luHz), %luch"));
                _sntprintf_s(pcFormat, sizeOfBuffer, _TRUNCATE, format, dSamplingFrequencyMHz, ulSamplingFrequency, ulChannelNum);
                break;
            default:                    // 該当なし何も作らない
                break;
        }
    }
}

bool LoadFileDSF(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    BYTE Head[4] = { 0 };
    UINT64 metapos;
    BOOL bRet;
    CString strFileName = GetFullPath(pFileMP3);
    STDSFHEADER StDsdHeader;
    TCHAR cAudioFormat[256];
    size_t uiSizeofFormatBuffer = sizeof(cAudioFormat) / sizeof(TCHAR);

    bRet = ReadDsfHeader(pFileMP3, &StDsdHeader);
    if(bRet == FALSE){
        CString strMsg;
        strMsg.Format(_T("%s がオープンできませんでした"), strFileName);
        MessageBox(NULL, strMsg, _T("ファイルのオープン失敗"), MB_ICONSTOP | MB_OK | MB_TOPMOST);
        return false;
    }

    bRet = GetPointerToMetadata(pFileMP3, &metapos, Head);
    if(bRet == FALSE){
        CString strMsg;
        strMsg.Format(_T("%s がオープンできませんでした"), strFileName);
        MessageBox(NULL, strMsg, _T("ファイルのオープン失敗"), MB_ICONSTOP | MB_OK | MB_TOPMOST);
        return false;
    }

    SetFormat(pFileMP3, nFileTypeDSF);
    SetFileTypeName(pFileMP3, _T("DSF"));

    MakeAudioFormat(&StDsdHeader, cAudioFormat, uiSizeofFormatBuffer, nAudioFormatDispType);
    SetAudioFormat(pFileMP3, cAudioFormat);

    if (memcmp(Head, "ID3", 3) == 0) {
        // === ID3v2 形式ファイル ===
        if (ReadTagID3v2(strFileName, pFileMP3) == false) {
            MessageBox(NULL, _T("ID3v2 タグの読み込みに失敗しました"), GetFullPath(pFileMP3), MB_ICONSTOP|MB_OK|MB_TOPMOST);
            return false;
        }
    }

    return true;
}

bool ReadTagID3v2(LPCTSTR sFileName, FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // ID3v2 タグの取得
    CId3tagv2   id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    if (id3v2.Load(sFileName) != ERROR_SUCCESS// 読み込み失敗
    ||  id3v2.IsEnable() == FALSE            // ID3v2 ではない
    ||  !TRUE/*id3v2.IsSafeVer()*/) {                // 未対応のバージョン
        // 読み込み失敗
        if (id3v2.GetVer() > 0x0000 && !TRUE/*id3v2.IsSafeVer()*/) {
            return(true);
        }
        return(false);
    }

    // *** タグ情報の設定 ***
    SetTrackNameSI(pFileMP3, id3v2.GetTitle());     // タイトル
    SetArtistNameSI(pFileMP3, id3v2.GetArtist());    // アーティスト名
    SetAlbumNameSI(pFileMP3, id3v2.GetAlbum());     // アルバム名
    SetYearSI(pFileMP3, id3v2.GetYear());            // リリース
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
    SetComposerSI(pFileMP3, id3v2.GetComposer());        // 作曲
    SetAlbumArtistSI(pFileMP3, id3v2.GetAlbumArtist()); // Albm. アーティスト
    SetOrigArtistSI(pFileMP3, id3v2.GetOrigArtist());    // Orig.アーティスト
    SetURLSI(pFileMP3, id3v2.GetUrl());                 // URL
    SetEncodest(pFileMP3, id3v2.GetEncodedBy());         // エンコードした人
    SetEngineerSI(pFileMP3,id3v2.GetEngineer());        // エンジニア（出版）

//    SetFileTypeName(pFileMP3, "MP3(ID3v2)");
    setFileType(id3v2, pFileMP3);

    return(true);
}

void setFileType(CId3tagv2& id3v2, FILE_INFO *pFileMP3)
{
    CString strFileTypeName;
    switch(id3v2.GetVer()){
    case 0x0200:
        strFileTypeName = _T("DSF(ID3v2.2");
        break;
    case 0x0300:
        strFileTypeName = _T("DSF(ID3v2.3");
        break;
    case 0x0400:
        strFileTypeName = _T("DSF(ID3v2.4");
        break;
    default:
        strFileTypeName = _T("DSF(ID3v2.?");
        break;
    }
#if 0
    if (GetFormat(pFileMP3) == nFileTypeMP3V1) {
        strFileTypeName = strFileTypeName + _T("+v1.0");
    }
    if (GetFormat(pFileMP3) == nFileTypeMP3V11) {
        strFileTypeName = strFileTypeName + _T("+v1.1");
    }
#endif
    strFileTypeName = strFileTypeName + _T(")");
    if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_ISO_8859_1) {
    } else if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_16) {
        strFileTypeName = strFileTypeName + _T(",UTF16");
    } else if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_16BE) {
        strFileTypeName = strFileTypeName + _T(",UTF16BE");
    } else if (id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_8) {
        strFileTypeName = strFileTypeName + _T(",UTF8");
    }
    //if (id3v2.GetUniocdeEncode()) {
    //    strFileTypeName = strFileTypeName + _T(",Uni");
    //}
    if (id3v2.GetUnSynchronization()) {
        strFileTypeName = strFileTypeName + _T(",US");
    }
    SetFileTypeName(pFileMP3, strFileTypeName);
    SetFormat(pFileMP3, nFileTypeDSF);          // ファイル形式：DSF(ID3v2)
}

bool WriteFileDSF(FILE_INFO *pFileMP3)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    UINT64 metapos;
    BOOL bRet;
    CString strFileName = GetFullPath(pFileMP3);

    bRet = GetPointerToMetadata(pFileMP3, &metapos);
    if(metapos == 0){
        if (MakeTagID3v2(strFileName, NULL) == false) {
            MessageBox(NULL, _T("ID3V2タグの作成に失敗しました"), strFileName, MB_ICONSTOP|MB_OK|MB_TOPMOST);
            return false;
        }
    }

    // ID3v2 タグの取得
    CId3tagv2    id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    if (id3v2.Load(GetFullPath(pFileMP3))  != ERROR_SUCCESS // 読み込み失敗
    ||    id3v2.IsEnable() == FALSE            // ID3v2 ではない
    ||    !TRUE/*id3v2.IsSafeVer()*/) {                // 未対応のバージョン
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
    //    id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16BE);
    //    break;
    default:
        break;
    }

    // ソフトウェアの設定
//    if (_tcslen(GetSoftwareSI(pFileMP3)) == 0) {
//        SetSoftwareSI(pFileMP3, strOptSoftwareTag);
//    }

#ifdef _UNICODE
    if(nId3v2Encode == 0 && //「ID3v2 の保存形式」-「文字エンコード」-「変更しない」
        id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_ISO_8859_1 && //更新前が ISO_8859_1
        bAutoISO8859_1toUtf16){
        //by Kobarin
        //変更しない設定でも、更新前の文字エンコードが ISO_8859_1 の場合は更新後の方で UNICODE 固有
        //の文字を使用しているかどうかをチェックし、使用している場合は UTF16 で書き込む
        if(IsUnicodeStr(GetTrackNameSI(pFileMP3))  ||
           IsUnicodeStr(GetArtistNameSI(pFileMP3)) ||
           IsUnicodeStr(GetAlbumNameSI(pFileMP3))  ||
           IsUnicodeStr(GetSoftwareSI(pFileMP3))   ||
           IsUnicodeStr(GetTrackNumberSI(pFileMP3))||
           IsUnicodeStr(GetTrackTotalSI(pFileMP3)) ||
           IsUnicodeStr(GetDiscNumberSI(pFileMP3)) ||
           IsUnicodeStr(GetDiscTotalSI(pFileMP3))  ||
           IsUnicodeStr(GetCommentSI(pFileMP3))    ||
           IsUnicodeStr(GetCopyrightSI(pFileMP3))  ||
           IsUnicodeStr(GetWriterSI(pFileMP3))     ||
           IsUnicodeStr(GetComposerSI(pFileMP3))   ||
           IsUnicodeStr(GetAlbumArtistSI(pFileMP3))||
           IsUnicodeStr(GetOrigArtistSI(pFileMP3)) ||
           IsUnicodeStr(GetURLSI(pFileMP3))        ||
           IsUnicodeStr(GetEncodest(pFileMP3))     ||
           IsUnicodeStr(GetEngineerSI(pFileMP3))   ||
           IsUnicodeStr(GetGenreSI(pFileMP3))      ||
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
        if (!strTrackNumber.IsEmpty() && !strTrackTotal.IsEmpty()) {
            strTrackNumber = strTrackNumber + _T("/") + strTrackTotal;
        }
        id3v2.SetTrackNo(strTrackNumber);
    }
    {//ディスク番号/ディスク数
     //ディスク数用の ID が ID3v2 にはないので "ディスク番号/ディスク数" の
     //ような文字列に変換してディスク数に書き込む
        CString strDiscNumber = GetDiscNumberSI(pFileMP3);
        CString strDiscTotal = GetDiscTotalSI(pFileMP3);
        if (!strDiscNumber.IsEmpty() && !strDiscTotal.IsEmpty()) {
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
#if 0
    CString strGenre;
    BYTE    byGenre;
    byGenre = STEPGetGenreCode(GetGenreSI(pFileMP3));
    if (!_tcslen(GetGenreSI(pFileMP3)) == 0) {
        if (byGenre == (BYTE)0xff || STEPIsUserGenre(GetGenreSI(pFileMP3)) || bOptID3v2GenreAddNumber == false) strGenre.Format(_T("%s"), GetGenreSI(pFileMP3));
        else                       strGenre.Format(_T("(%d)%s"), STEPGetGenreCode(GetGenreSI(pFileMP3)), GetGenreSI(pFileMP3));
    }
    id3v2.SetGenre(strGenre/* 2005.08.23 del , bOptID3v2GenreAddNumber*/);
#else
    id3v2.SetGenre(GetGenreSI(pFileMP3));
#endif

    id3v2.SetUnSynchronization(false);//フレーム非同期化オフ
    // *** タグ情報を更新する ***
    //return(id3v2.Save(/*AfxGetMainWnd()->GetSafeHwnd(),*/ GetFullPath(pFileMP3)) == ERROR_SUCCESS ? true : false);
    bool result = id3v2.Save(GetFullPath(pFileMP3)) == ERROR_SUCCESS ? true : false;
    if (result) {
        setFileType(id3v2, pFileMP3);
    }
    return result;
}

bool MakeTagID3v2(const TCHAR *sFileName, HWND hWnd)
{
    CId3tagv2    id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
    switch (nId3v2VersionNew) {
    case 0: // v2.2
        id3v2.SetVer(0x0200);
        break;
    case 1: // v2.3
        id3v2.SetVer(0x0300);
        break;
    case 2: // v2.4
        id3v2.SetVer(0x0400);
        break;
    default:
        break;
    }
    switch (nId3v2EncodeNew) {
    case 0:
        id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_ISO_8859_1);
        break;
    case 1:
        id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16);
        break;
    case 2:
        id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_8);
        break;
    //case 3:
    //    id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16BE);
    //    break;
    default:
        break;
    }

    if (id3v2.MakeTag(/*hWnd,*/ sFileName) != ERROR_SUCCESS) {
        // 変換失敗
        return(false);
    }
    return(true);
}

bool WINAPI DeleteId3V2tag(FILE_INFO* pFileInfo, int nProcFlag, HWND hWnd)
{
    if (nProcFlag == 3) {
        return false;
    }
    int nFormat = GetFormat(pFileInfo);
    if (!TYPE_IS_SUPPORT(nFormat)) { /* 2005.09.21 */
        return true;
    }
    // 事前にタグを更新
    if (isModify(pFileInfo)) {
        if (STEPWriteTag(pFileInfo) == false) {
            return false;
        }
    }
    bool result = DeleteTagID3v2(GetFullPath(pFileInfo), hWnd);
    STEPInitDataID3(pFileInfo);
    STEPInitDataSIF(pFileInfo);
    extern bool LoadFileDSF(FILE_INFO *pFileMP3);
    LoadFileDSF(pFileInfo);
//    CId3tagv2    id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
//    if (id3v2.Load(GetFullPath(pFileInfo)) != ERROR_SUCCESS    // 読み込み失敗
//    ||    id3v2.IsEnable() == FALSE            // ID3v2 ではない
//    ||    !TRUE/*id3v2.IsSafeVer()*/) {                // 未対応のバージョン
//        // 読み込み失敗
//        // タグなし
//        SetFileTypeName(pFileInfo, "MP3");
//        SetFormat(pFileInfo, nFileTypeMP3); // ファイル形式：MP3
//        return false;
//    }
//    extern void setFileType(CId3tagv2& id3v2, FILE_INFO *pFileMP3);
//    setFileType(id3v2, pFileInfo);
    return result;
}

bool DeleteTagID3v2(const TCHAR *sFileName, HWND hWnd)
{
    // ID3v2 タグの取得
    CId3tagv2    id3v2/*(USE_SCMPX_GENRE_ANIMEJ)*/;
#if 0	// 強制削除にする為、チェック不要にする
    if (id3v2.Load(sFileName) != ERROR_SUCCESS    // 読み込み失敗
    ||    id3v2.IsEnable() == FALSE            // ID3v2 ではない
    ||    !TRUE/*id3v2.IsSafeVer()*/) {                // 未対応のバージョン
        return false;
    }
#endif
    // ID3v2 タグの削除
    if (id3v2.DelTag(/*hWnd,*/ sFileName) != ERROR_SUCCESS) {
        return false;
    }
    return true;
}

bool WINAPI ConvID3v2Version(FILE_INFO* pFileInfo, int nProcFlag, HWND hWnd)
{
    if (nProcFlag == 3) {
        return false;
    }
    int nFormat = GetFormat(pFileInfo);
    if (!TYPE_IS_SUPPORT(nFormat)) { /* 2005.09.21 */
        return true;
    }
    // 事前にタグを更新
    if (isModify(pFileInfo)) {
        if (STEPWriteTag(pFileInfo) == false) {
            return false;
        }
    }
    if (GetFormat(pFileInfo) != nFileTypeDSF) return true;
    CId3tagv2    id3v2;
    if (id3v2.Load(GetFullPath(pFileInfo)) != ERROR_SUCCESS    // 読み込み失敗
    ||    id3v2.IsEnable() == FALSE            // ID3v2 ではない
    ||    !TRUE/*id3v2.IsSafeVer()*/) {                // 未対応のバージョン
        // 読み込み失敗
        return false;
    }
    CString szYear = id3v2.GetYear();//
    extern int nId3v2EncodeConv;
    extern int nId3v2VersionConv;
    extern int nId3v2Encode;
    extern int nId3v2Version;
    bool needConv = false;
    if (nId3v2VersionConv != 0) {
        // 変更しない以外
        switch (nId3v2VersionConv) {
        case 1: // v2.2
            if (id3v2.GetVer() != 0x0200) needConv = true;
            id3v2.SetVer(0x0200);
            break;
        case 2: // v2.3
            if (id3v2.GetVer() != 0x0300) needConv = true;
            id3v2.SetVer(0x0300);
            break;
        case 3: // v2.4
            if (id3v2.GetVer() != 0x0400) needConv = true;
            id3v2.SetVer(0x0400);
            break;
        default:
            break;
        }
    }
    //ややこし過ぎ…。
    if(nId3v2EncodeConv == 0){//文字エンコードを変更しない
        //id3v2.4以外に変更で、変更前が UTF8 の場合は UTF16 に変換
        if(id3v2.GetCharEncoding() == CId3tagv2::ID3V2CHARENCODING_UTF_8){
            if(id3v2.GetVer() != 0x0400){
                nId3v2EncodeConv = 2;
            }
        }
    }
    else if(nId3v2EncodeConv == 3){//UTF8
        //id3v2.4以外に変更の場合は UTF16 に変換
        if(id3v2.GetVer() != 0x0400){
            nId3v2EncodeConv = 2;
        }
    }
    if (nId3v2EncodeConv != 0) {
        //GetCharEncoding() の戻り値は必ずしも全てのフレームに対してのものではなく
        //実際はフレームごとに文字エンコードが異なる
        //ここではなるべく全てのフレームに対して指定した文字エンコードに変換する
        needConv = true;
        switch (nId3v2EncodeConv) {
        case 1:
        //    if (id3v2.GetCharEncoding() != CId3tagv2::ID3V2CHARENCODING_ISO_8859_1) needConv = true;
            id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_ISO_8859_1);
            break;
        case 2:
        //    if (id3v2.GetCharEncoding() != CId3tagv2::ID3V2CHARENCODING_UTF_16) needConv = true;
            id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16);
            break;
        case 3:
        //    if (id3v2.GetCharEncoding() != CId3tagv2::ID3V2CHARENCODING_UTF_8) needConv = true;
            id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_8);
            break;
        //case 4:
        //    if (id3v2.GetCharEncoding() != CId3tagv2::ID3V2CHARENCODING_UTF_16BE) needConv = true;
        //    id3v2.SetCharEncoding(CId3tagv2::ID3V2CHARENCODING_UTF_16BE);
        //    break;
        default:
            needConv = false;
            break;
        }
    }
    if (id3v2.GetUnSynchronization()) {//非同期化されてる場合はオフにする
        needConv = true;
    }
    if (needConv == false) return true;
    id3v2.SetUnSynchronization(false);//非同期化オフ
    id3v2.ApplyStringEncode();//文字エンコードの変更を反映させる
    id3v2.SetYear(szYear);//"TYER" <=> "TDRC" 変換
    bool result = id3v2.Save(GetFullPath(pFileInfo)) == ERROR_SUCCESS ? true : false;
    if (result) {
        extern bool LoadFileDSF(FILE_INFO *pFileMP3);
        if (LoadFileDSF(pFileInfo) == false) {
            return false;
        }
    }
    return result;
}
