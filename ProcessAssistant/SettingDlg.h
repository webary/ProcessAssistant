#pragma once


// SettingDlg 对话框

class SettingDlg : public CDialogEx
{
    DECLARE_DYNAMIC(SettingDlg)

public:
    SettingDlg(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~SettingDlg();

    // 对话框数据
    enum {
        IDD = IDD_SET
    };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedSetOk();
    void setStartup(bool enable);   //设置开机自启动状态
    bool getStartup();              //是否开启开机自启动
    afx_msg void OnBnClickedCheckOpenLastDir();
};
