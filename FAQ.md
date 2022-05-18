## FAQ

### 编译相关问题

```c++
/**
 * @brief 记录写代码中遇到的编译bug
 * 
 */
void test_compile_bug()
{
   // Q: 头文件中定义函数后，编译出现重定义的错误
   // A: 源文件中定义；小函数可以再头文件中使用inline定义

   // Q: 尽量少使用前置声明

   // Q: 头文件递归包含

   // Q: hpp 多包含，重定义
   // A: 没有定义，只有类声明；还是重定义。后面把include的路径改掉后就解决了。
   // #include "cephfs/libcephfs.hpp" ==> #include "include/cephfs/libcephfs.hpp"
}
```

### 代码bug

**进程无异常日志，重启**

    Q: 某进程有异常处理模块，在调用动态库导出函数后会down掉，并且无任何异常日志。

    A: 1. 查看是否生成corefile
       2. 查看dmesg和top, 是否有异常    

该问题我在定位的时候，一直忘记看corefile，浪费了一上午时间。最后看完corefile，是动态库中挂了，boost中的lambda引用的成员是空指针。不用lambda后正常。（没看具体原因）

总结：进程异常，查看日志，coredump，dmesg，top等信息

**神奇的bug**

   osdc里的journaler，由于项目原因，拷贝了一份rmajournaler。其中类使用的多个回调函数中，有一个Context func_x不是在类内声明的，只是单纯的friend声明。

   结果: mdlog中有一个journaler成员，结果回调会调用到rmajournaler中的同名函数。

   A: 回调函数Context func_x改为类内声明后，问题解决