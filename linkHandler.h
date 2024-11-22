#ifndef LINK_HANDLER_H
#define LINK_HANDLER_H

#include "ns3/satellite-module.h"

using namespace ns3;

/**
Calculates both the angle from sat0's velocity vector and a vector pointing at sat1 and the opposite order also. Calculations are based on
ECEF system and take place in each satellites local reference frame. Conversions to said frame are done before the angle can be calculated
Returns the angle from sat0 to sat1 and sat1 to sat0 in that order.
*/
std::pair<double, double> getAngleFromSatPair(Ptr<SatSGP4MobilityModel>sat0, Ptr<SatSGP4MobilityModel>sat1);

/**
Normalizes a vector to have lenght 1.
*/
Vector normalizeVector(Vector vec);

/**
Returns the cross product of vec0(A) and vec1(B): A x B = |A||B|*sin(Theta), where sin 
*/
Vector vectorCrossProduct(Vector vec0, Vector vec1);

/**
Returns the Vector which is the projection of the relative 'fromVec' onto the plane spanned by the SRF's x and y axes. 'normVecZ' is the
unit z-axis of the SRF and is orthogonal to the plane spanned by x and y. 
*/
Vector projectOntoPlane(Vector fromVec, Vector normVecZ);

#endif