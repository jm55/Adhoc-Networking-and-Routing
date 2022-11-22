/*
MCO2 PROJECT
ESCALONA, J.M.
NSCOMO2

REFEFENCE: examples\routing\manet-routing-compare.cc
*/

#include <fstream>
#include <iostream>
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
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

/**
 * Routing experiment class.
 * 
 * It handles the creation and run of an experiment.
 */
class RoutingExperiment
{
    public:
        RoutingExperiment ();
        /**
         * Run the experiment.
         * \param nSinks The number of Sink Nodes.
         * \param txp The Tx power.
         * \param CSVfileName The output CSV filename.
         */
        void Run (int nSinks, double txp, std::string CSVfileName);
        //static void SetMACParam (ns3::NetDeviceContainer & devices,
        //                                 int slotDistance);
        /**
         * Handles the command-line parmeters.
         * \param argc The argument count.
         * \param argv The argument vector.
         * \return the CSV filename.
         */
        std::string CommandSetup (int argc, char **argv);
    private:
        /**
         * Setup the receiving socket in a Sink Node.
         * \param addr The address of the node.
         * \param node The node pointer.
         * \return the socket.
         */
        Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
        /**
         * Receive a packet.
         * \param socket The receiving socket.
         */
        void ReceivePacket (Ptr<Socket> socket);
        /**
         * Compute the throughput.
         */
        void CheckThroughput ();

        uint32_t port;            //!< Receiving port number.
        uint32_t bytesTotal;      //!< Total received bytes.
        uint32_t packetsReceived; //!< Total received packets.

        std::string m_CSVfileName;  //!< CSV filename.
        int m_nSinks;               //!< Number of sink nodes.
        std::string m_protocolName; //!< Protocol name.
        double m_txp;               //!< Tx power.
        bool m_traceMobility;       //!< Enavle mobility tracing.
        uint32_t m_protocol;        //!< Protocol type.
};

RoutingExperiment::RoutingExperiment (): 
    port (80),
    bytesTotal (0),
    packetsReceived (0),
    m_CSVfileName ("manet-routing.output.csv"),
    m_traceMobility (false),
    m_protocol () // AODV
    {}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
    std::ostringstream oss;

    oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

    if (InetSocketAddress::IsMatchingType (senderAddress))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
        oss << " received one packet from " << addr.GetIpv4 ();
    }
    else
    {
        oss << " received one packet!";
    }
    return oss.str ();
}

void RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    while ((packet = socket->RecvFrom (senderAddress)))
    {
        bytesTotal += packet->GetSize ();
        packetsReceived += 1;
        NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void RoutingExperiment::CheckThroughput ()
{
    double kbs = (bytesTotal * 8.0) / 1000;
    bytesTotal = 0;

    std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

    out << (Simulator::Now ()).GetSeconds () << ","
        << kbs << ","
        << packetsReceived << ","
        << m_nSinks << ","
        << m_protocolName << ","
        << m_txp << ""
        << std::endl;

    out.close ();
    packetsReceived = 0;
    Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket> RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket (node, tid);
    InetSocketAddress local = InetSocketAddress (addr, port);
    sink->Bind (local);
    sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

    return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
    CommandLine cmd (__FILE__);
    cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
    cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
    cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
    cmd.Parse (argc, argv);
    return m_CSVfileName;
}

int main (int argc, char *argv[])
{
    RoutingExperiment experiment;
    std::string CSVfileName = experiment.CommandSetup (argc,argv);

    //blank out the last output file and write the column headers
    std::ofstream out (CSVfileName.c_str ());
    out << "SimulationSecond," << "ReceiveRate," << "PacketsReceived," <<
    "NumberOfSinks," << "RoutingProtocol," << "TransmissionPower" << std::endl;
    out.close ();

    int nSinks = 10;
    double txp = 7.5;

    experiment.Run (nSinks, txp, CSVfileName);
}

void
RoutingExperiment::Run (int nSinks, double txp, std::string CSVfileName)
{
    Packet::EnablePrinting ();
    m_nSinks = nSinks;
    m_txp = txp;
    m_CSVfileName = CSVfileName;

    int staticDevices = 15;
    int mobileDevices = 15;

    double TotalTime = 300.0; //300s
    std::string rate ("2048"); //Originally set as 2048bps
    std::string phyMode ("Dsssrate11Mbps");
    std::string tr_name ("MCO2 - Escalona");
    int nodeSpeed = 1; //in m/s
    int nodePause = 0; //in s
    m_protocolName = "OLSR";

    Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("512"));
    Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

    //Set Non-unicastMode rate to unicast mode
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

    NodeContainer adhocNodes;
    adhocNodes.Create (mobileDevices);

    // setting up wifi phy and channel using helpers
    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel (wifiChannel.Create ());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

    wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
    wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

    wifiMac.SetType ("ns3::AdhocWifiMac");
    NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

    MobilityHelper mobilityAdhoc;
    [[maybe_unused]] int64_t streamIndex = 0; // used to get consistent mobility across scenarios

    ObjectFactory pos;
    pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
    pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
    pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
    streamIndex += taPositionAlloc->AssignStreams (streamIndex);

    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                    "Speed", StringValue (ssSpeed.str ()),
                                    "Pause", StringValue (ssPause.str ()),
                                    "PositionAllocator", PointerValue (taPositionAlloc));
    mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
    mobilityAdhoc.Install (adhocNodes);
    streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);

    OlsrHelper olsr;
    DsrMainHelper dsrMain;
    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;

    internet.SetRoutingHelper (list);
    internet.Install (adhocNodes);

    NS_LOG_UNCOND ("Assigning IP Address");

    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces;
    adhocInterfaces = addressAdhoc.Assign (adhocDevices);

    OnOffHelper onoff1 ("ns3::TcpSocketFactory",Address ());
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < nSinks; i++)
    {
        Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i));

        AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
        onoff1.SetAttribute ("Remote", remoteAddress);

        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
        ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + nSinks));
        temp.Start (Seconds (var->GetValue (100.0,101.0)));
        temp.Stop (Seconds (TotalTime));
    }

    std::stringstream ss;
    ss << mobileDevices;
    std::string nodes = ss.str ();

    std::stringstream ss2;
    ss2 << nodeSpeed;
    std::string sNodeSpeed = ss2.str ();

    std::stringstream ss3;
    ss3 << nodePause;
    std::string sNodePause = ss3.str ();

    std::stringstream ss4;
    ss4 << rate;
    std::string sRate = ss4.str ();

    //NS_LOG_UNCOND ("Configure Tracing.");
    //tr_name = tr_name + "_" + m_protocolName +"_" + nodes + "nodes_" + sNodeSpeed + "speed_" + sNodePause + "pause_" + sRate + "rate";

    //AsciiTraceHelper ascii;
    //Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (tr_name + ".tr").c_str());
    //wifiPhy.EnableAsciiAll (osw);
    AsciiTraceHelper ascii;
    MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));

    //Ptr<FlowMonitor> flowmon;
    //FlowMonitorHelper flowmonHelper;
    //flowmon = flowmonHelper.InstallAll ();


    NS_LOG_UNCOND ("Run Simulation.");

    CheckThroughput ();

    Simulator::Stop (Seconds (TotalTime));
    Simulator::Run ();

    //flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);

    Simulator::Destroy ();
}

