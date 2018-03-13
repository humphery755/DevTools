// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <Freeze/Freeze.h>
#include <StringSeqRangeMap.h>

using namespace std;
using namespace tddl::sequences;
using namespace Freeze;

class Create : public Ice::Application
{
public:
    
    virtual int run(int, char*[]);
};

int main(int argc, char* argv[])
{
    Create app;
    return app.main(argc, argv);
}

int
Create::run(int, char*[])
{
    const string names[] = { "don", "ed", "frank", "gary", "arnold", "bob", "carlos" };

    const size_t size = 7;


    ConnectionPtr connection = createConnection(communicator(), "db");
    StringSeqRangeMap contacts(connection, "StringSeqRangeMap");
    
  
    //
    // Create a bunch of contacts within one transaction, and commit it
    //
  
    TransactionHolder txh(connection);
    for(size_t i = 0; i < size; ++i)
    {
        SequenceRange data;

        contacts.put(StringSeqRangeMap::value_type(names[i], data));
    }
    
    txh.commit();
            
    cout << size << " contacts were successfully created or updated" << endl;
    
    return 0;
}
