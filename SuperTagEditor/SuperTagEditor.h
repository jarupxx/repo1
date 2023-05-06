// SuperTagEditor.h : DMTLOGVIEW アプリケーションのメイン ヘッダー ファイル
//

#if !defined(AFX_DMTLOGVIEW_H__D67B6444_BFAB_11D3_9459_00402641B29B__INCLUDED_)
#define AFX_DMTLOGVIEW_H__D67B6444_BFAB_11D3_9459_00402641B29B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // メイン シンボル

#define PROG_NAME       _T("STEP_J")     // プログラム名
#define PROG_VERSION    _T("1.04")  // バージョン
#define PROG_COPYRIGHT  _T("Copyright (C) 2022 jarupxx")
#define PROG_URL        _T("https://github.com/jarupxx/STEP_J/")
// #define PROG_MAIL       _T("example@co.jp")
#ifdef _WIN64
#define PROG_PLATFORM   _T("x64")
#else
#define PROG_PLATFORM   _T("x86")
#endif

#define STEP_MUTEX_NAME       _T("STEP_J")    //起動済み Mutex オブジェクト名
#define STEP_DDE_SERVICE_NAME _T("STEP_J")    //STEP DDE サービス名
#define STEP_DDE_TOPIC_NAME   _T("STEP_J")    //STEP DDE トピック名

#define PROG_NAME_ORG       _T("SuperTagEditor")    // プログラム名
#define PROG_VERSION_ORG    _T("2.00β")            // バージョン
#define PROG_NAME_ORG2      _T("STEP")              // プログラム名
#define PROG_VERSION_ORG2   _T("1.02")              // バージョン
#define PROG_NAME_ORG3      _T("STEP_M")
#define PROG_VERSION_ORG3   _T("1.054f")
#define PROG_NAME_ORG4      _T("STEP_K")
#define PROG_VERSION_ORG4   _T("1.05a")

#define VCRUNTIME_URL _T("https://visualstudio.microsoft.com/ja/downloads/#other-ja-family") //VS2022 再頒布可能パッケージURL
#define WM_USER_SET_STATUS_POS      WM_USER+1
#define WM_USER_SET_STATUS_SIZE     WM_USER+2
#define WM_USER_SET_STATUS_INPUT_MODE   WM_USER+3 /* Misirlou 150 */

//#ifndef iskanji
//#define iskanji(c)      ((BYTE)(c) >= (BYTE)0x81 && (BYTE)(c) <= (BYTE)0x9f || (BYTE)(c) >= (BYTE)0xe0 && (BYTE)(c) <= (BYTE)0xfc)
//#endif

#ifndef shortString /* FreeFall 051 */
#define shortString(str, count) str.GetLength() > count ? (str.Left(count) + "...") : str
#endif
#define TEIKEI_MENU_STRING_MAX  30  /* FreeFall 051 */

#ifndef shortFavorite   /* RockDance 129 */
#define shortFavorite(str)  str.GetLength() > 30 ? (str.Left(3) + "..." + str.Right(27)) : str
#endif

#include "INI/ini.h"
#include "regexp.h"
#include "MyRecentFileList.h"   // ClassView によって追加されました。

//フォルダの同期処理を無効にする(by Kobarin)
//機能の存在意義が分からず、不具合があった場合に致命的になるため STEP_K では無効にします。
//開発の後を引き継ぐ人でこの機能を有効にしたい場合はコメントアウトして下さい。
//同期処理の動作確認は不十分なので、有効にする場合は注意して下さい。
#define DISABLE_FOLDER_SYNC

enum    {
    FILENAME_NO_CHECK,              // チェックしない
    FILENAME_CONV_MULTIBYTE,        // ２バイト文字に変換
};

#define SORT_KEY_MAX        5
struct  SORT_STATE  {               // ソート情報
    int     nKeyColumn;             // ソートキー(カラム番号)
    int     nType;                  // タイプ(0:昇順/1:降順)
};

#define CLASS_MAX           5       // 分類階層の深さ(最大)
struct  CLASS_INFO  {               // 分類情報
    int     nType;                  // 分類タイプ
    int     nColumn[CLASS_MAX];     // カラム番号
};

enum    {
    COMMAND_GROUP_FILE,
    COMMAND_GROUP_EDIT,
    COMMAND_GROUP_DISP,
    COMMAND_GROUP_CONV, /* 2006.03.03 */
    COMMAND_GROUP_PLAYLIST,
    COMMAND_GROUP_PLAYER,
    COMMAND_GROUP_PLUGIN,
    COMMAND_GROUP_MAX,
};
struct  KEY_CONFIG  {               // キー割り当て
    WORD    wCmdID;                 // メニューリソースＩＤ
    DWORD   dwKeyCode;              // ホットキー
    int     nGroupID;               // グループ名
    const TCHAR *sName;             // 項目名
    const TCHAR *sRegName;          // レジストリ名
};
extern  KEY_CONFIG  g_listKeyConfig[];

#define FILENAME_REPLACE_MAX        12
#define USER_FILENAME_REPLACE_MAX    6   /* FreeFall 050 */
struct  FILENAME_REPLACE {          // ファイル名置換
    CString strBefore;              // 置換前の文字
    CString strAfter;               // 置換後の文字
};
extern  const TCHAR *g_sRepTable[FILENAME_REPLACE_MAX][2];

#define USER_CONV_FORMAT_MAX        5 /*3 LastTrain 057 */
struct  USER_CONV_FORMAT    {       // ユーザー変換書式
    CString strName;                // 名称
    CString strTag2File;            // 書式：タグ情報 => ファイル名
    CString strFile2Tag;            // 書式：ファイル名 => タグ情報
};

#define USER_CONV_FORMAT_EX_MAX     10
struct  USER_CONV_FORMAT_EX {       // 拡張版ユーザー変換書式
    CString strName;                // 名称
    CString strFormat;              // 書式
    CString strFixString;           // 固定文字列
    int     nInitNumber;            // 連番：初期値
    int     nAddNumber;             // 連番：加算値
    int     nColumnCount;           // 連番：桁数
    bool    bSpaceInitNumber;       // 書き込み不可のセルで連番クリア
};

#define USER_MOVE_FODLER_FORMAT_MAX 5
struct  USER_MOVE_FODLER_FORMAT {   // 移動先フォルダ書式
    CString strName;                // 名称
    CString strFormat;              // 書式
    CString strFixString;           // 固定文字列
    bool    bCopy;                  // コピーする
    CString strInitFolder;          // 初期フォルダ /* STEP 022 */
    CString strCurrentMoveDirectory;// 選択された移動先フォルダ /* STE 022 */
};

/* STEP 034 */
#define USER_CONV_FORMAT_TAG2TAG_MAX    5
struct USER_CONV_FORMAT_TAG2TAG {   // タグ振分変換書式
    CString strName;                // 名称
    CString strFormat;              // 書式
};

/* FunnyCorn 175 */
#define USER_COPY_FORMAT_FORMAT_MAX 5
struct  USER_COPY_FORMAT_FORMAT {   // 書式コピー
    CString strName;                // 名称
    CString strFormat;              // 書式
    CString strFixString;           // 固定文字列
};

/* STEP 035 */
#define TEIKEI_INFO_MAX 30
struct TEIKEI_INFO {                // 定型文
    CString strTeikei;              // 定型文
    int     nTeikeiPaste;           // 貼り付け位置
    BOOL    bAddSpace;              // スペース追加
    BOOL    bAddChar;               // 前後に文字を追加
    CString strFront;               // 前付加文字
    CString strBack;                // 後付加文字
    BOOL    bShowDialog;            // 貼り付け時ダイアログ表示
};

#define WRITE_FORMAT_MAX            5
struct  WRITE_FORMAT    {           // 拡張版ユーザー変換書式
    CString strName;                // 名称
    CString strFileName;            // 書式ファイル
    CString strExtName;             // 拡張子
    bool    bWriteSelected;         // 選択ファイルのみ上書き
    CString strCurrentFile;         // カレントファイル名
    bool    bIsHtml;                // HTML ファイル出力用(空の場合に全角スペースを出力)
    bool    bWriteHtml;             // HTML ファイルとして出力（タグを置換する）/* BeachMonster5 120 */
};

struct  CHECK_WORD_STATE    {       // 条件チェック用の情報
    CString strSearchWord;          // 検索文字列
    CString strReplaceWord;         // 置換文字列
    int     nTargetColumn;          // 検索対象カラム
    BOOL    bCheckDiffUL;           // 大文字／小文字の区別
    BOOL    bRegExp;                // 正規表現
    BOOL    bRangeSelected;         // 選択範囲のみ
    BOOL    bMatchComplete;         // 完全に一致
    BOOL    bMatchSelected;         // チェックせずに選択状態にする
    int     nMatchColumn;           // (出力)検索結果：一致したカラム
    int     nMatchPos;              // (出力)検索結果：一致した位置
    int     nMatchLength;           // (出力)検索結果：一致した長さ
    CRegExp regexp;                 // 正規表現クラス(内部処理用)
};
enum    {CHECK_STATE_CHECK, CHECK_STATE_REPLACE, CHECK_STATE_MAX};
extern  CHECK_WORD_STATE    g_chkWord[CHECK_STATE_MAX];

enum    {PLAYER_WINAMP, PLAYER_SCMPX, PLAYER_KBMPLAY, PLAYER_FOOBAR, PLAYER_LILITH, PLAYER_ULILITH, PLAYER_EXTEND};

extern  RECT        g_rectMainWindow;       // メインウィンドウの座標
extern  BOOL        g_bMainFrameZoomed;
extern  BOOL        g_bMainFrameIconic;
extern  CString     g_strCurrentDirectory;
extern  CString     g_strCurrentMoveDirectory;
extern  CString     g_strCurrentPlayList;
extern  bool        g_bIsVersionUp;
extern  int         g_nUserConvFormatType;
extern  USER_CONV_FORMAT    g_userConvFormat[USER_CONV_FORMAT_MAX];
extern  USER_CONV_FORMAT_EX g_userConvFormatEx[USER_CONV_FORMAT_EX_MAX];
extern  USER_MOVE_FODLER_FORMAT g_userMoveFolder[USER_MOVE_FODLER_FORMAT_MAX];
extern  USER_COPY_FORMAT_FORMAT g_userCopyFormat[USER_COPY_FORMAT_FORMAT_MAX]; /* FunnyCorn 175 */
extern  USER_CONV_FORMAT_TAG2TAG g_userConvFormatTag2Tag[USER_CONV_FORMAT_TAG2TAG_MAX]; /* STEP 034 */
extern  TEIKEI_INFO g_teikeiInfo[TEIKEI_INFO_MAX]; /* STEP 035 */
extern  WRITE_FORMAT        g_writeFormat[WRITE_FORMAT_MAX];
extern  bool        g_bOptESCEditCancel;
extern  bool        g_bOptEnableEditCursorExit;
extern  bool        g_bOptEnterBeginEdit;
extern  bool        g_bOptEditOkDown;
extern  bool        g_bOptKeepTimeStamp;
extern  bool        g_bOptSyncCreateTime;
extern  bool        g_bOptChangeFileExt;
extern  bool        g_bOptChangeTextFile;
extern  int         g_nOptCheckFileName;
extern  bool        g_bOptLoadFileChecked;
extern  bool        g_bOptHideMP3ListFile;
extern  bool        g_bOptDropSearchSubFolder; /* TyphoonSwell 026 */
extern  bool        g_bOptShowZenSpace; /* BeachMonster 107 */
extern  CString     g_sOptShowOtherChar; /* BeachMonster 107 */ // とりあえずいれとく
extern  bool        g_bOptSortIgnoreCase; /* BeachMonster4 114 */
extern  bool        g_bOptSortIgnoreZenHan; /* BeachMonster4 114 */
extern  bool        g_bOptSortIgnoreKataHira; /* BeachMonster4 114 */
extern  bool        g_bOptShowTotalParent; /* RockDance 128 */
extern  bool        g_bOptShowTips; /* Rumble 188 */
extern  int         g_nOptPlayerType;
extern  CString     g_sOptWinAmpPath;
extern  bool        g_bOptEditFieldSIF;
extern  bool        g_bOptAutoOpenFolder;
extern  bool        g_bOptLoadFileAdjustColumn;
extern  bool        g_bOptSearchLyricFile;
extern  bool        g_bOptSetLyricsDir;
extern  bool        g_bOptSearchLyricsSubDir;
extern  bool        g_bEnableSearchSubDir;
extern  bool        g_bEnableMoveFolderCopy;
extern  CString     g_strOptLyricsPath;
extern  SORT_STATE  g_sortState[SORT_KEY_MAX];
extern  CLASS_INFO  g_classInfo;

//extern    bool        g_bOptID3v2GenreAddNumber;
extern  bool        g_bConfFileNameMaxCheck;
extern  int         g_nConfFileNameMaxChar;
extern  bool        g_bFileNameMaxCellColor; /* SeaKnows 036 */

extern  bool        g_bEnableFolderSync;
extern  CString     g_strRootFolder;
extern  bool        g_bSyncSelectAlways;
extern  bool        g_bSyncDeleteFolder;
extern  bool        g_bSyncLyricsFileMove;

extern  bool        g_bConfDeleteFile;
extern  bool        g_bConfDeleteList;
extern  bool        g_bConfEditModify;
extern  bool        g_bConfFolderSync;

extern  bool        g_bPlayListClearList;
extern  bool        g_bPlayListClearCheck;
extern  bool        g_bPlayListAddList;
extern  bool        g_bPlayListFileCheck;

/* STEP 035 *///extern  CString     g_strTeikei[10+20]; /* SeaKnows 030 *//* FreeFall 046 */
extern  CString     g_strTeikeiGroupName[3];    /* FreeFall 046 */
extern  bool        g_bValidFolderSelect;   /* SeaKnows 033 */
extern  bool        g_bValidDupExec;    /* FreeFall 045 */

extern  int         g_nRecentFolder; /* StartInaction 053 */
extern  bool        g_bSaveRepDlgPos; /* WildCherry4 086 */
extern  int         g_nSaveRepDlgPosX; /* WildCherry4 086 */
extern  int         g_nSaveRepDlgPosY; /* WildCherry4 086 */

// 文字種統一 /* StartInaction 054 */
extern  UINT        g_nUnifyAlpha;
extern  UINT        g_nUnifyAlpha;
extern  UINT        g_nUnifyHiraKata;
extern  UINT        g_nUnifyKata;
extern  UINT        g_nUnifyKigou;
extern  UINT        g_nUnifySuji;
extern  UINT        g_nUnifyUpLow;
extern  UINT        g_nUnifyFixedUpLow; /* STEP 040 */

// ファイル名文字種統一 /* LastTrain 058 */
extern  UINT        g_nFileUnifyAlpha;
extern  UINT        g_nFileUnifyHiraKata;
extern  UINT        g_nFileUnifyKata;
extern  UINT        g_nFileUnifyKigou;
extern  UINT        g_nFileUnifySuji;
extern  UINT        g_nFileUnifyUpLow;

// 拡張子変換 /* STEP 006 */
extern  UINT        g_nFileExtChange;

extern  bool        g_bEndEditMoveRightCell; /* BeachMonster 091 */

extern  bool        g_bShowLoadPlaylistDlg; /* RockDance 126 */

extern  CString     g_strFavorite[10];  /* RockDance 129 */

extern  int         g_nAddNumberWidth; /* Baja 159 */
extern  int         g_nAddNumberPos; /* Baja 159 */
extern  CString     g_strAddNumberSep; /* Baja 159 */
extern  CString     g_strAddNumberBef; /* Conspiracy 194 */
extern  CString     g_strAddNumberAft; /* Conspiracy 194 */

extern  bool        g_bAudioListShow; /* Conspiracy 199 */

extern bool         g_bFirstUpperIgnoreWord; /* STEP 026 */
extern CString      g_strFirstUpperIgnoreWords; /* STEP 026 */
extern CString      g_strFirstUpperSentenceSeparator; /* STEP 026 */
extern bool         g_bUserConvAddMenu; /* STEP 030 */
extern bool         g_bZenHanKigouKana; /* STEP 016 */

extern bool         g_bAutoTilde2WaveDash;//STEP_K(全角チルダを波ダッシュに自動置換)

extern  LOGFONT     g_fontReport;           // フォント

extern  FILENAME_REPLACE    g_fileNameReplace[FILENAME_REPLACE_MAX];

extern  FILENAME_REPLACE    g_userFileNameReplace[USER_FILENAME_REPLACE_MAX];   /* FreeFall 050 */


extern  KEY_CONFIG *SearchKeyConfigID(WORD);

extern  CStringArray g_arFixedWords; /* STEP 040 */

/////////////////////////////////////////////////////////////////////////////
// CSuperTagEditorApp:
// このクラスの動作の定義に関しては SuperTagEditor.cpp ファイルを参照してください。
//

class CSuperTagEditorApp : public CWinApp
{
public:
    void WriteFixedWordList();
    CSuperTagEditorApp();
    ~CSuperTagEditorApp();

protected:
    BOOL OnOpenRecentFile(UINT nID); /* BeachMonster 104 */
    HANDLE  m_hMutex;           // 多重起動禁止処理用のMutex
    HACCEL  m_hAccel;           // アクセラレータテーブルのハンドル
    ACCEL   *m_accelTable;      // アクセラレータテーブル
    int     m_nAccelTable;      // アクセラレータテーブル数 /* STEP 030 */
    CIniFile m_IniFile;
public:
    CIniFile *GetIniFile(void){return &m_IniFile;}
    void AddToRecentFileList(LPCTSTR lpszPathName);
    inline  HACCEL  GetAccelHandle(void) {return(m_hAccel);}
    TCHAR   *MakeFileName(TCHAR *);
    void    ReadWindowStatus(const TCHAR *, RECT *);
    void    WriteWindowStatus(const TCHAR *, RECT *);
    void    ReadRegistry(void);
    void    WriteRegistry(void);
    void    ReadKeyConfig(bool = false);
    void    WriteKeyConfig(bool = false);
    void    ReadGenreList(void);
    void    WriteGenreList(void);
    void    UpdateAccelerator(int = -1);
    void    DestroyAccelerator(void);
    void    GetWinampPath(void);
    void    ReadFixedWordList(void);

// オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CSuperTagEditorApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// インプリメンテーション
    //{{AFX_MSG(CSuperTagEditorApp)
    afx_msg void OnAppAbout();
    afx_msg void OnUpdateFileMruFile(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder01();
    afx_msg void OnUpdateFavoriteFolder01(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder02();
    afx_msg void OnUpdateFavoriteFolder02(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder03();
    afx_msg void OnUpdateFavoriteFolder03(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder04();
    afx_msg void OnUpdateFavoriteFolder04(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder05();
    afx_msg void OnUpdateFavoriteFolder05(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder06();
    afx_msg void OnUpdateFavoriteFolder06(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder07();
    afx_msg void OnUpdateFavoriteFolder07(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder08();
    afx_msg void OnUpdateFavoriteFolder08(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder09();
    afx_msg void OnUpdateFavoriteFolder09(CCmdUI* pCmdUI);
    afx_msg void OnFavoriteFolder10();
    afx_msg void OnUpdateFavoriteFolder10(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    void OnFavoriteFolder(CString& strFolder);
    void LoadStdProfileSettings(UINT nMaxMRU);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DMTLOGVIEW_H__D67B6444_BFAB_11D3_9459_00402641B29B__INCLUDED_)
