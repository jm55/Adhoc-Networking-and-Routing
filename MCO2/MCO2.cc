//NSCOM02 MCO2
//MEMBER: ESCALONA, JOSE MIGUEL

//THIS WAS BASED FROM NS3's:
//  1. examples/wireless/wifi-simple-adhoc-grid.cc
//  2. examples/routing/manet-routing-compare.cc

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
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

void ReceivePacket (Ptr<Socket> socket){
  while (socket->Recv ()){
    NS_LOG_UNCOND (socket->GetNode()->GetId() << " Received one packet!");
  }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval ){
  NS_LOG_UNCOND ("Generating traffic...");
  if (pktCount > 0){
    NS_LOG_UNCOND ("Socket: " << socket->GetNode()->GetId() << " sending " << pktSize << " bytes of packet");
    socket->Send (Create<Packet> (pktSize));
    Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,pktCount - 1, pktInterval);
  }
  else{
    socket->Close ();
  }
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
  double interval = 1.0; //IN SECONDS
  double sim_time = 300; //SIMULATION TIME
  double movementSpeed = 300;
  double pauseSpeed = 0;
  bool verbose = false;
  bool tracing = true;


  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("NSCOM02 S11");
  NS_LOG_UNCOND ("MCO2 PROJECT");
  NS_LOG_UNCOND ("Member: ESCALONA, J.M.");
  NS_LOG_UNCOND ("The code was based from: ");
  NS_LOG_UNCOND ("1. examples/wireless/wifi-simple-adhoc-grid.cc");
  NS_LOG_UNCOND ("2. examples/routing/manet-routing-compare.cc");
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("");

  NS_LOG_UNCOND ("=============================================="); 
  NS_LOG_UNCOND ("SPECIFICATION:");
  NS_LOG_UNCOND ("");
  NS_LOG_UNCOND ("Data Flow: Messages");
  NS_LOG_UNCOND ("Traffic Pattern: TCP");
  NS_LOG_UNCOND ("Simulation Time: " << sim_time << "s");
  NS_LOG_UNCOND ("Transmission Range: " << distance << "m");
  NS_LOG_UNCOND ("Protocol Stack: TCP/IP");
  NS_LOG_UNCOND ("Medium: 802.11");
  NS_LOG_UNCOND ("Packet Size: "  << packetSize << " bytes");
  NS_LOG_UNCOND ("Mobility Model: Static & RandomWay Point");
  NS_LOG_UNCOND ("No of Nodes: " << numStaticNodes << " static & " << numMobileNodes << " mobile");
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("");

  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("BUILDING CONFIGURATIONS...");
  NS_LOG_UNCOND ("==============================================");

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

  // CONVERT TO TIME OBJECT
  Time interPacketInterval = Seconds (interval);

  // FIX NON-UNICAST DATA RATE TO BE THE SAME AS THAT OF UNICAST
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

  // THE BELOW SET OF HELPERS WILL HELP US TO PUT TOGETHER THE WIFI NICS WE WANT
  WifiHelper wifi;
  
  //VERBOSE
  if (verbose)
  {
    wifi.EnableLogComponents ();  // TURN ON ALL WIFI LOGGING
  }

  YansWifiPhyHelper wifiPhy;
  // SET IT TO ZERO; OTHERWISE, GAIN WILL BE ADDED
  wifiPhy.Set ("RxGain", DoubleValue (-10) );
  // NS-3 SUPPORTS RADIOTAP AND PRISM TRACING EXTENSIONS FOR 802.11B
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // ADD AN UPPER MAC AND DISABLE RATE CONTROL
  WifiMacHelper wifiMac;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // SET IT TO ADHOC MODE
  //MODIFIED TO AGGREGATE STATIC AND MOBILE NETDEVICECONTAINERS INTO ONE 'devices' NETDEVICECONTAINER
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer static_Devices = wifi.Install (wifiPhy, wifiMac, staticNodes);
  NetDeviceContainer mobile_Devices = wifi.Install (wifiPhy, wifiMac, mobileNodes);
  NetDeviceContainer devices = static_Devices;
  devices.Add(mobile_Devices);

  NS_LOG_UNCOND ("Aggregated Devices: " << devices.GetN());

  //MOBILITY FOR STATIC NODES
  NS_LOG_UNCOND ("Setting Static Mobility...");
  MobilityHelper static_mobility;
  static_mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (30), //MODIFIED GRIDWIDTH AS THEIR WILL BE 30 DEVICES INSTEAD OF THE ORIGINAL 25
                                 "LayoutType", StringValue ("RowFirst"));
  static_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  static_mobility.Install (staticNodes);
  NS_LOG_UNCOND ("Static Mobility Set!");
  NS_LOG_UNCOND ("Static Mobility Type: " << static_mobility.GetMobilityModelType());
  
  //MOBILITY FOR MOVING NODES (INITIAL)
  NS_LOG_UNCOND ("Setting Mobile Mobility (Initial)...");
  MobilityHelper random_mobility;
  random_mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (30), //MODIFIED GRIDWIDTH AS THEIR WILL BE 30 DEVICES INSTEAD OF THE ORIGINAL 25
                                 "LayoutType", StringValue ("RowFirst"));
  NS_LOG_UNCOND ("Mobile Mobility (Initial) Set!");

  //POSITIONALLOCATOR FOR MOBILE FINAL DESTINATION
  NS_LOG_UNCOND ("Building PositionAllocator...");
  
  int64_t streamIndex = 0;
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  
  std::stringstream setX;
  std::stringstream setY;
  setX << "ns3::UniformRandomVariable[Min=0.0|Max=" << movementSpeed << "]";
  setY << "ns3::UniformRandomVariable[Min=0.0|Max=" << movementSpeed << "]";
  pos.Set ("X", StringValue (setX.str()));
  pos.Set ("Y", StringValue (setY.str()));

  Ptr<PositionAllocator> mobilePosAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  streamIndex += mobilePosAlloc->AssignStreams (streamIndex);
  
  NS_LOG_UNCOND ("PositionAllocator Built!");

  //MOBILITY FOR MOVING NODES (FINAL DESTINATION)
  NS_LOG_UNCOND ("Setting Mobile Mobility (Final Destination)...");
  
  std::stringstream speed; //MOVEMENT SPEED
  std::stringstream pause; //PAUSE MOVEMENT SPEED
  speed << "ns3::UniformRandomVariable[Min=0.0|Max=" << movementSpeed << "]";
  pause << "ns3::ConstantRandomVariable[Constant=" << pauseSpeed << "]";
  
  random_mobility.SetMobilityModel (
    "ns3::RandomWaypointMobilityModel",
    "Speed", 
    StringValue(speed.str()), 
    "Pause", 
    StringValue(pause.str()),
    "PositionAllocator",
    PointerValue(mobilePosAlloc)
  );
  random_mobility.Install (mobileNodes);
  
  NS_LOG_UNCOND ("Mobile Mobility (Final Destination) Set!");
  NS_LOG_UNCOND ("Mobile Mobility Type: " << random_mobility.GetMobilityModelType());
  NS_LOG_UNCOND ("Mobile Mobility Speed/Pause: " << movementSpeed << "/" << pauseSpeed);

  //ENABLE OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // HAS EFFECT ON THE NEXT Install()
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
    // TRACE ROUTING TABLES
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
    olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
    Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
    olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

    // TO DO-- ENABLE AN IP-LEVEL TRACE THAT SHOWS FORWARDING EVENTS ONLY
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

  // GIVE OLSR TIME TO CONVERGE-- 30 SECONDS PERHAPS
  NS_LOG_UNCOND ("Setting Simulator OLSR Convergence for Static Nodes...");
  Simulator::Schedule (Seconds (15.0), &GenerateTraffic,
                       sourceStatic, packetSize, numPackets, interPacketInterval);
  NS_LOG_UNCOND ("Setting Simulator OLSR Convergence for Mobile Nodes...");
  Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
                       sourceMobile, packetSize, numPackets, interPacketInterval);

  // OUTPUT WHAT WE ARE DOING
  NS_LOG_UNCOND ("Testing from node " << sourceNodeStatic << " to " << sinkNodeStatic << " with grid distance " << distance);
  NS_LOG_UNCOND ("Testing from node " << sourceNodeMobile << " to " << sinkNodeMobile << " with grid distance " << distance);

  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("RUNNING SIMULATOR...");
  NS_LOG_UNCOND ("==============================================");

  NS_LOG_UNCOND ("Running Simulator (" << sim_time << " seconds)...");
  Simulator::Stop (Seconds (sim_time));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}