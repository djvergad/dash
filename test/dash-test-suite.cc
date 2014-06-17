/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "ns3/dash-module.h"

// An essential include is test.h
#include "ns3/test.h"

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dash-module.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// This is an example TestCase.
class DashTestCase1 : public TestCase
{
public:
  DashTestCase1 ();
  virtual ~DashTestCase1 ();

private:
  virtual void DoRun (void);
};

// Add some help text to this case to describe what it is intended to test
DashTestCase1::DashTestCase1 ()
  : TestCase ("Dash test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
DashTestCase1::~DashTestCase1 ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
DashTestCase1::DoRun (void)
{

  bool tracing = false;
  uint32_t users = 1;
  double target_dt = 7;
  double stopTime = 100.0;

//
// Explicitly create the nodes required by the topology (shown above).
//
  NodeContainer nodes;
  nodes.Create(2);
//
// Explicitly create the point-to-point link required by the topology (shown above).
//
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("500Kbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("5ms"));
  NetDeviceContainer devices;
  devices = pointToPoint.Install(nodes);

//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install(nodes);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);
//
// Create a BulkSendApplication and install it on node 0
//
  uint16_t port = 80;  // well-known echo port number

  std::vector<DashClientHelper> clients;
  std::vector<ApplicationContainer> clientApps;

  for (uint32_t user = 1; user <= users; user++)
    {
      DashClientHelper client("ns3::TcpSocketFactory",
          InetSocketAddress(i.GetAddress(1), port));
      //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
      client.SetAttribute("VideoId", UintegerValue(user));
      client.SetAttribute("window", TimeValue(Seconds(target_dt)));
      ApplicationContainer clientApp = client.Install(nodes.Get(0));
      clientApp.Start(Seconds(1.0));
      clientApp.Stop(Seconds(stopTime));

      clients.push_back(client);
      clientApps.push_back(clientApp);

    }

  DashServerHelper server("ns3::TcpSocketFactory",
      InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer serverApps = server.Install(nodes.Get(1));
  serverApps.Start(Seconds(0.0));
  serverApps.Stop(Seconds(stopTime + 5.0));

//
// Set up tracing if enabled
//
  if (tracing)
    {
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll(ascii.CreateFileStream("dash-send.tr"));
      pointToPoint.EnablePcapAll("dash-send", false);
    }

//
// Now, do the actual simulation.
//
  /*Simulator::Stop(Seconds(100.0));*/
  Simulator::Run();
  Simulator::Destroy();

  uint32_t k;
  for (k = 0; k < users; k++)
    {
      Ptr<DashClient> app = DynamicCast<DashClient>(clientApps[k].Get(0));
      std::cout << "Node: " << k;
      app->GetStats();
    }

  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
  // Use this one for floating point comparisons
  NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class DashTestSuite : public TestSuite
{
public:
  DashTestSuite ();
};

DashTestSuite::DashTestSuite ()
  : TestSuite ("dash", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new DashTestCase1, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static DashTestSuite dashTestSuite;

