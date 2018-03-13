
#ifndef __SessionServer_h__
#define __SessionServer_h__

class SessionServerIceBox: public ::IceBox::Service
{
public:	
	virtual void start(const ::std::string&,		const ::Ice::CommunicatorPtr&,		const ::Ice::StringSeq&);
	virtual void stop();
private:
	::Ice::ObjectAdapterPtr _adapter;
	::Ice::ObjectAdapterPtr _adapter2;
	::Ice::ObjectAdapterPtr _adapter3;
	::Ice::ObjectAdapterPtr _adapter4;
	Ice::CommunicatorPtr   _communicator;
};

#endif
