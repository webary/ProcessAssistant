
// ProcessAssistantDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ProcessAssistant.h"
#include "ProcessAssistantDlg.h"
#include <afxdialogex.h>

#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")

#include <fstream>
#include <string>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CProcessAssistantDlg 对话框

CProcessAssistantDlg::CProcessAssistantDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CProcessAssistantDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_listCnt = 0;

    char path[MAX_PATH] = "";
    GetTempPath(MAX_PATH, path);
    CString myPath = path + CString("ProcessAssistant\\");
    CreateDirectory(myPath, 0); //在临时文件夹中创建本应用的文件夹

    m_autoRunListFile = myPath + "autorun.dat";
    m_checkedListFile = myPath + "checkedList.dat";

    pMapRunning = NULL;
}

CProcessAssistantDlg::~CProcessAssistantDlg()
{
    Shell_NotifyIcon(NIM_DELETE, &m_nid);
    ReleaseMutex(m_hmutex);
}

void CProcessAssistantDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS, m_wndList);
}


#define WM_NOTIFYICONMSG WM_USER + 1 //托盘消息
#define ID_HK_OPEN_MAIN_DLG 0 //全局快捷键-打开主窗口的ID

BEGIN_MESSAGE_MAP(CProcessAssistantDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_WM_HOTKEY()
    ON_MESSAGE(WM_NOTIFYICONMSG, OnNotifyIconMsg)
    ON_BN_CLICKED(IDOK, &CProcessAssistantDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BT_SET, &CProcessAssistantDlg::OnBnClickedBtSet)
    ON_COMMAND(ID_OPEN_MAIN_DLG, &CProcessAssistantDlg::OnOpenMainDlg)
    ON_COMMAND(ID_EXIT_ME, &CProcessAssistantDlg::OnExitMe)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_PROCESS, &CProcessAssistantDlg::OnDblclkListProcess)
    ON_NOTIFY(NM_RCLICK, IDC_LIST_PROCESS, &CProcessAssistantDlg::OnNM_RClickListProcess)
    ON_COMMAND(ID_OPEN_DIR, &CProcessAssistantDlg::OnRClick_OpenDir)
    ON_COMMAND(ID_START_PROCESS, &CProcessAssistantDlg::OnRClick_StartProcess)
    ON_COMMAND(ID_END_PROCESS, &CProcessAssistantDlg::OnRClick_EndProcess)
    ON_COMMAND(ID_CHECK_OR_NOT_PROCESS, &CProcessAssistantDlg::OnRClick_CheckOrNotProcess)
END_MESSAGE_MAP()

///利用共享内存方式实现进程通信。共享内存实际就是文件映射的一种特殊情况

//创建文件映射，相当于创建通信信道.创建成功返回0
int createMyFileMap(void* &lp, size_t size, const char* str)
{
    HANDLE h = CreateFileMapping((HANDLE)0xFFFFFFFF, 0, PAGE_READWRITE, 0, size, str);
    if (h == NULL)
    {
        MessageBox(0, "Create File Mapping Faild", str, 0);
        return -1;
    }
    lp = MapViewOfFile(h, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, size);
    if (lp == NULL)
    {
        MessageBox(0, "View MapFile Faild-c", str, 0);
        return -2;
    }
    return 0;
}

//打开文件映射，相当于访问信道。打开成功访问0
int openMyFileMap(void* &lp, size_t size, const char* str, bool showBox = 1)
{
    HANDLE h = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, 0, str);
    if (h == NULL)
    {
        if (showBox) MessageBox(0, "Open File Mapping Faild", str, 0);
        return -1;
    }
    lp = MapViewOfFile(h, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size);
    if (lp == NULL)
    {
        if (showBox) MessageBox(0, "View MapFile Faild-o", str, 0);
        return -2;
    }
    return 0;
}

//设置文件映射，相当于向信道传送消息
template<typename T_Set>
int setMyFileMap(void* &lp, size_t size, const T_Set* pValue)
{
    if (lp == 0 || pValue == 0 || size == 0)
        return -1;
    memcpy(lp, pValue, size);
    return 0;
}

//更新进程列表线程的函数
void updateListThread(void * _pDlg)
{
    CProcessAssistantDlg* pDlg = static_cast<CProcessAssistantDlg*>(_pDlg);
    while (pDlg){
        Sleep(3000);
        pDlg->updateProcessList();
    }
}

// CProcessAssistantDlg 消息处理程序

BOOL CProcessAssistantDlg::OnInitDialog()
{
    //利用互斥锁机制保证最多只有一个该实例正在运行
    m_hmutex = CreateMutex(NULL, true, "ProcessAssistant");
    if (ERROR_ALREADY_EXISTS == GetLastError()) { //若互斥锁已存在则直接关闭
        if (0 == openMyFileMap(pMapRunning, 4, "NewInstance", 0)){
            int val = 1;
            setMyFileMap(pMapRunning, 4, &val);
        }
        SetTimer(2, 3000, NULL); //3秒后自动关闭
        MessageBox("请不要重复打开该程序哦，我的前身还在通知栏运行着呢^ _ ^",
                   0, MB_ICONWARNING);
        OnCancel();
        return FALSE;
    }
    createMyFileMap(pMapRunning, 4, "NewInstance"); //创建共享内存

    CDialog::OnInitDialog();
    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    //整行选定 + 显示表格线 + 复选框 + 扁平滚动条
    m_wndList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_FLATSB);
    //设置表头
    m_wndList.InsertColumn(0, "(开启)   进程名", 0, 124);
    m_wndList.InsertColumn(1, "文件位置", 0, 415);
    m_wndList.InsertColumn(2, "备注", 0, 50);
    //设置文本显示颜色
    m_wndList.SetTextColor(RGB(0, 0, 0));
    //设置图标列表
    m_iconList.Create(32, 32, ILC_COLOR32, 0, 100);
    m_wndList.SetImageList(&m_iconList, LVSIL_SMALL);

    SetTimer(0, 100, NULL); //显示进程列表,打开上次未关闭的进程,并添加启动项
    SetTimer(1, 500, NULL);

    _beginthread(updateListThread, 0, this); //开启后台线程定期监测相应进程是否在运行

    //设置托盘消息 - 必须在这里赋值,如果在构造函数赋值,鼠标指向托盘图标后即消失
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hWnd = m_hWnd;
    m_nid.uFlags = NIF_INFO | NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.dwInfoFlags = NIIF_INFO;
    m_nid.hIcon = m_hIcon;
    m_nid.uCallbackMessage = WM_NOTIFYICONMSG;
    strcpy(m_nid.szTip, "进程助手");
    strcpy(m_nid.szInfoTitle, "进程助手已驻扎在通知栏");//气泡标题
    strcpy(m_nid.szInfo, "我将在这里检查主人设置的程序是否在运行，如果您关机的"
           "时候没有关闭他们，我将在下次开机时为主人打开他们哦，您可以点击"
           "主窗口的关闭，但我还会一直在这里运行着");//气泡内容
    Shell_NotifyIcon(NIM_ADD, &m_nid);

    //设置全局快捷键: Ctrl+Shift+D 打开主窗口
    RegisterHotKey(GetSafeHwnd(), ID_HK_OPEN_MAIN_DLG, MOD_CONTROL | MOD_SHIFT, 'D');

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

BOOL CProcessAssistantDlg::PreTranslateMessage(MSG* pMsg)
{
    static HACCEL hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));
    if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
    {
        if (hAccel && TranslateAccelerator(m_hWnd, hAccel, pMsg))
            return TRUE;
    }
    return CDialog::PreTranslateMessage(pMsg);
}

HCURSOR CProcessAssistantDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CProcessAssistantDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

void CProcessAssistantDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
        case 0: //在本程序启动时执行一次,检查当前任务管理器中的进程,并添加启动项
        {
            KillTimer(0);
            showProcessList();
            //该电脑上第一次运行该程序时默认设置为自启动
            bool firstRun = false;
            ifstream fin(m_checkedListFile);
            if (!fin.is_open())
                firstRun = true;
            fin.close();
            if (firstRun){
                m_setDlg.setStartup(1);
                //保证该文件在之后一直存在
                ofstream fout(m_checkedListFile, ios::out | ios::app);
                fout.close();
                MessageBox("勾选对应程序左边的选框即可开启开机自启动项，关机时"
                           "这些进程如果未关闭，下次开机后运行此程序将自动运行"
                           "对应进程！", "开启提示", MB_ICONINFORMATION);
            }
            openUnclosedProcess(); //打开上次关机时未关闭的进程
            break;
        }
        case 1:
        {
            if (pMapRunning && *(int*)pMapRunning == 1){
                int val = 0;
                setMyFileMap(pMapRunning, 4, &val);
                strcpy(m_nid.szInfoTitle, "温馨提示");//气泡标题
                strcpy(m_nid.szInfo, "主人，我还在这里运行着呢，快来点我吧");
                Shell_NotifyIcon(NIM_MODIFY, &m_nid);
            }
            break;
        }
        case 2:
        {
            exit(-1);
            break;
        }
        default:
            break;
    }
    CDialog::OnTimer(nIDEvent);
}

void CProcessAssistantDlg::OnBnClickedOk()
{
    m_checkedList.clear();
    ofstream fout(m_checkedListFile);
    for (int i = 0; i < m_wndList.GetItemCount(); ++i)
        if (m_wndList.GetCheck(i)){
            fout << m_wndList.GetItemText(i, 1) << endl;
            m_checkedList.insert(m_wndList.GetItemText(i, 1));
        }
    fout.close();

    CString selected;
    for (auto& elem : m_checkedList)
        selected += elem + "\n";
    if (selected.IsEmpty()){
        MessageBox("已取消所有开机启动项", "关闭提示", MB_ICONINFORMATION);
    }
    else{
        MessageBox("已开启下列开机启动项: \r\n" + selected + "关机时上述进程"
                   "如果未关闭，下次开机后运行此程序将自动打开上述进程！",
                   "开启提示", MB_ICONINFORMATION);
    }
}

void CProcessAssistantDlg::OnBnClickedBtSet()
{
    m_setDlg.DoModal();
}

void CProcessAssistantDlg::OnOpenMainDlg()
{
    ShowWindow(SW_SHOW);
    SetFocus();
}

void CProcessAssistantDlg::OnExitMe()
{
    OnOpenMainDlg();
    if (MessageBox("退出后将不能检测指定程序的运行状态，可能就无法完成下次开机"
        "后打开上次未关闭的进程了哦。主人确定要残忍的关闭我么？", "退出提醒",
        MB_YESNO | MB_ICONQUESTION) == IDYES)
        PostQuitMessage(0);
}

void CProcessAssistantDlg::OnClose()
{
    ShowWindow(SW_HIDE);
    strcpy(m_nid.szInfoTitle, "进程助手已隐藏到通知栏");//气泡标题
    strcpy(m_nid.szInfo, "我将在这里检查主人设置的程序是否在运行，如果真的"
           "想关闭我，可以右键单击我，然后选择退出");//气泡内容
    Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}

LRESULT CProcessAssistantDlg::OnNotifyIconMsg(WPARAM wParam, LPARAM lParam)
{
    switch (lParam) {
        case WM_RBUTTONDOWN: //如果按下鼠标右建
        {
            CMenu pMenu;//加载菜单
            if (pMenu.LoadMenu(IDR_MENU1)) {
                CPoint pt;
                GetCursorPos(&pt);
                SetForegroundWindow();
                CMenu* pPopup = pMenu.GetSubMenu(0);
                pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
            }
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            OnOpenMainDlg();
            break;
        }
        default:
            break;
    }
    return 0;
}

void CProcessAssistantDlg::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2)
{
    if (nHotKeyId == ID_HK_OPEN_MAIN_DLG)
        OnOpenMainDlg();
    CDialog::OnHotKey(nHotKeyId, nKey1, nKey2);
}

void CProcessAssistantDlg::OnDblclkListProcess(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int item = pNMItemActivate->iItem;
    if (item >= 0 && item <= m_wndList.GetItemCount())
        m_wndList.SetCheck(item, !m_wndList.GetCheck(item));
    *pResult = 0;
}

void CProcessAssistantDlg::OnNM_RClickListProcess(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    m_selected = pNMItemActivate->iItem;
    if (m_selected >= 0 && m_selected <= m_wndList.GetItemCount()){
        int subIndex = (m_wndList.GetItemText(m_selected, 2) == "") ? 1 : 2;
        CMenu pMenu; //加载菜单
        if (pMenu.LoadMenu(IDR_MENU1)) {
            CPoint pt;
            GetCursorPos(&pt);
            CMenu* pPopup = pMenu.GetSubMenu(subIndex);
            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
        }
    }
    *pResult = 0;
}

void CProcessAssistantDlg::OnRClick_OpenDir()
{
    CString path = m_wndList.GetItemText(m_selected, 1);
    path = "/e,/select, " + path; //通过命令参数实现选定对应文件
    ShellExecute(m_hWnd, "open", "explorer", path, 0, SW_SHOW); //打开文件夹
}

void CProcessAssistantDlg::OnRClick_StartProcess()
{
    CString path = m_wndList.GetItemText(m_selected, 1);
    ShellExecute(m_hWnd, "open", path, 0, 0, SW_SHOW);
    updateProcessList(); //主动刷新列表
}

void CProcessAssistantDlg::OnRClick_EndProcess()
{
    CString exeName = m_wndList.GetItemText(m_selected, 0) + ".exe";
    HANDLE hd_ths = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hd_ths == INVALID_HANDLE_VALUE)
        return;
    PROCESSENTRY32 pe32 = { sizeof(pe32) };
    //遍历快照中所有的进程的信息，并将当前的信息保存到变量pe32中
    for (BOOL find = Process32First(hd_ths, &pe32); find != 0; find = Process32Next(hd_ths, &pe32)) {
        if (pe32.szExeFile == exeName){
            HANDLE hd_op = OpenProcess(PROCESS_ALL_ACCESS, 0, pe32.th32ProcessID);
            TerminateProcess(hd_op, 0); //结束进程
            CloseHandle(hd_op);
            updateProcessList(); //主动刷新列表
            break;
        }
    }
    CloseHandle(hd_ths);
}

void CProcessAssistantDlg::OnRClick_CheckOrNotProcess()
{
    if (m_selected >= 0 && m_selected <= m_wndList.GetItemCount())
        m_wndList.SetCheck(m_selected, !m_wndList.GetCheck(m_selected));
}

// CProcessAssistantDlg 自定义函数

bool CProcessAssistantDlg::isProcessExist(CString name)
{
    HANDLE hp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hp == INVALID_HANDLE_VALUE)
        return false;
    int i = name.ReverseFind('\\');
    if (i != -1)
        name = name.Right(name.GetLength() - i - 1);
    PROCESSENTRY32 pe32 = { sizeof(pe32) };
    for (BOOL find = Process32First(hp, &pe32); find != 0; find = Process32Next(hp, &pe32)) {
        if (name == pe32.szExeFile){
            CloseHandle(hp);
            return true;
        }
    }
    CloseHandle(hp);
    return false;
}

void CProcessAssistantDlg::showProcessList()
{
    loadListFromTaskFile();
    loadListFromTaskMgr();
}

void CProcessAssistantDlg::loadListFromTaskFile()
{
    m_listCnt = 0;
    ifstream fin(m_checkedListFile);
    if (fin.is_open()){
        string process;
        while (getline(fin, process)){
            if (process == "" || !PathFileExists(process.c_str()))
                break;
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), process.c_str(), 0);
            int indexIcon = m_iconList.Add(hIcon);
            CString exeName = process.substr(process.rfind('\\') + 1).c_str(); //仅得到文件名
            int item = m_wndList.InsertItem(m_listCnt++, exeName.Left(exeName.GetLength() - 4), indexIcon);
            m_wndList.SetItemText(item, 1, process.c_str());
            m_wndList.SetCheck(item);
            if (isProcessExist(process.c_str()))
                m_wndList.SetItemText(item, 2, "运行中");
            m_processList.insert(process.c_str()); //存入map
            m_processIndexMap[process.c_str()] = m_listCnt - 1;
            m_checkedList.insert(process.c_str());
        }
    }
    fin.close();
}

void CProcessAssistantDlg::loadListFromTaskMgr()
{
    m_listCnt = m_wndList.GetItemCount();
    static char myName[MAX_PATH] = { 0 }; //本程序的进程名
    static int unused1 = GetModuleFileName(NULL, myName, MAX_PATH);
    static char unused2 = myName[0] = toupper(myName[0]); //避免盘符大小写不识别问题
    HANDLE hp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hp == INVALID_HANDLE_VALUE){
        CString err;
        err.Format("%d", GetLastError());
        MessageBox("建立进程快照失败！" + err, "温馨提示");
        return;
    }
    PROCESSENTRY32 pe32 = { sizeof(pe32) };
    //遍历快照中所有的进程的信息，并将当前的信息保存到变量pe32中
    for (BOOL find = Process32First(hp, &pe32); find != 0; find = Process32Next(hp, &pe32)) {
        HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, 0, pe32.th32ProcessID);
        if (hd != NULL) {
            char exePath[255];
            GetModuleFileNameEx(hd, NULL, exePath, 255);
            exePath[0] = toupper(exePath[0]); //避免盘符大小写不识别问题
            if (m_processList.count(exePath) == 0){
                //获得程序图标
                HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), exePath, 0);
                if (hIcon != NULL){ //路径有效并且有图标
                    CString path = exePath, exeName = pe32.szExeFile;
                    if (path == myName || path.Find("system32") != -1 || path.Find("SysWOW64") != -1){
                        DestroyIcon(hIcon); //销毁图标
                        CloseHandle(hd);
                        continue;
                    }
                    int indexIcon = m_iconList.Add(hIcon);
                    int item = m_wndList.InsertItem(m_listCnt++, exeName.Left(exeName.GetLength() - 4), indexIcon); //插入一项
                    m_wndList.SetItemText(item, 1, exePath);
                    m_wndList.SetItemText(item, 2, "运行中");
                    m_processList.insert(exePath); //存入该记录
                    m_processIndexMap[exePath] = m_listCnt - 1;
                    DestroyIcon(hIcon); //销毁图标
                }
            }
            CloseHandle(hd);
        }
    }
    CloseHandle(hp);
}

void CProcessAssistantDlg::openUnclosedProcess()
{
    ifstream fin(m_autoRunListFile);
    if (fin.is_open()){
        string process;
        while (getline(fin, process)){
            if (!isProcessExist(process.c_str())){
                if ((int)ShellExecute(0, "open", process.c_str(), 0, 0, SW_SHOW) < 32){
                    CString err;
                    err.Format("打开%s失败! lastError:%d", process.c_str(),
                               GetLastError());
                    MessageBox(err, "失败提示", MB_ICONERROR);
                }
            }
        }
    }
    fin.close();
    DeleteFile(m_autoRunListFile);
}

void CProcessAssistantDlg::updateProcessList()
{
    //删除已退出的进程的行和记录
    for (auto it = m_processList.begin(); it != m_processList.end();){
        auto itTmp = it++;
        int idx = m_processIndexMap[*itTmp]; //当前记录所在行的索引
        if (!isProcessExist(*itTmp)){ //该进程已不存在
            if (m_checkedList.count(*itTmp) == 0){ //不在自启动列表中,则需要删除
                for (auto& elem : m_processIndexMap) //将该进程下面的进程序号减一
                    if (elem.second > idx)
                        --elem.second;
                m_wndList.DeleteItem(idx); //删除行
                m_processIndexMap.erase(m_processIndexMap.find(*itTmp));
                m_processList.erase(itTmp); //从进程列表映射表中删除
            } //进程不存在但在自启动列表中,只需要将状态修改
            else if (m_wndList.GetItemText(idx, 2) != "")
                m_wndList.SetItemText(idx, 2, "");
        }
        else if (m_checkedList.count(*itTmp) != 0){ //在自启动列表中
            if (m_wndList.GetItemText(idx, 2) != "运行中")
                m_wndList.SetItemText(idx, 2, "运行中");
        }
    }
    //载入任务管理器中的任务列表
    loadListFromTaskMgr();
    //将正在运行的需要自启动的进程保存到文件中,以供下次开机可自动打开
    CString listsToRun;
    static CString lastLists;
    for (auto& elem : m_checkedList)
        if (isProcessExist(elem))
            listsToRun += elem + "\n";
    if (listsToRun != lastLists){
        if (listsToRun.IsEmpty())
            DeleteFile(m_autoRunListFile);
        else{
            ofstream fout(m_autoRunListFile);
            fout << listsToRun;
            fout.close();
        }
        lastLists = listsToRun;
    }
}


///另一种获取进程列表的方式
#include <Psapi.h>
#pragma comment(lib,"Psapi.lib")
void enumProcesses()
{
    int cnt = 0;
    DWORD pid[1024], needed;
    EnumProcesses(pid, sizeof(pid), &needed);
    DWORD numProcess = needed / sizeof(DWORD);
    char exePath[MAX_PATH];
    for (DWORD i = 0; i < numProcess; i++)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid[i]);
        if (hProcess != NULL)
        {
            GetModuleFileNameEx(hProcess, NULL, exePath, sizeof(exePath));
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), exePath, 0); //获得程序图标
            CString path = exePath, exeName = path.Right(path.GetLength() - path.ReverseFind('/'));
            DestroyIcon(hIcon); //销毁图标
        }
        CloseHandle(hProcess);
    }
}
