//NSCOM02 MCO2
//MEMBER: ESCALONA, JOSE MIGUEL & REINANTE, CHRISTIAN VICTOR

//THIS WAS BASED FROM NS3's:
//  1. examples/wireless/wifi-simple-adhoc-grid.cc
//  2. examples/routing/manet-routing-compare.cc

#include <cstdlib>
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
#include "ns3/point-to-point-module.h"
#include "ns3/packet-sink.h"
#include "ns3/netanim-module.h" //NEEDED FOR NETANIM

using namespace ns3;

uint32_t msgCounter = 1; //FOR COUNTING MESSAGES

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

void ReceivePacket (Ptr<Socket> socket){
  while (socket->Recv ()){
    NS_LOG_UNCOND ("Node " << socket->GetNode()->GetId() << " received one packet!");
    NS_LOG_UNCOND ("");
  }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval ){
  if (pktCount > 0){
    NS_LOG_UNCOND ("Node " << socket->GetNode()->GetId() << " sending " << pktSize << "bytes (Message " << msgCounter << ")");
    std::stringstream data;
    data << "Hello this is Node " << socket->GetNode()->GetId() << " sending you a packet of size " << pktSize << "bytes! (Message " << msgCounter << ")";
    Ptr<Packet> packet = Create<Packet>((uint8_t*) data.str().c_str(), pktSize);
    socket->Send (packet);
    Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,pktCount - 1, pktInterval);
    msgCounter++;
  }
  else{
    NS_LOG_UNCOND ("Closing socket (Node " << socket->GetNode()->GetId() << ")...");
    socket->Close ();
    NS_LOG_UNCOND ("Socket closed!");
    msgCounter = 0;
  }
}

int main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("NSCOM02 S11");
  NS_LOG_UNCOND ("MCO2 PROJECT");
  NS_LOG_UNCOND ("Member: ESCALONA, J.M. & Reinante I.");
  NS_LOG_UNCOND ("The code was based from: ");
  NS_LOG_UNCOND ("1. examples/wireless/wifi-simple-adhoc-grid.cc");
  NS_LOG_UNCOND ("2. examples/routing/manet-routing-compare.cc");
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("");
  
  std::string phyMode ("DsssRate11Mbps"); //DsssRate11Mbps
  uint32_t deviceCount = 30;
  double distance = 25;  //MODIFIED TO SPEICIFIED TRANSMISSION RANGE OF 25m
  uint32_t packetSize = 512; //MODIFIED TO SPECIFIED PACKET SIZE OF 512Bytes
  uint32_t numPackets = 1; //NUMBER OF PACKETS
  uint32_t numStaticNodes = 15;  //MODIFIED TO 15 STATIC NODES (ALBEIT NO INIDCATE WHETHER STATIC AND MOBILE)
  uint32_t numMobileNodes = 15;  //MODIFIED TO 15 MOBILE NODES (ALBEIT NO INIDCATE WHETHER STATIC AND MOBILE)
  uint32_t sourceNode = 0; //BASE 0 THUS RANGE IS 0 TO 29
  uint32_t sinkNode = 29;
  double interval = 1.0; //IN SECONDS
  double sim_time = 300; //SIMULATION TIME (S)
  double movementSpeed = 1.0; //IN M/S?
  double pauseSpeed = 0;
  bool verbose = false;
  bool tracing = true;
  double convergence = 15.0; //TIME FOR OLSR TO CONVERGE
  std::string title = "MCO2-Escalona_Reinante";

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
  cmd.AddValue ("sinkNode", "Receiver node number (0-29)", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number (0-29)", sourceNode);
  cmd.AddValue ("movmentSpeed", "Movement speed of mobile nodes", movementSpeed);
  cmd.AddValue ("sim_time", "Simulation Time", sim_time);
  cmd.AddValue ("convergence", "OLSR Convergence Time", convergence);
  cmd.Parse (argc, argv);

  NS_LOG_UNCOND ("=============================================="); 
  NS_LOG_UNCOND ("SPECIFICATION:");
  NS_LOG_UNCOND ("");
  NS_LOG_UNCOND ("Data Flow: Messages");
  NS_LOG_UNCOND ("Traffic Pattern: UDP");
  NS_LOG_UNCOND ("Simulation Time: " << sim_time << "s");
  NS_LOG_UNCOND ("Transmission Range: " << distance << "m");
  NS_LOG_UNCOND ("Protocol Stack: UDP/IP");
  NS_LOG_UNCOND ("Medium: 802.11");
  NS_LOG_UNCOND ("Packet Size: "  << packetSize << " bytes");
  NS_LOG_UNCOND ("Mobility Model: Static & RandomWay Point");
  NS_LOG_UNCOND ("No of Nodes: " << numStaticNodes << " static & " << numMobileNodes << " mobile");
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("");

  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("SETUP");
  NS_LOG_UNCOND ("Source/Sink Range: 0-14 for Static Nodes and 15-29 for Moving Nodes");
  NS_LOG_UNCOND ("SourceNode: " << sourceNode);
  NS_LOG_UNCOND ("SinkNode: " << sinkNode);
  NS_LOG_UNCOND ("Mobile Node Movement Speed: " << movementSpeed);
  NS_LOG_UNCOND ("Sim_Time: " << sim_time);
  NS_LOG_UNCOND ("Interval: " << interval);
  NS_LOG_UNCOND ("Number of Packets to be sent: " << numPackets);
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("");

  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("BUILDING CONFIGURATIONS...");
  NS_LOG_UNCOND ("==============================================");

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
                                 "GridWidth", UintegerValue (distance), //MODIFIED GRIDWIDTH AS THEIR WILL BE 30 DEVICES INSTEAD OF THE ORIGINAL 25
                                 "LayoutType", StringValue ("RowFirst"));
  static_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  static_mobility.Install (staticNodes);
  NS_LOG_UNCOND ("Static Mobility Set!");
  NS_LOG_UNCOND ("Static Mobility Set (" << static_mobility.GetMobilityModelType() << ")");
  
  //MOBILITY FOR MOVING NODES (INITIAL POSITION)
  NS_LOG_UNCOND ("Setting Mobile Mobility (Initial)...");
  MobilityHelper random_mobility;
  random_mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (1),
                                 "DeltaY", DoubleValue (1),
                                 "GridWidth", UintegerValue (distance), //MODIFIED GRIDWIDTH AS THEIR WILL BE 30 DEVICES INSTEAD OF THE ORIGINAL 25
                                 "LayoutType", StringValue ("RowFirst")); //Sets in Row
  std::stringstream speed;
  speed << "ns3::ConstantRandomVariable[Constant=" << movementSpeed << "]";
  random_mobility.SetMobilityModel (
    "ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (0,distance,0,distance)),
    //"Mode", StringValue ("Time"),
    //"Time", StringValue (std::to_string(sim_time)+"s"),
    "Speed", StringValue(speed.str())
  );
  random_mobility.Install (mobileNodes);
  
  NS_LOG_UNCOND ("Mobile Mobility (Final Destination) Set!");
  NS_LOG_UNCOND ("Mobile Mobility Type: " << random_mobility.GetMobilityModelType());
  NS_LOG_UNCOND ("Mobile Mobility Speed/Pause: " << movementSpeed << "/" << pauseSpeed);

  //COMBINE NODES
  NS_LOG_UNCOND ("Combining nodes...");
  NodeContainer combinedNodes;
  combinedNodes.Add(staticNodes);
  combinedNodes.Add(mobileNodes);
  NS_LOG_UNCOND ("Nodes Combined! (Count: " << combinedNodes.GetN() << ")");

  //ENABLE OLSR
  NS_LOG_UNCOND ("Setting OLSR...");
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, deviceCount);
  NS_LOG_UNCOND ("OSLR set!");

  NS_LOG_UNCOND ("Setting Internet...");
  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // HAS EFFECT ON THE NEXT Install()
  internet.Install (combinedNodes); //UPDATED internet.Install
  NS_LOG_UNCOND ("Internet set!");

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
    wifiPhy.EnableAsciiAll (ascii.CreateFileStream (title + ".tr"));
    NS_LOG_UNCOND ("Tracing: Enabling PCAP...");
    wifiPhy.EnablePcap (title, devices);
    NS_LOG_UNCOND ("Tracing: Enabled ASCIIAll and PCAP!");
    // TRACE ROUTING TABLES
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (title + ".routes", std::ios::out);
    olsr.PrintRoutingTableAllEvery (Seconds (1), routingStream);
    Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> (title + ".neighbors", std::ios::out);
    olsr.PrintNeighborCacheAllEvery (Seconds (1), neighborStream);

    // TO DO-- ENABLE AN IP-LEVEL TRACE THAT SHOWS FORWARDING EVENTS ONLY
  }

  //SHOW ADDRESSES
  NS_LOG_UNCOND("Showing Addresses...");
  NodeContainer::Iterator nc;
  for(nc = combinedNodes.Begin(); nc != combinedNodes.End(); nc++){
    Ptr<Node> node = *nc;
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
    NS_LOG_UNCOND("Node: " << node->GetId() << " @ " << addr);
  }

  //UDP
  NS_LOG_UNCOND ("Setting Combined Nodes for UDP Traffic...");
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (combinedNodes.Get (sinkNode), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink ->Bind(local);
  recvSink ->SetRecvCallback (MakeCallback (&ReceivePacket));
  Ptr<Socket> source = Socket::CreateSocket (combinedNodes.Get(sourceNode), tid); //CRASHES HERE
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
  source->Connect (remote);
  NS_LOG_UNCOND ("Setting Combined Nodes UDP Complete!");  

  // OLSR CONVERGENCE
  NS_LOG_UNCOND ("Setting Simulator OLSR Convergence...");
  Simulator::Schedule (Seconds (convergence), &GenerateTraffic,
                       source, packetSize, numPackets, interPacketInterval);
  
  // OUTPUT SOURCE TO TARGET
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (sim_time));

  NS_LOG_UNCOND("Setting NetAnim...");
  AnimationInterface anim(title + ".xml");
  for(int i = 0;  i < deviceCount; i++){ //DON'T CHANGE DATA TYPE OF i EVEN IF THERE ARE WARNINGS
    //RANDOMIZATION SAMPLE CODE SOURCE: https://stackoverflow.com/a/7560168
    int x_pos = std::rand()%(int)(distance);
    int y_pos = std::rand()%(int)(distance);

    NS_LOG_UNCOND ("Node " << i << " at " << x_pos << "," << y_pos);
    anim.SetMobilityPollInterval(Seconds(1)); //Move every on second? Source: https://www.nsnam.org/wiki/NetAnim_3.108
    anim.SetConstantPosition(combinedNodes.Get(i), x_pos, y_pos); //PARAMETERS ARE AS FOLLOWS: ith NODE, X-POS, Y-POS; (BOTH POS IN RANDOM DEFAULT 0-25)
    anim.EnablePacketMetadata(true);
    
  }
  NS_LOG_UNCOND ("NetAnim Set!");
  NS_LOG_UNCOND ("");

  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("RUNNING SIMULATOR...");
  NS_LOG_UNCOND ("==============================================");
  NS_LOG_UNCOND ("Running Simulator (" << sim_time << " seconds)...");
  NS_LOG_UNCOND ("");

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
