# ProcessAssistant  
open those process automatically which were not closed at last moment of shut down.

<hr>
###platform  
win8.1, visual studio 2013, MFC

###demo  
![运行效果示例图](demo.png)

###technical skills  
1. traverse the task manager to get the name and id of each process  
2. get the whole path name of the localtion of each process according to its process id  
3. get the icon of each process  
4. show the list of process with icon in a table  
5. make it available to select which process will be permitted to run on the startup by the user  
6. mark down those process which user selected and they are running  
7. update the process list in the table regularly  
8. run itself when the computer startup(set the boot entry)  
9. open the appointed process automatically when the computer startup  
10. add global and local hotkey to make some functions convenient  
11. add notify icon and message  
  
  
<hr>
##中文说明：  

###开发平台  
win8.1, visual studio 2013, MFC

###运行示例  
![运行效果示例图](demo.png)

###技术点  
1. 遍历任务管理器获取每个进程的名称和id  
2. 根据进程id打开进程然后获取它们所在位置的完整路径  
3. 获取每个进程的图标  
4. 在一个表格中显示具有图标的进程  
5. 允许用户选择哪些进程可以在开机时自启动(上次关机未关闭的进程才会自启动)  
6. 记录用户选择了的进程和正在运行的进程  
7. 定期更新表格中的进程列表  
8. 在电脑启动时该程序自启动(需要提权,并修改系统注册表中的启动项)  
9. 当电脑开机后自动打开用户指定的并且上次未关闭的进程  
10. 增加全局快捷键和局部快捷键,让一些功能操作更方便  
11. 增加通知栏图标和菜单消息  
