// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <string>  
#include <Ice/Ice.h>
#include <IceBox/IceBox.h>
#include <Glacier2/PermissionsVerifier.h>
#include <Glacier2/Session.h>
#include <Glacier2/Router.h>
#include <IceSSL/Plugin.h>
#include "SessionServer.h"

using namespace std;
using namespace Ice;

class PermissionsVerifierI : public Glacier2::PermissionsVerifier
{
public:

    virtual bool
    checkPermissions(const string& userId, const string& pwd, string&, const Ice::Current& current) const
    {
        cout << "checkPermissions(" << userId << ","<< pwd <<")"<<endl;
        return true;
    }
};
class SessionI : public Glacier2::Session
{
public:

    SessionI(bool shutdown, bool ssl) : _shutdown(shutdown), _ssl(ssl)
    {
    }

    virtual void
    destroy(const Ice::Current& current)
    {
      cout << "SessionI->checkPermissidestroyons()"<<endl;

        current.adapter->remove(current.id);
        if(_shutdown)
        {
            current.adapter->getCommunicator()->shutdown();
        }
    }

    virtual void
    ice_ping(const Ice::Current& current) const
    {
        cout << "SessionI->ice_ping()"<<endl;
    }

private:

    const bool _shutdown;
    const bool _ssl;
};
class SessionManagerI : public Glacier2::SessionManager
{
public:

    virtual Glacier2::SessionPrx
    create(const string& userId, const Glacier2::SessionControlPrx&, const Ice::Current& current)
    {
        cout << "SessionManagerI->create(" << userId <<")"<<endl;
        Glacier2::SessionPtr session = new SessionI(false, userId == "ssl");
        return Glacier2::SessionPrx::uncheckedCast(current.adapter->addWithUUID(session));
    }
};
class SessionServer : public Ice::Application
{
public:

    virtual int run(int, char*[]);
};
int
SessionServer::run(int, char**)
{
    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints(
        "Glacier2.Server", "tcp -h 127.0.0.1 -p 12350");
    adapter->add(new PermissionsVerifierI, communicator()->stringToIdentity("PermissionsVerifier"));
    adapter->add(new SessionManagerI, communicator()->stringToIdentity("SessionManager"));
    adapter->activate();
    communicator()->waitForShutdown();
    return EXIT_SUCCESS;
}

class Server : public Ice::Application
{
public:

    virtual int run(int argc, char* argv[]);
};

int
main(int argc, char* argv[])
{
    /*
    Server app;
    int status = app.main(argc, argv);
    */
#ifdef ICE_STATIC_LIBS
    Ice::registerIceSSL();
#endif
    SessionServer app1;
    int status = app1.main(argc, argv);
    return status;
}

int
Server::run(int argc, char*[])
{
    if(argc > 1)
    {
        cerr << appName() << ": too many arguments" << endl;
        return EXIT_FAILURE;
    }
    //Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Server");
    //Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("tddl.sequences.SequenceService");
    SessionServerIceBox seqService;
    std::string name="Server";
    ::Ice::StringSeq ss;
    seqService.start(name,communicator(),ss);
    //Demo::PricingEnginePtr pricing = new PricingI(properties->getPropertyAsList("Currencies"));
    communicator()->waitForShutdown();
    return EXIT_SUCCESS;
}
