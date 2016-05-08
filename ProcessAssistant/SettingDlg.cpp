// SettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ProcessAssistant.h"
#include "SettingDlg.h"
#include "afxdialogex.h"


// SettingDlg 对话框

IMPLEMENT_DYNAMIC(SettingDlg, CDialogEx)

SettingDlg::SettingDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(SettingDlg::IDD, pParent)
{

}

SettingDlg::~SettingDlg()
{
}

void SettingDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SettingDlg, CDialogEx)
    ON_BN_CLICKED(ID_SETOK, &SettingDlg::OnBnClickedSetOk)
    ON_BN_CLICKED(IDC_CHECK_OPEN_LAST_DIR, &SettingDlg::OnBnClickedCheckOpenLastDir)
END_MESSAGE_MAP()


// SettingDlg 消息处理程序


BOOL SettingDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    if (getStartup()){
        ((CButton *)GetDlgItem(IDC_CHECK_STARTUP))->SetCheck(TRUE);
    }
    return TRUE;
}

void SettingDlg::setStartup(bool enable)
{
    bool setOK = false;
    //判断是否有开机启动项，如果没有则添加
    static LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY hKey;
    long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE | KEY_READ, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        if (enable){ //开启开机启动
            static char myName[MAX_PATH] = { 0 };
            static DWORD dwRet = GetModuleFileName(NULL, myName, MAX_PATH); //得到程序自身的全路径
            lRet = RegSetValueEx(hKey, "ProcessAssistant", 0, REG_SZ, (BYTE*)myName, dwRet);
            if (lRet == ERROR_SUCCESS)
                setOK = true;
        }
        else{ //关闭开机启动
            lRet = RegDeleteValue(hKey, "ProcessAssistant");
            if (lRet == ERROR_SUCCESS)
                setOK = true;
        }
        RegCloseKey(hKey);
    }
    if (!setOK)
    {
        CString err;
        err.Format("lastError:%d(lRet:%d)", GetLastError(), lRet);
        MessageBox("修改开机启动项失败!" + err, "失败提示");
    }
}

bool SettingDlg::getStartup()
{
    bool setOK = false; //记录是否已有开机自启动
    static LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY hKey;
    long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE | KEY_READ, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        static char myName[MAX_PATH] = { 0 };
        static DWORD dwRet = GetModuleFileName(NULL, myName, MAX_PATH); //得到程序自身的全路径
        char data[1000] = { 0 };
        DWORD dwType = REG_SZ, dwSize;
        RegQueryValueEx(hKey, "ProcessAssistant", 0, &dwType, (BYTE*)data, &dwSize);
        if (CString(data) == myName)
            setOK = true;
        RegCloseKey(hKey);
    }
    return setOK;
}

void SettingDlg::OnBnClickedSetOk()
{
    if ((BST_CHECKED == IsDlgButtonChecked(IDC_CHECK_STARTUP)) != getStartup())
        setStartup(IsDlgButtonChecked(IDC_CHECK_STARTUP) != 0);
    CDialogEx::OnOK();
}

void SettingDlg::OnBnClickedCheckOpenLastDir()
{
    if (IsDlgButtonChecked(IDC_CHECK_OPEN_LAST_DIR))
        MessageBox("请按照以下步骤设置此选项：\n1. 打开一个文件夹；\n"
        "2. 打开文件夹选项(win7: 工具->文件夹选项；win8/win10: 查看->选项)；"
        "\n3. 切换到查看属性页；\n"
        "4. 勾选\"登录时还原上一个文件夹窗口\"", "温馨提示");
}
