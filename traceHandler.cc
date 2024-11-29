#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/socket.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P5TraceHandler");

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    std::clog << "[TraceHandler] ";



void CwndTraceSink(Ptr<OutputStreamWrapper> logStream, uint32_t oldval, uint32_t newval) {
    *logStream->GetStream() << Simulator::Now().GetSeconds() << "," << newval << std::endl;
}

void SetupTracing(Ptr<Node> node) {
    // Get the list of sockets on the specified node
    ObjectMapValue socketList;
    node->GetObject<TcpL4Protocol>()->GetAttribute("SocketList", socketList);

    NS_LOG_DEBUG("Setting up tracing for node " << node->GetId() << " (has " << socketList.GetN() << " socket)");
    // For each socket on the node, enable whatever tracing is relevant!
    for (size_t socketIndex = 0; socketIndex < socketList.GetN(); socketIndex++) {

        Ptr<TcpSocketBase> socketBase = DynamicCast<TcpSocketBase>(socketList.Get(socketIndex));

        // --- CONGESTION WINDOW ---
        std::string logName = "scratch/P5-Satellite/out/CongestionWindow_Node" + std::to_string(node->GetId()) + "_Socket" + std::to_string(socketIndex) + ".txt";
        // C++ can not make use of an ofstream through a bound callback, as noted by the CreateFileStream() method. Therefore we make use of the ns3 AsciiTraceHelper to pass around a stream which the callback methods can write to!
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> logStream = asciiTraceHelper.CreateFileStream(logName);
        // Row 1 should be the names of the columns
        *logStream->GetStream() << "time(s),CongestionWindow" << std::endl;
        // At time 0 the CWND=0, which is missing unless we just add it here
        *logStream->GetStream() << "0,0" << std::endl;
        
        // Trace a source to the congestion window, but do so with a Make*Bound*Callback!
        // This allows us to pass in additional method parameters beyond the one that CongestionWindow supports!
        // Genius for logging!
        socketBase->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndTraceSink, logStream));
        // TODO, PLOT THIS ALSO
        // socketBase->TraceConnectWithoutContext("RTT")

        // ======== ADDITIONAL SHIT IF NEEDED LATER ========
        // Setting socket MinRTO
        // socketBase->SetMinRto(Time(Seconds(2)));

        // Setting socket Congestion control algorithm AND Recovery type?!
        // socketBase->SetCongestionControlAlgorithm();
        // socketBase->SetRecoveryAlgorithm();
    }
}



void printCompleteRoute(Ptr<Node> srcNode, Ptr<Node> dstNode) {
    NS_LOG_DEBUG("[!] Route testing (from node " << srcNode->GetId() << " to node " << dstNode->GetId());

    Ipv4Header header;
    header.SetDestination( dstNode->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress() );
    header.SetSource( srcNode->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress() );
    header.SetProtocol( TcpL4Protocol::PROT_NUMBER );
    Socket::SocketErrno errnoOut = Socket::ERROR_NOTERROR;

    // Start at the source node and get the next hop node based on the routing table
    // The next hop is the gateway, which is found by continously using the IP of the destination node
    // When gateway is found, this becomes the new source node. Repeat until at destination
    Ptr<Node> currentNode = srcNode;
    while (true) {
        Ptr<Ipv4Route> route = currentNode->GetObject<Ipv4>()->GetRoutingProtocol()->RouteOutput(nullptr, header, 0, errnoOut);
        int32_t interfaceOut = currentNode->GetObject<Ipv4>()->GetInterfaceForDevice( route->GetOutputDevice() );
        
        Ptr<NetDevice> localNetDevice = currentNode->GetDevice(interfaceOut);
        Ptr<NetDevice> foreignNetDevice;
        for (int n=0; n < 2; n++) {
            Ptr<NetDevice> tmpDevice = localNetDevice->GetChannel()->GetDevice(n);
            if (localNetDevice != tmpDevice)
                foreignNetDevice = tmpDevice;
        }
        
        Ptr<Node> foreignNode = foreignNetDevice->GetNode();
        NS_LOG_DEBUG("Forward to " << foreignNode->GetId() << " through gateway " << route->GetGateway());

        if (foreignNode == dstNode)
            break;
        
        currentNode = foreignNode;
    }

    // return the route here or something
}


// Test function for utilizing SeqHeaders (see where it is called)
void ReceiveWithSeqTsSize(Ptr<const Packet> pkt, const Address& from, const Address& dst, const SeqTsSizeHeader& header) {
    NS_LOG_DEBUG("Sink received SetTsSize pkt -> " << header);
}
