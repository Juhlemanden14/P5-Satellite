#include "SRFMath.h"

#include "ns3/core-module.h"
#include "ns3/satellite-module.h"

#include <cmath>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("P5-SRFMath");


Vector normalizeVector(Vector vec) {
    double vec_mag = vec.GetLength();   // calculate the magnitude of the vector
    double x_norm = vec.x / vec_mag;    // devide every component by the magnitude
    double y_norm = vec.y / vec_mag;
    double z_norm = vec.z / vec_mag;
    Vector vec_norm = Vector(x_norm, y_norm, z_norm); // assemble the new vector
    return vec_norm;
}

Vector vectorCrossProduct(Vector vec0, Vector vec1) {
    // Compute the cross product components - see https://se.mathworks.com/help/aeroblks/3x3crossproduct.html for reference
    double cross_x = vec0.y * vec1.z - vec0.z * vec1.y;
    double cross_y = vec0.z * vec1.x - vec0.x * vec1.z;
    double cross_z = vec0.x * vec1.y - vec0.y * vec1.x;
    return Vector(cross_x, cross_y, cross_z); // Return the resulting vector
}

Vector projectOntoPlane(Vector fromVec, Vector normVecZ){
    // vec_proj = vec - (vec ⋅ z)*z.   IMPORTANT: the star (*) between two ns3:Vectors computes the dot-product and is NOT multiplication
    double scalar = fromVec * normVecZ;         // vec ⋅ z
    double z_component_x = scalar * normVecZ.x; // (vec ⋅ z)*z happens on these 3 lines, element for element
    double z_component_y = scalar * normVecZ.y;
    double z_component_z = scalar * normVecZ.z;
    Vector z_component = Vector(z_component_x, z_component_y, z_component_z); // creating vector based on individual parts from 3 lines above
    Vector vec_proj = fromVec - z_component;    // compute final projection
    return vec_proj;
}


std::pair<double, double> getAngleFromSatPair(Ptr<SatSGP4MobilityModel>sat0, Ptr<SatSGP4MobilityModel>sat1) {
    // Get the ECEF positions of the satellites
    Vector sat0_r = sat0->GetPosition();
    Vector sat1_r = sat1->GetPosition();
    NS_LOG_DEBUG("[LH] sat0_r: " << sat0_r);
    NS_LOG_DEBUG("[LH] sat1_r: " << sat1_r);


    // Get their velocity vectors
    Vector sat0_v = sat0->GetVelocity();
    Vector sat1_v = sat1->GetVelocity();
    NS_LOG_DEBUG("[LH] sat0_v: " << sat0_v);
    NS_LOG_DEBUG("[LH] sat1_v: " << sat1_v);


    // Compute the relative vectors
    Vector sat0_to_sat1 = sat1_r - sat0_r; // vector pointing from sat0 to sat1
    Vector sat1_to_sat0 = sat0_r - sat1_r; // opposite of above vector
    NS_LOG_DEBUG("[LH] sat0_to_sat1: " << sat0_to_sat1);
    NS_LOG_DEBUG("[LH] sat1_to_sat0: " << sat1_to_sat0);


    // compute future y-axis for both systems
    Vector sat0_y = vectorCrossProduct(sat0_r, sat0_v);
    Vector sat1_y = vectorCrossProduct(sat1_r, sat1_v);
    NS_LOG_DEBUG("[LH] sat0_y: " << sat0_y);
    NS_LOG_DEBUG("[LH] sat1_y: " << sat1_y);


    // normalize both satellite reference frames
    Vector SRF0_x = normalizeVector(sat0_v);
    Vector SRF0_y = normalizeVector(sat0_y);
    Vector SRF0_z = normalizeVector(sat0_r);
    NS_LOG_DEBUG("[LH] SRF0_x: " << SRF0_x);
    NS_LOG_DEBUG("[LH] SRF0_y: " << SRF0_y);
    NS_LOG_DEBUG("[LH] SRF0_z: " << SRF0_z);
    Vector SRF1_x = normalizeVector(sat1_v);
    Vector SRF1_y = normalizeVector(sat1_y);
    Vector SRF1_z = normalizeVector(sat1_r);
    NS_LOG_DEBUG("[LH] SRF1_x: " << SRF1_x);
    NS_LOG_DEBUG("[LH] SRF1_y: " << SRF1_y);
    NS_LOG_DEBUG("[LH] SRF1_z: " << SRF1_z);



    // random orthogonality test - remove later
    auto test = SRF0_x * SRF0_y;
    NS_LOG_DEBUG("[LH] SRF0_x dot SRF0_y: " << test);


    // Project the relative vectors onto the SRF's for each satellite
    Vector proj_sat1_onto_SRF0 = projectOntoPlane(sat0_to_sat1, SRF0_z);
    Vector proj_sat0_onto_SRF1 = projectOntoPlane(sat1_to_sat0, SRF1_z);
    NS_LOG_DEBUG("[LH] proj_sat1_onto_SRF0: " << proj_sat1_onto_SRF0);
    NS_LOG_DEBUG("[LH] proj_sat0_onto_SRF1: " << proj_sat0_onto_SRF1);


    // Decompose the projected vector in terms of the basis (Get what the unit vectors are scaled with)
    double x_scalar_proj_sat1 = proj_sat1_onto_SRF0 * SRF0_x;     // get the scalar of the x-axis unit vector in the SFR0 using the dot-product
    double y_scalar_proj_sat1 = proj_sat1_onto_SRF0 * SRF0_y;
    NS_LOG_DEBUG("[LH] proj_sat1_onto_SRF0 x-axis scalar: " << x_scalar_proj_sat1);
    NS_LOG_DEBUG("[LH] proj_sat1_onto_SRF0 y-axis scalar: " << y_scalar_proj_sat1);
    double x_scalar_proj_sat0 = proj_sat0_onto_SRF1 * SRF1_x;
    double y_scalar_proj_sat0 = proj_sat0_onto_SRF1 * SRF1_y;
    NS_LOG_DEBUG("[LH] proj_sat1_onto_SRF0 x-axis scalar: " << x_scalar_proj_sat1);
    NS_LOG_DEBUG("[LH] proj_sat1_onto_SRF0 y-axis scalar: " << y_scalar_proj_sat1);


    // Compute the angle with respect to any satellite's SRF_x by using arctan
    double angle_sat0_to_sat1 = atan2(y_scalar_proj_sat1, x_scalar_proj_sat1);  // angle from satellite0 to satellite 1 with respect to sat0's velocity vector
    double angle_sat1_to_sat0 = atan2(y_scalar_proj_sat0, x_scalar_proj_sat0);  // both of these angles are in radians
    angle_sat0_to_sat1 = angle_sat0_to_sat1 * 180 / M_PI;       // convert angles to degrees
    angle_sat1_to_sat0 = angle_sat1_to_sat0 * 180 / M_PI;
    NS_LOG_DEBUG("[LH] angle from sat0 to sat1 with respect to sat0's velocity vector: " << angle_sat0_to_sat1);
    NS_LOG_DEBUG("[LH] angle from sat1 to sat0 with respect to sat1's velocity vector: " << angle_sat1_to_sat0);

    // return the angles in order:
    //  First:  The angle from sat0 to sat1 from sat0's POV with regards to sat0's velocity
    //  Second: The angle from sat1 to sat0 from sat0's POV with regards to sat1's velocity
    return std::pair(angle_sat0_to_sat1, angle_sat1_to_sat0);
}
