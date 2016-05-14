
// ProcessAssistantDlg.h : 头文件
//

#pragma once
#include <map>
#include <set>
#include "afxwin.h"
#include "SettingDlg.h"

// CProcessAssistantDlg 对话框
class CProcessAssistantDlg : public CDialog
{
public:
    CProcessAssistantDlg(CWnd* pParent = NULL);	// 标准构造函数
    ~CProcessAssistantDlg();
    // 对话框数据
    enum {
        IDD = IDD_PROCESSASSISTANT_DIALOG
    };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedBtSet();
    afx_msg void OnOpenMainDlg();  //任务栏菜单-打开主界面
    afx_msg void OnExitMe();       //任务栏菜单-退出
    afx_msg void OnClose();        //对话框的关闭消息
    afx_msg LRESULT OnNotifyIconMsg(WPARAM wParam, LPARAM lParam);//处理通知栏消息
    afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2); //响应快捷键消息
    afx_msg void OnDblclkListProcess(NMHDR *pNMHDR, LRESULT *pResult); //双击消息
    afx_msg void OnNM_RClickListProcess(NMHDR *pNMHDR, LRESULT *pResult); //右键单击消息
    afx_msg void OnRClick_OpenDir();           //右键菜单-打开所在位置
    afx_msg void OnRClick_StartProcess();      //右键菜单-打开该进程
    afx_msg void OnRClick_EndProcess();        //右键菜单-结束该进程
    afx_msg void OnRClick_CheckOrNotProcess(); //右键菜单-勾选/不勾选该进程
    DECLARE_MESSAGE_MAP()

protected:
    HICON m_hIcon;
    std::set<CString> m_processList; //在表格中显示的所有进程集合(包含m_checkedList)
    std::set<CString> m_checkedList; //需要开机自启动的进程集合
    std::map<CString, int> m_processIndexMap; //进程对应表项序号的映射表
    CListCtrl m_wndList;        //进程列表控件变量
    CImageList m_iconList;      //图标列表
    int m_listCnt;              //列表计数器
    SettingDlg m_setDlg;        //设置对话框
    CString m_checkedListFile;  //保存已勾选的进程列表的文件(包含m_autoRunListFile)
    CString m_autoRunListFile;  //保存需要自动运行的进程列表的文件
    NOTIFYICONDATA m_nid;       //通知栏消息的结构体变量
    HANDLE m_hmutex;            //保证最多只有一个实例在运行的互斥锁
    void *pMapRunning;          //指向共享内存区的指针
    int m_selected;             //当前已选中的表项行索引

public:
    bool isProcessExist(CString name); //判断指定进程是否存在(即是否已打开该程序)
    void showProcessList();            //显示进程列表
    void loadListFromTaskFile();       //从任务列表文件获取进程列表
    void loadListFromTaskMgr();        //从任务管理器获取进程列表
    void openUnclosedProcess();        //打开上次未关闭的进程
    void updateProcessList();          //更新进程列表(通过计时器自动调用)
    CString getFileDescription(const CString& filePathName); //获取exe文件的文件描述
};
