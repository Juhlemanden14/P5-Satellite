#ifndef TRACING_HANDLER_H
#define TRACING_HANDLER_H

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

using namespace ns3;

/**
 * \brief Used as trace sink for logging Congestion Windows to a log stream
 * \warning INTERNAL METHOD do not call!
 * \param logStream The log stream where the output will be written
 * \param oldval (from trace source)
 * \param newval (from trace source)
 */
void CwndTraceSink(Ptr<OutputStreamWrapper> logStream, uint32_t oldval, uint32_t newval);

/**
 * \brief A master method for enabling tracing for all the sockets on a node
 * \param node The node which to enable the tracing
 */
void SetupTracing(Ptr<Node> node);

/**
 * \brief Given a source and destination node, get the complete path between them
 * \param srcNode Source node
 * \param dstNode Destination node
 */
void printCompleteRoute(Ptr<Node> srcNode, Ptr<Node> dstNode);

/**
 * A trace sink for receiving SeqTsSizeHeader packets on an application
 * All parameters are here to match the trace source <RxWithSeqTsSize>
 */
void ReceiveWithSeqTsSize(Ptr<const Packet> pkt,
                          const Address& from,
                          const Address& dst,
                          const SeqTsSizeHeader& header);

#endif
