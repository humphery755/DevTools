// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#ifndef __OrderSequenceAdapterI_I_H
#define __OrderSequenceAdapterI_I_H

template<typename pthread_mutex_t>
class lock_guard
{
private:
    pthread_mutex_t *m;
  
    explicit lock_guard(lock_guard&);
    lock_guard& operator=(lock_guard&);
public:
    explicit lock_guard(pthread_mutex_t& m_): 
        m(&m_)  
    {
        pthread_mutex_lock(m);
    }
    ~lock_guard()  
    {
        pthread_mutex_unlock(m);  
    }
};
void startOrderSequence(const Ice::CommunicatorPtr& communicator, Ice::PropertiesPtr& prop,IceExt::IceClientUtil *clientUtil);
void stopOrderSequence();

#endif
