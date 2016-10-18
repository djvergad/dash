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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/dash-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("Dash-Wifi");

int
main(int argc, char *argv[])
{
  uint32_t nWifi = 8;
  uint32_t maxBytes = 0;
  uint32_t users = 1;
  uint32_t bulkNo = 1;
  double target_dt = 35.0;
  double stopTime = 100.0;
  std::string linkRate = "500Kbps";
  std::string delay = "5ms";
  std::string protocol = "ns3::DashClient";
  std::string window = "10s";

  /*LogComponentEnable ("DashServer", LOG_LEVEL_ALL);
   LogComponentEnable ("DashClient", LOG_LEVEL_ALL);*/

  //
  // Allow the user to override any of the defaults at
  // run-time, via command-line arguments
  //
  CommandLine cmd;
  cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue("maxBytes", "Total number of bytes for application to send",
      maxBytes);
  cmd.AddValue("users", "The number of concurrent videos", users);
  cmd.AddValue("bulkNo", "The number of background TCP transfers", bulkNo);
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

  if (bulkNo + users > nWifi + 1)
    {
      std::cerr << "you need more stations" << std::endl;
      return -1;
    }

  std::cout << "nWifi= " << nWifi << std::endl;

  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(nWifi);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(channel.Create());

  //WifiHelper wifi = WifiHelper::Default();//deprecated
  WifiHelper wifi = WifiHelper();
  wifi.SetRemoteStationManager("ns3::AarfWifiManager");
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default();

  NetDeviceContainer staDevices;
  staDevices = wifi.Install(phy, mac, wifiStaNodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator", "MinX",
      DoubleValue(0.0), "MinY", DoubleValue(0.0), "DeltaX", DoubleValue(40.0),
      "DeltaY", DoubleValue(50.0), "GridWidth", UintegerValue(2), "LayoutType",
      StringValue("RowFirst"));

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiStaNodes);

  InternetStackHelper stack;
  stack.Install(wifiStaNodes);
  Ipv4AddressHelper address;
  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign(staDevices);


  std::vector<std::string> protocols;
  std::stringstream ss(protocol);
  std::string proto;
  uint32_t protoNum = 0; // The number of protocols (algorithms)
  while (std::getline(ss, proto, ',') && protoNum++ < users)
    {
      protocols.push_back(proto);
    }

  uint16_t port = 80;  // well-known echo port number

  std::vector<DashClientHelper> clients;
  std::vector<ApplicationContainer> clientApps;

  for (uint32_t user = 0; user < users; user++)
    {
      DashClientHelper client("ns3::TcpSocketFactory",
          InetSocketAddress(interfaces.GetAddress(0), port),protocols[user % protoNum]);
      //client.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
      client.SetAttribute("VideoId", UintegerValue(user + 1)); // VideoId should be positive
      client.SetAttribute("TargetDt", TimeValue(Seconds(target_dt)));
      client.SetAttribute("window", TimeValue(Time(window)));
      ApplicationContainer clientApp = client.Install(wifiStaNodes.Get(user + 1)); // Node 0 is the server
      clientApp.Start(Seconds(0.25));
      clientApp.Stop(Seconds(stopTime));

      clients.push_back(client);
      clientApps.push_back(clientApp);

    }

  DashServerHelper server("ns3::TcpSocketFactory",
      InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer serverApps = server.Install(wifiStaNodes.Get(0));
  serverApps.Start(Seconds(0.0));
  serverApps.Stop(Seconds(stopTime + 5.0));

  for (uint32_t bulk = 0; bulk < bulkNo; bulk++)
    {
      BulkSendHelper source("ns3::TcpSocketFactory",
          InetSocketAddress(interfaces.GetAddress(1 + users + bulk), port));

      source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
      ApplicationContainer sourceApps = source.Install(wifiStaNodes.Get(0));
      sourceApps.Start(Seconds(0.0));
      sourceApps.Stop(Seconds(stopTime));

      PacketSinkHelper sink("ns3::TcpSocketFactory",
          InetSocketAddress(Ipv4Address::GetAny(), port));
      ApplicationContainer sinkApps = sink.Install(
          wifiStaNodes.Get(1 + users + bulk));
      sinkApps.Start(Seconds(0.0));
      sinkApps.Stop(Seconds(stopTime));

    }

  /*    UdpServerHelper server (22000);
   ApplicationContainer serverApps = server.Install(wifiStaNodes.Get(1));
   serverApps.Start(Seconds(1.0));
   serverApps.Stop(Seconds(10.0));

   UdpClientHelper client (interfaces.GetAddress (1), 22000);
   client.SetAttribute("Interval", TimeValue(Seconds(0.01)));
   client.SetAttribute("PacketSize", UintegerValue(512));
   client.SetAttribute("MaxPackets", UintegerValue (15000));
   ApplicationContainer clientApps = client.Install(wifiStaNodes.Get(4));
   clientApps.Start(Seconds(1.5));
   clientApps.Stop(Seconds(9.5));*/

  Simulator::Stop(Seconds(stopTime));
  AnimationInterface anim("dash-wifi.xml");
  Simulator::Run();
  Simulator::Destroy();

  uint32_t k;
  for (k = 0; k < users; k++)
    {
      Ptr<DashClient> app = DynamicCast<DashClient>(clientApps[k].Get(0));
      std::cout << protocols[k % protoNum] << "-Node: " << k;
      app->GetStats();
    }

  return 0;
}
