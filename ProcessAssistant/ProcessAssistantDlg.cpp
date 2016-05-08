
// ProcessAssistantDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ProcessAssistant.h"
#include "ProcessAssistantDlg.h"
#include <afxdialogex.h>

#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib,"Psapi.lib")

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

    char path[MAX_PATH] = "";
    GetTempPath(MAX_PATH, path);
    m_myPath = path + CString("ProcessAssistant\\");
    CreateDirectory(m_myPath, 0);

    m_autoRunFile = m_myPath + "autorun.txt";
}

CProcessAssistantDlg::~CProcessAssistantDlg()
{
    Shell_NotifyIcon(NIM_DELETE, &nid);
    ReleaseMutex(hmutex);
}

void CProcessAssistantDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS, m_wndList);
}


#define WM_NOTIFYICONMSG WM_USER + 1 //托盘消息

BEGIN_MESSAGE_MAP(CProcessAssistantDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_MESSAGE(WM_NOTIFYICONMSG, OnNotifyIconMsg)
    ON_BN_CLICKED(IDOK, &CProcessAssistantDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BT_SET, &CProcessAssistantDlg::OnBnClickedBtSet)
    ON_COMMAND(ID_OPEN_MAIN_DLG, &CProcessAssistantDlg::OnOpenMainDlg)
    ON_COMMAND(ID_EXIT_ME, &CProcessAssistantDlg::OnExitMe)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_PROCESS, &CProcessAssistantDlg::OnDblclkListProcess)
END_MESSAGE_MAP()


// CProcessAssistantDlg 消息处理程序

BOOL CProcessAssistantDlg::OnInitDialog()
{
    //利用互斥锁机制保证最多只有一个该实例正在运行
    hmutex = ::CreateMutex(NULL, true, "ProcessAssistant");
    if (ERROR_ALREADY_EXISTS == GetLastError()) { //若互斥锁已存在则直接关闭
        MessageBox("请不要重复打开该程序哦,我的前身还在通知栏运行着呢^ _ ^", 0, MB_ICONWARNING);
        OnCancel();
    }
    CDialog::OnInitDialog();
    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    //整行选定 + 显示表格线 + 复选框 + 扁平滚动条
    m_wndList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_FLATSB);
    //设置表头
    m_wndList.InsertColumn(0, "(开启)   进程名", 0, 124);
    m_wndList.InsertColumn(1, "文件位置", 0, 415);
    m_wndList.InsertColumn(2, "备注", 0, 50);
    //设置文本显示颜色
    m_wndList.SetTextColor(RGB(0, 255, 0));

    m_listCnt = 0;
    m_imgList.Create(32, 32, ILC_COLOR32, 0, 100);
    m_wndList.SetImageList(&m_imgList, LVSIL_SMALL);

    SetTimer(0, 100, NULL);  //检查当前任务管理器中的进程,并添加启动项
    SetTimer(1, 1000, NULL); //查看监测的进程是否在运行
    openUnclosedProcess();   //打开上次关机时未关闭的进程

    //设置托盘消息 - 必须在这里赋值，如果在构造函数赋值，鼠标指向托盘图标后即消失
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hWnd;
    nid.uFlags = NIF_INFO | NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.dwInfoFlags = NIIF_INFO;
    nid.hIcon = m_hIcon;
    nid.uCallbackMessage = WM_NOTIFYICONMSG;
    strcpy(nid.szTip, "进程助手");
    strcpy(nid.szInfoTitle, "进程助手已驻扎在通知栏");//气泡标题
    strcpy(nid.szInfo, "我将在这里检查主人设置的程序是否在运行，您可以点击"
           "主窗口的关闭，我还会一直在这里运行着，如果真的想关闭我，"
           "可以右键单击我，然后选择退出");//气泡内容
    Shell_NotifyIcon(NIM_ADD, &nid);

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

HCURSOR CProcessAssistantDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CProcessAssistantDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
        case 0: //在本程序启动时执行一次, 检查当前任务管理器中的进程,并添加启动项
        {
            KillTimer(0);
            showProcessList();
            //该电脑上第一次运行该程序时默认设置为自启动
            bool firstRun = false;
            ifstream fin(m_myPath + "AutoRunProcessList.txt");
            if (!fin.is_open())
                firstRun = true;
            fin.close();
            if (firstRun){
                setDlg.setStartup(1);
                //保证该文件在之后一直存在
                ofstream fout(m_myPath + "AutoRunProcessList.txt", ios::out | ios::app);
                fout.close();
                MessageBox("勾选对应程序左边的选框即可开启开机自启动项，关机时"
                           "这些进程如果未关闭，下次开机后运行此程序将自动运行"
                           "对应进程！", "开启提示", MB_ICONINFORMATION);
            }
            break;
        }
        case 1: //每隔一定时间会执行一次, 查看监测的进程是否在运行
        {
            updateProcessList();
            break;
        }
        default:
            break;
    }
    CDialog::OnTimer(nIDEvent);
}

void CProcessAssistantDlg::OnBnClickedOk()
{
    m_runList.clear();
    ofstream fout(m_myPath + "AutoRunProcessList.txt");
    for (int i = 0; i < m_listCnt; ++i)
        if (m_wndList.GetCheck(i)) {
            fout << m_wndList.GetItemText(i, 1) << endl;
            m_runList.insert(m_wndList.GetItemText(i, 1));
        }
    fout.close();

    CString selected;
    for (auto& elem : m_runList)
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
    setDlg.DoModal();
}

void CProcessAssistantDlg::OnOpenMainDlg()
{
    ShowWindow(SW_SHOW);
}

void CProcessAssistantDlg::OnExitMe()
{
    PostQuitMessage(0);
}

void CProcessAssistantDlg::OnClose()
{
    ShowWindow(SW_HIDE);
    strcpy(nid.szInfoTitle, "进程助手已隐藏到通知栏");//气泡标题
    strcpy(nid.szInfo, "我将在这里检查主人设置的程序是否在运行，如果您关机的"
           "时候没有关闭他们，我将在下次开机时为主人打开他们哦");//气泡内容
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

LRESULT CProcessAssistantDlg::OnNotifyIconMsg(WPARAM wParam, LPARAM lParam)
{
    CPoint Point;
    CMenu pMenu;//加载菜单
    switch (lParam) {
        case WM_RBUTTONDOWN: //如果按下鼠标右建
            if (pMenu.LoadMenu(IDR_MENU1)) {
                CMenu* pPopup = pMenu.GetSubMenu(0);
                GetCursorPos(&Point);
                SetForegroundWindow();
                pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, Point.x, Point.y, this);
            }
            break;
        case WM_LBUTTONDBLCLK:
            this->ShowWindow(SW_SHOW);
            break;
        default:
            break;
    }
    return 0;
}

void CProcessAssistantDlg::OnDblclkListProcess(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int item = pNMItemActivate->iItem;
    if (item >= 0 && item <= m_wndList.GetItemCount())
        m_wndList.SetCheck(item, !m_wndList.GetCheck(item));
    *pResult = 0;
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
    loadTaskFileList();
    loadTaskMgrList();
}

void CProcessAssistantDlg::loadTaskFileList()
{
    ifstream fin(m_myPath + "AutoRunProcessList.txt");
    if (fin.is_open()){
        string process;
        while (getline(fin, process)){
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), process.c_str(), 0);
            int indexIcon = m_imgList.Add(hIcon);
            CString exeName = process.substr(process.rfind('\\') + 1).c_str();
            int item = m_wndList.InsertItem(m_listCnt++, exeName.Left(exeName.GetLength() - 4), indexIcon); //插入一项
            m_wndList.SetItemText(item, 1, process.c_str());
            m_wndList.SetCheck(item);
            if (isProcessExist(process.c_str()))
                m_wndList.SetItemText(item, 2, "运行中");
            m_processListMap[process.c_str()] = exeName; //存入map
            m_processIndexMap[process.c_str()] = m_listCnt - 1;
            m_runList.insert(process.c_str());
        }
    }
    fin.close();
}

void CProcessAssistantDlg::loadTaskMgrList()
{
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
    for (BOOL find = Process32First(hp, &pe32); find != 0; find = Process32Next(hp, &pe32)) {
        HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, 0, pe32.th32ProcessID);
        if (hd != NULL) {
            char exePath[255];
            GetModuleFileNameEx(hd, NULL, exePath, 255);
            exePath[0] = toupper(exePath[0]); //避免盘符大小写不识别问题
            //获得程序图标
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), exePath, 0);
            if (hIcon != NULL && m_processListMap.find(exePath) == m_processListMap.end()){
                CString path = exePath, exeName = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
                if (path == myName || path.Find("system32") != -1 || path.Find("SysWOW64") != -1)
                    continue;
                int indexIcon = m_imgList.Add(hIcon);
                int item = m_wndList.InsertItem(m_listCnt++, exeName.Left(exeName.GetLength() - 4), indexIcon); //插入一项
                m_wndList.SetItemText(item, 1, exePath);
                m_wndList.SetItemText(item, 2, "运行中");
                m_processListMap[exePath] = exeName; //存入map
                m_processIndexMap[exePath] = m_listCnt - 1;
            }
            DestroyIcon(hIcon); //销毁图标
            CloseHandle(hd);
        }
    }
    CloseHandle(hp);
}

void CProcessAssistantDlg::openUnclosedProcess()
{
    ifstream fin(m_autoRunFile);
    if (fin.is_open()){
        string process;
        while (getline(fin, process)){
            if (!isProcessExist(process.c_str())){
                if ((int)ShellExecute(0, "open", process.c_str(), 0, 0, SW_SHOW) < 32){
                    CString err;
                    err.Format("打开%s失败!lastError:%d", process.c_str(),
                               GetLastError());
                    MessageBox(err, "失败提示");
                }
            }
        }
    }
    fin.close();
    DeleteFile(m_autoRunFile);
}

void CProcessAssistantDlg::updateProcessList()
{
    //删除已退出的进程的行和记录
    for (auto it = m_processListMap.begin(); it != m_processListMap.end();){
        auto itTmp = it++;
        CString path = itTmp->first;
        if (!isProcessExist(itTmp->second)){ //该进程已不存在
            if (m_runList.count(itTmp->first) == 0){ //不在自启动列表中,则需要删除
                for (auto& elem : m_processIndexMap)  //将该进程下面的进程序号减一
                    if (elem.second > m_processIndexMap[path])
                        --elem.second;
                m_wndList.DeleteItem(m_processIndexMap[path]); //删除行
                m_processIndexMap.erase(m_processIndexMap.find(path));
                m_processListMap.erase(itTmp); //从进程列表映射表中删除
            }
            else{ //进程不存在但在自启动列表中,只需要将状态修改
                if (m_wndList.GetItemText(m_processIndexMap[path], 2) != "")
                    m_wndList.SetItemText(m_processIndexMap[path], 2, "");
            }
        }
    }
    //更新每个进程的状态
    m_listCnt = m_wndList.GetItemCount();
    for (int i = 0; i < m_listCnt; ++i){
        if (isProcessExist(m_wndList.GetItemText(i, 1))){
            if (m_wndList.GetItemText(i, 2) != "运行中")
                m_wndList.SetItemText(i, 2, "运行中");
        }
        else if (m_wndList.GetItemText(i, 2) != "")
            m_wndList.SetItemText(i, 2, "");
    }
    //载入任务管理器中的任务列表
    loadTaskMgrList();
    //将正在运行的需要自启动的进程保存到文件中,以供下次开机可自动打开
    CString lists;
    for (auto& elem : m_runList)
        if (isProcessExist(elem))
            lists += elem + "\n";
    if (lists != ""){
        ofstream fout(m_autoRunFile);
        fout << lists;
        fout.close();
    }
    else
        DeleteFile(m_autoRunFile);
}



///另一种获取进程列表的方式
#include <Psapi.h>
#pragma comment(lib,"Psapi.lib")
void enumProcesses()
{
    int cnt = 0;
    DWORD PID[1024];
    DWORD needed;
    EnumProcesses(PID, sizeof(PID), &needed);
    DWORD NumProcess = needed / sizeof(DWORD);
    char exePath[MAX_PATH];

    for (DWORD i = 0, cnt = 0; i < NumProcess; i++, cnt++)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID[i]);
        if (hProcess)
        {
            GetModuleFileNameEx(hProcess, NULL, exePath, sizeof(exePath));
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), exePath, 0); //获得程序图标
            CString path = exePath, exeName = path.Right(path.GetLength() - path.ReverseFind('/'));
            DestroyIcon(hIcon); //销毁图标
        }
        CloseHandle(hProcess);
    }
}
