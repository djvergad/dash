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
 * Author: Dimitrios J. Vergados <djvergad@gmail.com>
 */

// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
#include <string>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dash-module.h"

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
    NodeContainer nodes;
    nodes.Create(3);

    NS_LOG_INFO("Create channels.");

    //
    // Explicitly create the point-to-point link required by the topology (shown above).
    //
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(linkRate));
    pointToPoint.SetChannelAttribute("Delay", StringValue(delay));
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
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    NS_LOG_INFO("Create Applications.");

    std::vector<std::string> protocols;
    std::stringstream ss(protocol); // ns3::DashClient
    std::string proto;
    uint32_t protoNum = 0; // The number of protocols (algorithms)


    while (std::getline(ss, proto, ',') && protoNum++ < users) {
        protocols.push_back(proto);
    }

    uint16_t port = 80;  // well-known echo port number
    std::vector<DashClientHelper> clients;
    std::vector<ApplicationContainer> clientApps;

    for (uint32_t user = 0; user < users; user++) {
        DashClientHelper client("ns3::TcpSocketFactory",
            InetSocketAddress(i.GetAddress(1), port), protocols[user % protoNum]);

        std::cout << "Address=" << i.GetAddress(1) << '\n';
        //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        client.SetAttribute("VideoId", UintegerValue(user + 1)); // VideoId should be positive
        client.SetAttribute("TargetDt", TimeValue(Seconds(target_dt)));
        client.SetAttribute("window", TimeValue(Time(window)));
        ApplicationContainer clientApp = client.Install(nodes.Get(0));
        clientApp.Start(Seconds(0.25));
        clientApp.Stop(Seconds(stopTime));

        clients.push_back(client);
        clientApps.push_back(clientApp);
    }

    // Building fog nodes connections
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get(1), TcpSocketFactory::GetTypeId ());

    DashServerHelper server("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer serverApps = server.Install(nodes.Get(1));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(stopTime + 5.0));

    // TODO
    // CacheServiceHelper cacheService("ns3::TcpSocketFactory",
    //     InetSocketAddress(Ipv4Address::GetAny(), port));
    // ApplicationContainer cacheApp = cacheService.Install(nodes.Get(2));
    // cacheApp.Start(Seconds(0.0));
    // cacheApp.Stop(Seconds(stopTime + 5.0));


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
