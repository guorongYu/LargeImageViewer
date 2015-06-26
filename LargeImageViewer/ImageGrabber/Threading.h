#ifndef _VPL_THREADING_H_
#define _VPL_THREADING_H_

namespace vee {

typedef struct DummyMutex
{
    __forceinline void lock()
    {
        // Nothing to do.
    }
    __forceinline void unlock()
    {
        // Nothing to do.
    }
    __forceinline void try_lock()
    {
        // Nothing to do.
    }
} NOTHREADING;

template <class MutexTy>
class LockGuard
{
public:
    typedef MutexTy mutex_type;

    explicit LockGuard(MutexTy& m): _RefMutex(m)
    {
        _RefMutex.lock();
    }
    ~LockGuard() _NOEXCEPT
    {
        _RefMutex.unlock();
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
private:
    MutexTy& _RefMutex;
};

} // namespace vee

#endif // _VPL_THREADING_H_