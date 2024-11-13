#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
// #include "ns3/satellite-module.h"

#include "tle-handler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5-Satellite");


int main(int argc, char* argv[]) {
    LogComponentEnable("P5-Satellite", LOG_LEVEL_ALL);
    
    // ========================================= Setup default commandline parameters  =========================================
    std::string tleDataPath = "scratch/P5-Satellite/TLE-data/starlink_12-11-2024_tle_data.txt";

    CommandLine cmd(__FILE__);
    cmd.AddValue("tledata", "TLE Data path", tleDataPath);
    cmd.Parse(argc, argv);
    NS_LOG_INFO("[+] CommandLine arguments parsed succesfully");
    // ========================================= Setup default commandline parameters  =========================================

    
    std::vector<TLE> TLEVector = ReadTLEFile(tleDataPath);

    NS_LOG_INFO(TLEVector.size());

    NS_LOG_INFO(TLEVector[0].name);
    
    Time::SetResolution(Time::NS);

    NodeContainer nodes;
    nodes.Create(2);
    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}


