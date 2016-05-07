
// ProcessAssistantDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ProcessAssistant.h"
#include "ProcessAssistantDlg.h"
#include "afxdialogex.h"
#include "tlhelp32.h"
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
}

void CProcessAssistantDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS, m_wndList);
}

BEGIN_MESSAGE_MAP(CProcessAssistantDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDOK, &CProcessAssistantDlg::OnBnClickedOk)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CProcessAssistantDlg 消息处理程序

BOOL CProcessAssistantDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO:  在此添加额外的初始化代码
    //整行选定 + 显示表格线 + 复选框 + 扁平滚动条
    m_wndList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_FLATSB);
    //设置表头
    m_wndList.InsertColumn(0, "(开启)   进程名", 0, 124);
    m_wndList.InsertColumn(1, "文件位置", 0, 415);
    m_wndList.InsertColumn(2, "备注", 0, 50);
    //设置文本显示颜色
    m_wndList.SetTextColor(RGB(0, 255, 0));

    SetTimer(0, 100, NULL);  //检查当前任务管理器中的进程,并添加启动项
    SetTimer(1, 1, NULL);    //打开上次关闭时设置的进程
    SetTimer(2, 2000, NULL); //查看监测的进程是否在运行

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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CProcessAssistantDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


#include <psapi.h>
#pragma comment(lib,"Psapi.lib")
void CProcessAssistantDlg::showProcessList()
{
    HANDLE hp = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    if (hp == INVALID_HANDLE_VALUE){
        CString err;
        err.Format("%d", GetLastError());
        MessageBox("建立进程快照失败！" + err, "温馨提示");
        return;
    }
    m_wndList.DeleteAllItems();
    int cnt = 0;
    static CImageList *imgList = NULL;
    delete imgList;
    imgList = new CImageList;
    imgList->Create(32, 32, ILC_COLOR32, 0, 100);
    m_wndList.SetImageList(imgList, LVSIL_SMALL);

    m_processListMap.clear();
    m_runList.clear();
    ifstream fin(m_myPath + "AutoRunProcessList.txt");
    if (fin.is_open()){
        string process;
        while (getline(fin, process)){
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), process.c_str(), 0);
            int indexIcon = imgList->Add(hIcon);
            CString exeName = process.substr(process.rfind('\\') + 1).c_str();
            int item = m_wndList.InsertItem(cnt++, exeName.Left(exeName.GetLength() - 4), indexIcon); //插入一项
            m_wndList.SetItemText(item, 1, process.c_str());
            m_wndList.SetCheck(item);
            if (isProcessExist(process.c_str()))
                m_wndList.SetItemText(item, 2, "运行中");
            m_processListMap[process.c_str()] = exeName; //存入map
            m_runList.push_back(process.c_str());
        }
    }
    fin.close();

    PROCESSENTRY32 pe32 = { sizeof(pe32) };
    for (BOOL find = Process32First(hp, &pe32); find != 0; find = Process32Next(hp, &pe32)) {
        HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, 0, pe32.th32ProcessID);
        if (hd != NULL) {
            char exePath[255];
            GetModuleFileNameEx(hd, NULL, exePath, 255);
            //获得程序图标
            HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), exePath, 0);
            if (hIcon != NULL && m_processListMap.find(exePath) == m_processListMap.end()){
                CString path = exePath, exeName = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
                static char myName[MAX_PATH] = { 0 }; //本程序的进程名
                static int unused1 = GetModuleFileName(NULL, myName, MAX_PATH);
                if (path == myName || path.Find("system32") != -1 || path.Find("SysWOW64") != -1)
                    continue;
                int indexIcon = imgList->Add(hIcon);
                int item = m_wndList.InsertItem(cnt++, exeName.Left(exeName.GetLength() - 4), indexIcon); //插入一项
                m_wndList.SetItemText(item, 1, exePath);
                if (isProcessExist(exeName))
                    m_wndList.SetItemText(item, 2, "运行中");
                m_processListMap[exePath] = exeName; //存入map
            }
            DestroyIcon(hIcon); //销毁图标
            CloseHandle(hd);
        }
    }
    CloseHandle(hp);
}


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

void CProcessAssistantDlg::OnBnClickedOk()
{
    m_runList.clear();
    ofstream fout(m_myPath + "AutoRunProcessList.txt");
    for (int i = 0; i < m_wndList.GetItemCount(); ++i)
        if (m_wndList.GetCheck(i)) {
            fout << m_wndList.GetItemText(i, 1) << endl;
            m_runList.push_back(m_wndList.GetItemText(i, 1));
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


void CProcessAssistantDlg::OnTimer(UINT_PTR nIDEvent)
{
    static CString autoRunFile = m_myPath + "autorun.txt";
    switch (nIDEvent)
    {
        case 0: //在本程序启动时执行一次, 检查当前任务管理器中的进程,并添加启动项
        {
            KillTimer(0);
            showProcessList();
            //判断是否有开机启动项，如果没有则添加
            HKEY hKey;
            LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
            long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE | KEY_READ, &hKey);
            if (lRet == ERROR_SUCCESS)
            {
                char myName[MAX_PATH] = { 0 };
                DWORD dwRet = GetModuleFileName(NULL, myName, MAX_PATH); //得到程序自身的全路径
                char data[1000] = { 0 };
                DWORD dwType = REG_SZ, dwSize;
                RegQueryValueEx(hKey, "ProcessAssistant", 0, &dwType, (BYTE*)data, &dwSize); //看是否已有该启动项
                if (CString(data) == myName){
                    MessageBox("已存在该程序开机自启动项！", "温馨提示");
                    break;
                }
                lRet = RegSetValueEx(hKey, "ProcessAssistant", 0, REG_SZ, (BYTE *)myName, dwRet);
                RegCloseKey(hKey);
                if (lRet != ERROR_SUCCESS)
                {
                    MessageBox("系统参数错误,不能完成开机启动设置", "温馨提示");
                }
                else
                {
                    MessageBox("已成功设置该程序开机自启动！", "温馨提示", MB_ICONINFORMATION);
                }
            }
            else
            {
                CString err;
                err.Format("lastError:%d(lRet:%d)", GetLastError(), lRet);
                MessageBox("打开注册表中启动项目录失败!" + err, "失败提示");
            }
            break;
        }
        case 1: //在本程序启动时执行一次, 自动打开上次未关闭的进程
        {
            KillTimer(1);
            ifstream fin(autoRunFile);
            if (fin.is_open()){
                string process;
                while (getline(fin, process)){
                    if (!isProcessExist(process.c_str()))
                        ShellExecute(0, "open", process.c_str(), 0, 0, SW_SHOW);
                }
            }
            fin.close();
            DeleteFile(autoRunFile);
            break;
        }
        case 2: //每隔一定时间会执行一次, 查看监测的进程是否在运行
        {
            CString lists;
            for (auto& elem : m_runList){
                if (isProcessExist(elem)){
                    lists += elem + "\n";
                }
            }
            if (lists != ""){
                ofstream fout(autoRunFile);
                fout << lists;
                fout.close();
            }
            else{
                DeleteFile(autoRunFile);
            }
            break;
        }
        default:
            break;
    }
    CDialog::OnTimer(nIDEvent);
}


//另一种方式获取进程列表
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
