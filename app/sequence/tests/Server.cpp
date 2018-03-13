// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <tddl_sequence_SequenceServiceI.h>

using namespace std;

class Server : public Ice::Application
{
public:

    virtual int run(int argc, char* argv[]);
};

int
main(int argc, char* argv[])
{
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

    Ice::PropertiesPtr properties = communicator()->getProperties();
    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("tddl.sequences.SequenceService");
	tddl::sequences::SequenceServicePtr seqService = new tddl_sequence_SequenceServiceI();
    //Demo::PricingEnginePtr pricing = new PricingI(properties->getPropertyAsList("Currencies"));
    adapter->add(seqService, communicator()->stringToIdentity("tddl.sequences.SequenceService"));
    adapter->activate();
    communicator()->waitForShutdown();
    return EXIT_SUCCESS;
}