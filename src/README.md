1.配置文件filter
  项目->属性->c/c++ General->Paths and Symbols
  选择Source Location,展开项目/src，编辑filter
  
2.mac下跨项目编译执行时，需要在控制台下设置
	export DYLD_LIBRARY_PATH=/Users/ltzd/Documents/workspacecpp/common/Debug
  然后在控制台下执行可执行文件
  也可以在run configures下的envirements配置环境变量
  mac下配置为:DYLD_LIBRARY_PATH=/Users/ltzd/Documents/workspacecpp/common/Debug:/Users/ltzd/Documents/workspacecpp/game/Debug