# lab0 report

## 思考题

### 0.1

实验中三个文件内容如下：

![1678003978077](image/lab0_report/1678003978077.png)

Untracked.txt中显示未追踪的 README.txt 在执行`git add .`之后变为Stage.txt显示的待提交变更

在执行`git commit -m 学号`后，README.txt变为 "未修改" 状态，对其进行编辑后其变为 "已修改(但未暂存)" 状态

### 0.2

- add the file : `git add (file_name)`
  
- stage the file : `git add (file_name)`
  
- commit : `git commit`
  
### 0.3

1. 如果在暂存区留有print.c可以使用`git checkout -- print.c`; 如果在版本库留有对应文件，也可以使用`git checkout HEAD print.c`
   
2. 执行`git checkout HEAD print.c`(因为git rm除了删除工作区文件也会将删除记入暂存区)或者`git reset HEAD`回到1. 的情况
   
3. 执行`git rm -- hello.txt`
   
### 0.4

![1678006716002](image/lab0_report/1678006716002.png)

![1678006734764](image/lab0_report/1678006734764.png)

![1678006759503](image/lab0_report/1678006759503.png)

![1678006772035](image/lab0_report/1678006772035.png)

### 0.5

![1678006978936](image/lab0_report/1678006978936.png)

`>>`会在文件末尾添加而`>`是覆盖写入

### 0.6

![1678009613808](image/lab0_report/1678009613808.png)

1. test文件操作说明：
   
    定义变量a=1,b=1,c=a+b=3
   
    将变量c,b,a的值分别写入file1,file2,file3
   
    此时，file1内容为3，file2内容为2，file3内容为1
   
    将file1,file2,file3一个个写入file4
   
    file4内容为3\n2\n1
   
    将file4写入result文件末尾
   
2. `echo echo Shell Start` 与 `echo 'echo Shell Start'`效果没有区别，但是`echo echo $c>file1` 与 `echo 'echo $c>file1'`效果有区别,前者`$c`会被视为变量，后者原样输出
   
## 难点分析

本次实验重点在sh脚本和make文件的编写，第一个难点在于是否知道命令，第二个难点是特殊字符转义与否的控制

## 实验体会

编写带有特殊字符，如',的命令脚本用来很长时间，经常有将$变量抑制导致脚本执行错的情况

