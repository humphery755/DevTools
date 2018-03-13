// **********************************************************************
//
// Copyright (c) 2003-2006 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution. 
//
// **********************************************************************

#pragma once

#include <Ice/BuiltinSequences.ice>

module Test
{

exception CallbackException
{
    string reason;
};

interface CallbackReceiver
{
    void callbackEx()
        throws CallbackException;

    void callbackWithPayload(Ice::ByteSeq payload);
};

interface Callback
{
    ["amd"] void initiateCallbackEx(CallbackReceiver* proxy)
        throws CallbackException;

    ["amd"] void initiateCallbackWithPayload(CallbackReceiver* proxy);

};

};