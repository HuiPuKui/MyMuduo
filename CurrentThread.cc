#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            // ���� Linux ϵͳ���û�ȡ��ǰ�̵߳� tid ֵ
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}