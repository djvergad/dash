/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 TEI of Western Macedonia, Greece
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Eduardo S. Gama <eduardogama72@gmail.com>
 */

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dash-module.h"


// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DashExample");

int main(int argc, char *argv[]) {

    bool tracing = false;
    uint32_t maxBytes = 100;
    uint32_t users = 1;
    double target_dt = 35.0;
    double stopTime = 100.0;
    std::string linkRate = "500Kbps";
    std::string delay = "5ms";
    std::string protocol = "ns3::DashClient";
    std::string window = "10s";


    // MpdFileHandler *mpd_instance = MpdFileHandler::getInstance();

  /*  LogComponentEnable("MpegPlayer", LOG_LEVEL_ALL);*/
//    LogComponentEnable ("DashServer", LOG_LEVEL_ALL);
//    LogComponentEnable ("DashClient", LOG_LEVEL_ALL);

    //
    // Allow the user to override any of the defaults at
    // run-time, via command-line arguments
    //
    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes", "Total number of bytes for application to send",
      maxBytes);
    cmd.AddValue("users", "The number of concurrent videos", users);
    cmd.AddValue("targetDt",
      "The target time difference between receiving and playing a frame.",
      target_dt);
    cmd.AddValue("stopTime",
      "The time when the clients will stop requesting segments", stopTime);
    cmd.AddValue("linkRate",
      "The bitrate of the link connecting the clients to the server (e.g. 500kbps)",
      linkRate);
    cmd.AddValue("delay",
      "The delay of the link connecting the clients to the server (e.g. 5ms)",
      delay);
    cmd.AddValue("protocol",
      "The adaptation protocol. It can be 'ns3::DashClient' or 'ns3::OsmpClient (for now).",
      protocol);
    cmd.AddValue("window",
      "The window for measuring the average throughput (Time).", window);
    cmd.Parse(argc, argv);

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create nodes.");

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(1);
    // node2.Add(node1.Get(1));
    // node2.Create(1);

    NS_LOG_INFO("Create channels.");

    //
    // Explicitly create the point-to-point link required by the topology (shown above).
    //
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(linkRate));
    pointToPoint.SetChannelAttribute("Delay", StringValue(delay));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install (csmaNodes);

    InternetStackHelper stack;
    stack.Install (p2pNodes.Get (0));
    stack.Install (csmaNodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign (p2pDevices);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign (csmaDevices);

    NS_LOG_INFO("Create Applications.");

    std::vector<std::string> protocols;
    std::stringstream ss(protocol); // ns3::DashClient
    std::string proto;
    uint32_t protoNum = 0; // The number of protocols (algorithms)


    while (std::getline(ss, proto, ',') && protoNum++ < users) {
        protocols.push_back(proto);
    }

    uint16_t port = 80;  // well-known echo port number
    // uint16_t port_client = 1000;
    std::vector<DashClientHelper> clients;
    std::vector<ApplicationContainer> clientApps;

    for (uint32_t user = 0; user < users; user++) {
        DashClientHelper client("ns3::TcpSocketFactory",
            InetSocketAddress(p2pInterfaces.GetAddress(0), port),
            InetSocketAddress(p2pInterfaces.GetAddress(1), port),
            protocols[user % protoNum]);

        std::cout << "Address=" << p2pInterfaces.GetAddress(1) << '\n';
        //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        client.SetAttribute("VideoId", UintegerValue(user + 1)); // VideoId should be positive
        client.SetAttribute("TargetDt", TimeValue(Seconds(target_dt)));
        client.SetAttribute("window", TimeValue(Time(window)));
        ApplicationContainer clientApp = client.Install(csmaNodes.Get(1));
        clientApp.Start(Seconds(0.25));
        clientApp.Stop(Seconds(stopTime));

        clients.push_back(client);
        clientApps.push_back(clientApp);
    }

    DashServerHelper server("ns3::TcpSocketFactory",
      InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer serverApps = server.Install(p2pNodes.Get(0));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(stopTime + 5.0));


    // Building fog nodes connections
    CacheServiceHelper cache("ns3::TcpSocketFactory",
        InetSocketAddress(p2pInterfaces.GetAddress(1), port),     // Fog node which will be a Socket server
        InetSocketAddress(p2pInterfaces.GetAddress(0), port));    // Fog node which will be a Socket client

    // Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId ());

    // Ptr<CacheService> app = CreateObject<CacheService>();
    // app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));

    ApplicationContainer cacheApp = cache.Install(p2pNodes.Get(1));
    cacheApp.Start(Seconds(0.0));
    cacheApp.Stop(Seconds(stopTime + 5.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    //
    // Set up tracing if enabled
    //
    if (tracing) {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll(ascii.CreateFileStream("dash-send.tr"));
        pointToPoint.EnablePcapAll("dash-send", false);
    }

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    /*Simulator::Stop(Seconds(100.0));*/
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    uint32_t k;
    for (k = 0; k < users; k++)
    {
        Ptr<DashClient> app = DynamicCast<DashClient>(clientApps[k].Get(0));
        std::cout << protocols[k % protoNum] << "-Node: " << k;
        app->GetStats();
    }

    return 0;

}
