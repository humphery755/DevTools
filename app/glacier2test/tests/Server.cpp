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

// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <glog/logging.h>
#include "Toolkits.h"

using namespace std;


std::string
getTestEndpoint(const Ice::PropertiesPtr& properties, int num, const std::string& prot)
{
    std::ostringstream ostr;
    std::string protocol = prot;
    if(protocol.empty())
    {
        protocol = properties->getPropertyWithDefault("Ice.Default.Protocol", "default");
    }

    int basePort = properties->getPropertyAsIntWithDefault("Test.BasePort", 12010);

    if(protocol == "bt")
    {
        //
        // For Bluetooth, there's no need to specify a port (channel) number.
        // The client locates the server using its address and a UUID.
        //
        switch(num)
        {
        case 0:
            ostr << "default -u 5e08f4de-5015-4507-abe1-a7807002db3d";
            break;
        case 1:
            ostr << "default -u dae56460-2485-46fd-a3ca-8b730e1e868b";
            break;
        case 2:
            ostr << "default -u 99e08bc6-fcda-4758-afd0-a8c00655c999";
            break;
        default:
            assert(false);
        }
    }
    else
    {
        ostr << protocol << " -p " << (basePort + num);
    }
    return ostr.str();
}
class Server : public Ice::Application
{
public:

    virtual int run(int argc, char* argv[]);
};

int
main(int argc, char* argv[])
{
//#ifdef ICE_STATIC_LIBS
    Ice::registerIceSSL(false);
    Ice::registerIceWS(true);
//#endif
    Server app;
    int status = app.main(argc, argv);
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

	//if(!startTookits((void*)0))return 1;
    std::string endpoint =  getTestEndpoint(communicator()->getProperties(), 1,"");
    cout << endpoint <<endl;
    communicator()->getProperties()->setProperty("Glacier2.Server.Endpoints", endpoint);
	SessionServerIceBox seqService;
    std::string name="Server";
    ::Ice::StringSeq ss;
    seqService.start(name,communicator(),ss);
    //Demo::PricingEnginePtr pricing = new PricingI(properties->getPropertyAsList("Currencies"));
    communicator()->waitForShutdown();
    return EXIT_SUCCESS;
}
