
// ProcessAssistantDlg.h : 头文件
//

#pragma once
#include <map>
#include <vector>
#include "afxwin.h"

// CProcessAssistantDlg 对话框
class CProcessAssistantDlg : public CDialog
{
    // 构造
public:
    CProcessAssistantDlg(CWnd* pParent = NULL);	// 标准构造函数

    // 对话框数据
    enum {
        IDD = IDD_PROCESSASSISTANT_DIALOG
    };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

protected:
    HICON m_hIcon;
    std::map<CString, CString> m_processListMap; //进程列表
    std::vector<CString> m_runList; //需要开机自启动的进程列表
    CListCtrl m_wndList; //进程列表控件变量
    CString m_myPath;      //临时文件夹中的个人文件夹

public:
    void showProcessList();
    bool isProcessExist(CString name);
    afx_msg void OnBnClickedOk();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};
