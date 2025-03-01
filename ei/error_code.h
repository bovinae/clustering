#ifndef _ERROR_CODE_H
#define _ERROR_CODE_H

// basic error code
enum errorcode{
    E_OK,             ///< 成功
    E_FAIL,           ///< 一般性错误
    E_INNER,          ///< 内部错误（一般在同一个模块内使用，不对外公开
    E_POINTER,        ///< 非法指针
    E_INVALARG,       ///< 非法参数
    E_NOTIMPL,        ///< 功能未实现
    E_OUTOFMEM,       ///< 内存申请失败/内存不足
    E_BUFERROR,       ///< 缓冲区错误（不足，错乱）
    E_PERM,           ///< 权限不足，访问未授权的对象
    E_TIMEOUT,        ///< 超时
    E_NOTINIT,        ///< 未初始化
    E_INITFAIL,       ///< 初始化失败
    E_ALREADY,        ///< 已初始化，已经在运行
    E_INPROGRESS,     ///< 已在运行、进行状态
    E_EXIST,          ///< 申请资源对象(如文件或目录)已存在
    E_NOTEXIST,       ///< 资源对象(如文件或目录)、命令、设备等不存在
    E_BUSY,           ///< 设备或资源忙（资源不可用）
    E_FULL,           ///< 设备/资源已满
    E_EMPTY,          ///< 对象/内存/内容为空
    E_OPENFAIL,       ///< 资源对象(如文件或目录、socket)打开失败
    E_READFAIL,       ///< 资源对象(如文件或目录、socket)读取、接收失败
    E_WRITEFAIL,      ///< 资源对象(如文件或目录、socket)写入、发送失败
    E_DELFAIL,        ///< 资源对象(如文件或目录、socket)删除、关闭失败
    E_CODECFAIL,      ///< 加解密、编码解密失败
    E_CRC_FAIL,       ///< crc校验错误
    E_TOOMANY,        ///< 消息、缓冲区、内容太多
    E_TOOSMALL,       ///< 消息、缓冲区、内容太少
    E_NETNOTREACH,    ///< 网络不可达（无路由，网关错误）
    E_NETDOWN,        ///< 网络不可用（断网）

    // business error code

    E_END,            ///< 占位，无实际作用
};

const char* get_errorcode(int ec);

#endif
