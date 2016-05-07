
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

    // 对话框数据
    enum {
        IDD = IDD_PROCESSASSISTANT_DIALOG
    };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedBtSet();
    DECLARE_MESSAGE_MAP()

protected:
    HICON m_hIcon;
    std::map<CString, CString> m_processListMap; //进程列表
    std::map<CString, int> m_processIndexMap;    //进程对应表项序号
    std::set<CString> m_runList; //需要开机自启动的进程集合
    CListCtrl m_wndList;   //进程列表控件变量
    CImageList m_imgList;  //图标列表
    int m_listCnt;         //列表计数器
    CString m_myPath;      //临时文件夹中的个人文件夹
    SettingDlg setDlg;     //设置对话框
    CString m_autoRunFile; //保存需要自动运行的进程的文件

public:
    bool isProcessExist(CString name); //判断指定进程是否存在(即是否已打开该程序)
    void showProcessList();            //显示进程列表
    void loadTaskFileList();           //从任务列表文件获取进程列表
    void loadTaskMgrList();            //从任务管理器获取进程列表
    void openUnclosedProcess();        //打开上次未关闭的进程
    void updateProcessList();          //更新进程列表
};
