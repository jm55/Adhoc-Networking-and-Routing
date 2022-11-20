//NSCOM02 MCO2
//MEMBER: ESCALONA, JOSE MIGUEL

//THIS WAS BASED FROM NS3's examples/wireless/wifi-simple-adhoc-grid.cc

//
// This program configures a grid (default 5x5) of nodes on an
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n20  n21  n22  n23  n24
// n15  n16  n17  n18  n19
// n10  n11  n12  n13  n14
// n5   n6   n7   n8   n9
// n0   n1   n2   n3   n4
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./ns3 run "wifi-simple-adhoc-grid --help"
//

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND (": Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

double RandomValue(double max){
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute("Min", DoubleValue(0.0));
  x->SetAttribute("Max", DoubleValue(max));

  return x->GetValue();
}

//BASED FROM https://groups.google.com/g/ns-3-users/c/n1-N6zVlOLc/m/mvNV9lEwJy8J
Ptr<PositionAllocator> getPositionAlloc(uint32_t gridSize){
  ObjectFactory pos;
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute("Min", DoubleValue(0.0));
  x->SetAttribute("Max", DoubleValue(gridSize));

  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", x->GetValue ());
  pos.Set ("Y", x->GetValue());
  return pos.Create ()->GetObject<PositionAllocator> ();
}


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 25;  //MODIFIED TO SPEICIFIED TRANSMISSION RANGE OF 25m
  uint32_t packetSize = 512; //MODIFIED TO SPECIFIED PACKET SIZE OF 512Bytes
  uint32_t numPackets = 1;
  uint32_t numStaticNodes = 15;  //MODIFIED TO 15 STATIC NODES (ALBEIT NO INIDCATE WHETHER STATIC AND MOBILE)
  uint32_t numMobileNodes = 15;  //MODIFIED TO 15 MOBILE NODES (ALBEIT NO INIDCATE WHETHER STATIC AND MOBILE)
  uint32_t sinkNodeStatic = 0;
  uint32_t sourceNodeStatic = 14; //BASE 0 THUS RANGE IS 0 TO 14
  uint32_t sinkNodeMobile = 0;
  uint32_t sourceNodeMobile = 14; //BASE 0 THUS RANGE IS 0 TO 14
  double interval = 1.0; // In seconds
  double sim_time = 300; //SIMULATION TIME
  bool verbose = false;
  bool tracing = true;

  //SOME OF THE FLAGS WERE TEMPORARILY OMMITED TO REDUCE ISSUES WHEN DEBUGGING
  CommandLine cmd (__FILE__);
  //cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "Distance (m)", distance);
  cmd.AddValue ("packetSize", "Size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "Number of packets generated", numPackets);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "Turn on ASCII and PCAP tracing", tracing);
  cmd.AddValue ("numStaticNodes", "Number of Static Nodes", numStaticNodes);
  cmd.AddValue ("numMobileNodes", "Number of Mobile Nodes", numMobileNodes);
  cmd.AddValue ("sinkNodeStatic", "Static Receiver node number", sinkNodeStatic);
  cmd.AddValue ("sourceNodeStatic", "Static Sender node number", sourceNodeStatic);
  cmd.AddValue ("sinkNodeMobile", "Mobile Receiver node number", sinkNodeMobile);
  cmd.AddValue ("sourceNodeMobile", "Mobile Sender node number", sourceNodeMobile);
  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  //STATIC NODES
  NodeContainer staticNodes;
  staticNodes.Create (numStaticNodes);
  //MOBILE NODES
  NodeContainer mobileNodes;
  mobileNodes.Create(numMobileNodes);

  NS_LOG_UNCOND ("Static Node Count: " << staticNodes.GetN());
  NS_LOG_UNCOND ("Mobile Node Count: " << mobileNodes.GetN());

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  
  //VERBOSE
  if (verbose)
  {
    wifi.EnableLogComponents ();  // Turn on all Wifi logging
  }

  YansWifiPhyHelper wifiPhy;
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  //MODIFIED TO AGGREGATE STATIC AND MOBILE NETDEVICECONTAINERS INTO ONE 'devices' NETDEVICECONTAINER
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer static_Devices = wifi.Install (wifiPhy, wifiMac, staticNodes);
  NetDeviceContainer mobile_Devices = wifi.Install (wifiPhy, wifiMac, mobileNodes);
  NetDeviceContainer devices = static_Devices;
  devices.Add(mobile_Devices);

  NS_LOG_UNCOND ("Aggregated Devices: " << devices.GetN());

  //MOBILITY FOR STATIC NODES
  MobilityHelper static_mobility;
  static_mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (6), //MODIFIED GRIDWIDTH AS THEIR WILL BE 30 DEVICES INSTEAD OF THE ORIGINAL 25
                                 "LayoutType", StringValue ("RowFirst"));
  static_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  static_mobility.Install (staticNodes);
  NS_LOG_UNCOND ("Static Mobility Type: " << static_mobility.GetMobilityModelType());
  
  //MOBILITY FOR MOVING NODES
  MobilityHelper random_mobility;
  Ptr<PositionAllocator> positionAlloc = getPositionAlloc(30);
  random_mobility.SetMobilityModel (
    "ns3::RandomWaypointMobilityModel",
    "Speed", 
    RandomValue(300.0), 
    "Pause", 
    RandomValue(0)
  ); 
  random_mobility.SetPositionAllocator(positionAlloc);
  random_mobility.Install (mobileNodes);
  NS_LOG_UNCOND ("Mobile Mobility Type: " << random_mobility.GetMobilityModelType());

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (staticNodes); //UPDATED internet.Install TO ACCOMODATE SEPARATE STATIC NODES
  internet.Install (mobileNodes); //UPDATED internet.Install TO ACCOMODATE SEPARATE MOBILE NODES


  //PROTOCOL ALREADY SET TO USE TCP/IP
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  //TRACING
  if (tracing == true)
  {
    NS_LOG_UNCOND ("Tracing: " << tracing);
    AsciiTraceHelper ascii;
    NS_LOG_UNCOND ("Tracing: Enabling ASCIIAll...");
    wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
    NS_LOG_UNCOND ("Tracing: Enabling PCAP...");
    wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
    NS_LOG_UNCOND ("Tracing: Enabled ASCIIAll and PCAP!");
    // Trace routing tables
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
    olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
    Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
    olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

    // To do-- enable an IP-level trace that shows forwarding events only
  }

  //FOR STATIC NODES
  NS_LOG_UNCOND ("Setting Static Nodes recv/src...");
  TypeId tidStatic = TypeId::LookupByName ("ns3::TcpSocketFactory");
  Ptr<Socket> recvSinkStatic = Socket::CreateSocket (staticNodes.Get (sinkNodeStatic), tidStatic);
  InetSocketAddress localStatic = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSinkStatic ->Bind (localStatic);
  recvSinkStatic ->SetRecvCallback (MakeCallback (&ReceivePacket));
  Ptr<Socket> sourceStatic = Socket::CreateSocket (staticNodes.Get(sourceNodeStatic), tidStatic); //CRASHES HERE
  InetSocketAddress remoteStatic = InetSocketAddress (i.GetAddress (sinkNodeStatic, 0), 80);
  sourceStatic->Connect (remoteStatic);
  NS_LOG_UNCOND ("Setting Static Nodes recv/src Complete!");  

  //FOR MOBILE NODES
  NS_LOG_UNCOND ("Setting Mobile Nodes recv/src...");
  TypeId tidMobile = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSinkMobile = Socket::CreateSocket (mobileNodes.Get (sinkNodeMobile), tidMobile);
  InetSocketAddress localMobile = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSinkMobile ->Bind (localMobile);
  recvSinkMobile ->SetRecvCallback (MakeCallback (&ReceivePacket));
  Ptr<Socket> sourceMobile = Socket::CreateSocket (mobileNodes.Get (sourceNodeMobile), tidMobile);
  InetSocketAddress remoteMobile = InetSocketAddress (i.GetAddress (sinkNodeMobile, 0), 80);
  sourceMobile->Connect (remoteMobile);
  NS_LOG_UNCOND ("Setting Mobile Nodes recv/src Complete!");

  // Give OLSR time to converge-- 30 seconds perhaps
  NS_LOG_UNCOND ("Setting Simulator OLSR Convergence for Static Nodes...");
  Simulator::Schedule (Seconds (15.0), &GenerateTraffic,
                       sourceStatic, packetSize, numPackets, interPacketInterval);
  NS_LOG_UNCOND ("Setting Simulator OLSR Convergence for Mobile Nodes...");
  Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
                       sourceMobile, packetSize, numPackets, interPacketInterval);

  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNodeStatic << " to " << sinkNodeStatic << " with grid distance " << distance);
  NS_LOG_UNCOND ("Testing from node " << sourceNodeMobile << " to " << sinkNodeMobile << " with grid distance " << distance);

  Simulator::Stop (Seconds (sim_time));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

