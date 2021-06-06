# <p align="center"> Simple Player </p>

### ***雷霄骅 YYDS! 感谢雷神的教程！***

### 搭建环境(ffmpeg+SDL2)

1. 下载头文件和库  
   * ffmpeg:[https://github.com/BtbN/FFmpeg-Builds/releases]  
    选择`'ffmpeg-*-win64-gpl-shared.zip'`,这里面是头文件和动态连接库

   * SDL2:[https://www.libsdl.org/download-2.0.php]  
    选择`Development Libraries`中的`'SDL2-devel-2.0.14-VC.zip'`
2. 解压放置到对应目录  
   先运行项目中的`gen_project.bat`,会自动生成工程文件
   * 将`ffmpeg`和`SDL2`的头文件分别放到`dependency\include\ffmpeg`和`dependency\include\SDL2`中  
   * 将`ffmpeg`的`lib`目录中的所有文件放到`dependency\lib`  
     将`SDL2`中的`lib\x64\SDL2.lib`放到`dependency\lib`  
   * 将`ffmpeg`的`bin`目录中的所有`*.dll`文件放到`buildx64\output\bin\Release`  
     将`SDL2`中`lib\x64\SDL2.dll`放到`buildx64\output\bin\Release`