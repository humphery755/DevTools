// **********************************************************************
//
// Copyright (c) 2003-2006 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#pragma once

//[["java:package:com.taobao.tddl.client"]]
module tddl{
module sequences
{

exception SequenceException{
	string reason;
};

struct SequenceRange{
	long min;
	long max;
	//long min2;
	//long max2;
};

interface SequenceService {

	 /**
	 * 取得序列下一个值
	 * @param step  序列名称
	 * @param step 步长
	 * @return 返回序列下一个值
	 * @throws SequenceException
	 */
	SequenceRange nextValue(string name,int step) throws SequenceException;
};

};
};