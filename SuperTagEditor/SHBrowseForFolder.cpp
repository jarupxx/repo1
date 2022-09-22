// ======================================================
// 概  要    ： フォルダ選択ダイアログのラッパクラス
// ターゲット： Windows95/98/NT
// 処理系    ： Visual C++ Ver 6.0
// 作成者    ： MERCURY
// 作成日    ： 00/06/17(土)
// 著作権表記： Copyright(C) 2000 MERCURY.
// ======================================================



// ======================================
// =====   条件コンパイルフラグ     =====
// ======================================



// ======================================
// =====   インクルードファイル     =====
// ======================================
#include "stdafx.h"
#include "SHBrowseForFolder.h"
#include <shlobj.h>


// ======================================
// =====           定  数           =====
// ======================================
#define IDC_CH_SUB_DIR      1000        // フォルダ参照の[サブディレクトリを検索]ボタンのID



// ======================================
// =====           マクロ           =====
// ======================================



// ======================================
// =====       typedef／enum        =====
// ======================================



// ======================================
// =====       構造体／共用体       =====
// ======================================



// ======================================
// =====         const 変数         =====
// ======================================



// ======================================
// =====        extern 変数         =====
// ======================================
//bool    CSHBrowseForFolder::m_bEnableSubDirButton = false;
//bool    CSHBrowseForFolder::m_bSearchSubDirState = false;
//const TCHAR* CSHBrowseForFolder::pCheckBoxTitle = NULL;/* FunnyCorn 185 *///"サブディレクトリを検索";
//WNDPROC CSHBrowseForFolder::m_VSSelectOrgProc;



// ======================================
// =====        static 変数         =====
// ======================================


// ======================================
// =====     関数プロトタイプ       =====
// ======================================



// ======================================
// =====      プログラム領域        =====
// ======================================
// =============================================
// CSHBrowseForFolder::CSHBrowseForFolder
// Func  : コンストラクタ
// Input : bEnable      = [サブディレクトリを検索]ボタンを追加するか?
//       : bState       = [サブディレクトリを検索]ボタンの初期状態
// Output: none
// =============================================
CSHBrowseForFolder::CSHBrowseForFolder(bool bEnable, bool bState)
{
    m_pSubDir = NULL;
    m_szInitialFolder[0] = 0;
    _tcsncpy_s(m_szCheckBoxTitle, _T("サブディレクトリを検索"), _TRUNCATE);
    m_VSSelectOrgProc = NULL;
    m_bEnableSubDirButton = bEnable;
    m_bSearchSubDirState = bState;
}

// =============================================
// CSHBrowseForFolder::~CSHBrowseForFolder
// Func  : デストラクタ
// Input : none
// Output: none
// =============================================
CSHBrowseForFolder::~CSHBrowseForFolder()
{
    delete m_pSubDir;
}

// =============================================
// VSSelectFolderSubProc
// 概要  : フォルダ参照の[サブディレクトリを検索]ボタン処理
// 引数  : ...
// 戻り値: int CALLBACK
// =============================================
LRESULT CALLBACK CSHBrowseForFolder::VSSelectFolderSubProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSHBrowseForFolder *pThis = (CSHBrowseForFolder*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if(pThis){
        return pThis->VSSelectFolderSubProcInternal(hWnd, uMsg, wParam, lParam);
    }
    else{
        return 0;
    }
}
LRESULT CSHBrowseForFolder::VSSelectFolderSubProcInternal(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ((uMsg == WM_COMMAND) && (LOWORD(wParam) == IDC_CH_SUB_DIR)) {
        // ボタンの処理
        CWnd    *pClient = CWnd::FromHandle(hWnd);
        CButton *pSubDir = (CButton *)pClient->GetDlgItem(IDC_CH_SUB_DIR);

        // チェック状態を反転
        SetSearchSubDirState(GetSearchSubDirState() ? false : true);
        pSubDir->SetCheck(GetSearchSubDirState() ? TRUE : FALSE);
    }
    return(CallWindowProc(m_VSSelectOrgProc, hWnd, uMsg, wParam, lParam));
}


// =============================================
// CallbackSelectDir
// 概要  : フォルダ参照で使用するコールバック関数
//       : 開いたときにフォルダを選択した状態で開かせる処理をする場合に必要
// 引数  : ...
// 戻り値: int CALLBACK
// =============================================
int CALLBACK CSHBrowseForFolder::CallbackSelectDir(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    return ((CSHBrowseForFolder*)lpData)->CallbackSelectDirInternal(hWnd, uMsg, lParam);
}
int CSHBrowseForFolder::CallbackSelectDirInternal(HWND hWnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == BFFM_INITIALIZED) {
        // デフォルトのフォルダを選択させる
        SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, (LPARAM)m_szInitialFolder);
        {
            // フォルダーツリーで常に選択状態を表示
            HWND hwndTree = FindWindowEx(hWnd, NULL, _T("SysTreeView32"), NULL);
            if (hwndTree != NULL) {
                LONG style;
                style = GetWindowLong(hwndTree, GWL_STYLE);
                style |= TVS_SHOWSELALWAYS;
                SetWindowLong(hwndTree, GWL_STYLE, style);
            }
        }

        // [サブディレクトリを検索] ボタンを追加
        if (GetEnableSubDirButton()) {
            m_pSubDir = new CButton; /* WildCherry2 080 */
            if (m_pSubDir) {
                RECT    rectClient, rectOK, rectCancel, rect;
                CWnd    *pClient    = CWnd::FromHandle(hWnd);
                CWnd    *pOK        = pClient->GetDlgItem(IDOK);
                CWnd    *pCANCEL    = pClient->GetDlgItem(IDCANCEL);
                // クライアントウィンドウサイズを取得
                pClient->GetClientRect(&rectClient);
                // [OK] / [CANCEL] のウィンドウ位置を取得
                pOK->GetWindowRect(&rectOK);
                pCANCEL->GetWindowRect(&rectCancel);
                pClient->ScreenToClient(&rectOK) ;
                pClient->ScreenToClient(&rectCancel) ;
                // サブディレクトリ対象チェックボタンの RECT を求める
                rect = CRect(rectClient.right - rectCancel.right,
                             rectCancel.top,
                             rectOK.left,
                             rectCancel.bottom);
                // チェックボタン作成
                if (m_pSubDir->Create(GetCheckBoxTitle(),
                                    WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP,
                                    rect, pClient, IDC_CH_SUB_DIR)) {
                    // フォントを設定
                    HFONT hFontCurr = (HFONT)pClient->SendMessage(WM_GETFONT, 0, 0);
                    if (hFontCurr != NULL) m_pSubDir->SendMessage(WM_SETFONT, (WPARAM)hFontCurr, 0);

                    // チェック状態を設定
                    m_pSubDir->SetCheck(GetSearchSubDirState() ? TRUE : FALSE);

                    // サブクラス化
                    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
                    m_VSSelectOrgProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)VSSelectFolderSubProc);
                }
            }
        }
    }
    return 0;
}

// =============================================
// CSHBrowseForFolder::SelectDirectory
// 概要  : フォルダ参照ダイアログ処理
// 引数  : sLocal           = パス(入出力)
// 戻り値: bool
// =============================================
bool CSHBrowseForFolder::Exec(TCHAR *sLocal, int size)
{
    BOOL            bResult = FALSE;
    BROWSEINFO      bi;
    LPTSTR          lpBuffer;
    LPITEMIDLIST    pidlRoot;      // ブラウズのルートPIDL
    LPITEMIDLIST    pidlBrowse;    // ユーザーが選択したPIDL
    LPMALLOC        lpMalloc = NULL;

    HRESULT hr = SHGetMalloc(&lpMalloc);
    if (FAILED(hr)) return(FALSE);

    HWND    hwnd = AfxGetMainWnd()->GetSafeHwnd();

    // ブラウズ情報受け取りバッファ領域の確保
    if ((lpBuffer = (LPTSTR) lpMalloc->Alloc(_MAX_PATH*sizeof(TCHAR))) == NULL) {
        lpMalloc->Release(); /* WildCherry2 080 */
        return(FALSE);
    }
    // ダイアログ表示時のルートフォルダのPIDLを取得
    // ※以下はデスクトップをルートとしている。デスクトップをルートとする
    //   場合は、単に bi.pidlRoot に０を設定するだけでもよい。その他の特
    //   殊フォルダをルートとする事もできる。詳細はSHGetSpecialFolderLoca
    //   tionのヘルプを参照の事。
    if (!SUCCEEDED(SHGetSpecialFolderLocation(  hwnd,
                                                CSIDL_DESKTOP,
                                                &pidlRoot))) {
        lpMalloc->Free(lpBuffer);
        lpMalloc->Release(); /* WildCherry2 080 */
        return(FALSE);
    }
    // BROWSEINFO構造体の初期値設定
    // ※BROWSEINFO構造体の各メンバの詳細説明もヘルプを参照
    bi.hwndOwner = hwnd;
    bi.pidlRoot = pidlRoot;
    bi.pszDisplayName = lpBuffer;
    bi.lpszTitle = _T("フォルダを選択して下さい。");
    bi.ulFlags = BIF_EDITBOX;
    bi.lpfn = CallbackSelectDir;        // コールバック関数のアドレスを設定
    bi.lParam = (LPARAM)this;
    _tcsncpy_s(m_szInitialFolder, sLocal, _TRUNCATE);
    // フォルダ選択ダイアログの表示
    pidlBrowse = SHBrowseForFolder(&bi);
    if (pidlBrowse != NULL) {
        // PIDL形式の戻り値のファイルシステムのパスに変換
        if (SHGetPathFromIDList(pidlBrowse, lpBuffer)) {
            // 取得成功
            _tcsncpy_s(sLocal, size, lpBuffer, _TRUNCATE);
            bResult = TRUE;
        }
        // SHBrowseForFolderの戻り値PIDLを解放
        lpMalloc->Free(pidlBrowse);
    }
    // クリーンアップ処理
    lpMalloc->Free(pidlRoot);
    lpMalloc->Free(lpBuffer);
    lpMalloc->Release();
    delete m_pSubDir;
    m_pSubDir = NULL;
    //if (pSubDir) { /* WildCherry2 080 */
    //  delete pSubDir;
    //  pSubDir = NULL;
    //}
    return(bResult ? true : false);
}
