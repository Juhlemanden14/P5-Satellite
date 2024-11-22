"""
This file contains a calculation of the angle between STARLINK-4423 and STARLINK-4416 relative to STARLINK-4423's movement direction (forward)

These velocities are calculated with TLE data from 2024-11-13 14:33:31. And can be found in one of the TLE data files with that timestamp

satellite 0's velocity: x=1803.23, y=-7328.81, z=-1237.8
satellite 0's pos: lat=77.8136, long=-35.7327
satellite 1's velocity: x=1761.77, y=-7308.61, z=-1408.91
satellite 1's pos: lat=76.758, long=-39.3981

Plots are included to show the mathematical operations



Requirements for the c++ NS-3 implementation of this:
 - Vector addition/subtraction
 - Vector scaling (multiplication)
 - Vector dot product
 - Vector magnitude
 - Vector normalization
 - Vector cross product
 - Vector projection onto plane (remove z component)
 - arctan with +/- pi range (important to have either 0 to 360 deg or -180 to 180 degs range. Be careful with negative values)

Ressources:
 - Linear Algebra knowledge from classes
 - https://medium.com/@zackfizell10/reference-frames-every-aerospace-engineer-should-know-db10638b6d7a
 - https://en.wikipedia.org/wiki/Geographic_coordinate_conversion
"""
import matplotlib.pyplot as plt
import numpy as np


def LLA_to_ECEF(lat, long, alt):
    """
    Convert from Lat, Long, Alt to Earth Centered - Earth Fixed coordinates (X, Y, Z style).
    They will then be usable with the sat velocities.
    Using this https://en.wikipedia.org/wiki/Geographic_coordinate_conversion but assuming perfect sphere
    """
    x = alt * np.cos(np.deg2rad(lat)) * np.cos(np.deg2rad(long))
    y = alt * np.cos(np.deg2rad(lat)) * np.sin(np.deg2rad(long))
    z = alt * np.sin(np.deg2rad(lat))       # assumes perfect sphere where a²/b² = 1

    return np.array([x, y, z])
    
def vector_magnitude(array):
    """
    return the vector magnitude"""
    return np.sqrt(np.dot(array, array))

def normalize_vector(vec):
    """
    normalize with normalized = vec/|vec|
    """
    return vec / vector_magnitude(vec)







if __name__ == "__main__":
    
    earth_radius = 6_378_000 # meters - assuming perfect sphere, which will create a small, varying error

    # using STARLINK-4423 and -4416
    sat0_v = np.array([1803.23, -7328.81, -1237.8])
    #                  lat,     long,     altitude + earth radius
    sat0_r = np.array([77.8136, -35.7327, earth_radius + 554_918])

    # using STARLINK-4423 and -4416
    sat1_v = np.array([1761.77, -7308.61, -1408.91])
    sat1_r = np.array([76.758, -39.3981, earth_radius + 554_561])
    # -------------------- fake sat1_r for testing: --------------------------
    # sat1_r = np.array([56.758, -39.3981, earth_radius + 554_561 - 1000000])
    # ------------------------------------------------------------------------

    # get ECEF instead of LLA
    sat0_ECEF = LLA_to_ECEF(sat0_r[0], sat0_r[1], sat0_r[2])
    sat1_ECEF = LLA_to_ECEF(sat1_r[0], sat1_r[1], sat1_r[2])
    print(f"Sat0's xyz vals: [{sat0_ECEF}]")
    print(f"Sat1's xyz vals: [{sat1_ECEF}]")

    # normalize v and r (both from ECEF first)
    sat0_r_norm = normalize_vector(sat0_ECEF)
    sat0_v_norm = normalize_vector(sat0_v)
    print(f"sat0's normalized r(ECEF): {sat0_r_norm}")
    print(f"sat0's normalized v(ECEF): {sat0_v_norm}")
    print(f"dot product: {np.dot(sat0_r_norm, sat0_v_norm)}")
    sat1_r_norm = normalize_vector(sat1_ECEF)
    sat1_v_norm = normalize_vector(sat1_v)
    print(f"sat1's normalized r(ECEF): {sat1_r_norm}")
    print(f"sat1's normalized v(ECEF): {sat1_v_norm}")
    print(f"dot product: {np.dot(sat1_r_norm, sat1_v_norm)}")

    # find relative vector using ECEF:
    r_rel = sat1_ECEF - sat0_ECEF
    print(f"r_rel(ECEF norm): {r_rel}")

    # find sat0 y-unit axis with cross-product of x and z:
    sat0_y_norm = np.cross(sat0_r_norm, sat0_v_norm)
    print(sat0_y_norm)
    print(f"dot product x * y: {np.dot(sat0_v_norm, sat0_y_norm)}")
    print(f"dot product z * y: {np.dot(sat0_r_norm, sat0_y_norm)}")

    # rename variables to make more sense:
    sat0_x = sat0_v_norm    # forward direction
    sat0_y = sat0_y_norm    # left direction
    sat0_z = sat0_r_norm    # upward direction
    print(f"sat x: {sat0_x}")
    print(f"sat y: {sat0_y}")
    print(f"sat z: {sat0_z}")

    # Compute the projection of the relative vector onto the plane spanned by sat0_x and sat0_y
    # A plane spanned by x and y has a normal vector z, that is orthogonal to every vector in the plane. We will subtract this component
    # from the r_rel vector
    # vec_proj = vec - (vec ⋅ z)*z
    vec_proj = r_rel - (np.dot(r_rel, sat0_z))*sat0_z
    print(f"The vector projection of sat1 onto sat0's SRF: {vec_proj}")

    print(f"distance between sats: {vector_magnitude(r_rel)} m")

    # Decompose the projected vector in terms of the basis (Get v_rel_x and v_rel_y as expressed by the local coordinate system)
    vec_x = np.dot(vec_proj,sat0_x)
    vec_y = np.dot(vec_proj, sat0_y)
    print(f" decomp: {vec_x}, {vec_y}")

    # Compute the angle with respect to sat0_x by using arctan
    theta = np.arctan2(vec_y, vec_x)
    print(f"Angle between satellites in sat0's reference plane: {np.rad2deg(theta)} degrees")



    # ============================================ Plotting ============================================
    fig = plt.figure(figsize=(16, 12))
    # ------------- subplot 1 -------------
    ax1 = fig.add_subplot(121, projection="3d")
    ax1.scatter(sat0_ECEF[0], sat0_ECEF[1], sat0_ECEF[2], label="sat0_xyz")
    ax1.scatter(sat1_ECEF[0], sat1_ECEF[1], sat1_ECEF[2], label="sat1_xyz")
    ax1.scatter(0, 0, 0, label="core")

    test = LLA_to_ECEF(sat0_r[0], sat0_r[1], earth_radius)
    ax1.scatter(test[0], test[1], test[2], label="surface")

    test2 = LLA_to_ECEF(sat1_r[0], sat1_r[1], earth_radius)
    ax1.scatter(test2[0], test2[1], test2[2], label="surface2")
    
    ax1.set_xlabel("x")
    ax1.set_ylabel("y")
    ax1.set_zlabel("z")
    ax1.set_title("Scatterplot of STARLINK-4423 and STARLINK-4416's positions")
    ax1.set_xlim(-10_000_000, 10_000_000)
    ax1.set_ylim(-10_000_000, 10_000_000)
    ax1.set_zlim(-10_000_000, 10_000_000)
    ax1.legend()
    # -------------------------------------


    # ------------- subplot 2 -------------
    ax2 = fig.add_subplot(122, projection="3d")
    ax2.quiver(0, 0, 0, sat0_z[0], sat0_z[1], sat0_z[2], label="sat0_r", color="black", alpha=0.1) # sat r
    ax2.quiver(sat0_z[0], sat0_z[1], sat0_z[2], sat0_z[0], sat0_z[1], sat0_z[2], label="sat0_z", color="blue", alpha=0.1)       # sat z
    ax2.quiver(sat0_z[0], sat0_z[1], sat0_z[2], sat0_x[0], sat0_x[1], sat0_x[2], label="sat0_x", color="red", alpha=0.1)        # sat x
    ax2.quiver(sat0_z[0], sat0_z[1], sat0_z[2], sat0_y[0], sat0_y[1], sat0_y[2], label="sat0_y", color="green", alpha=0.1)      # sat y

    # ax2.quiver(0, 0, 0, sat1_r_norm[0], sat1_r_norm[1], sat1_r_norm[2], label="sat0_r")
    # ax2.quiver(sat1_r_norm[0], sat1_r_norm[1], sat1_r_norm[2], sat1_v_norm[0], sat1_v_norm[1], sat1_v_norm[2], label="sat0_v")

    # show relative vector between sat1 and sat0 (sat1 - sat0)
    ax2.quiver(sat0_r_norm[0], sat0_r_norm[1], sat0_r_norm[2], r_rel[0], r_rel[1], r_rel[2], label="r_rel", color="purple")
    ax2.quiver(sat0_r_norm[0], sat0_r_norm[1], sat0_r_norm[2], vec_proj[0], vec_proj[1], vec_proj[2], label="vec_proj", color="yellow")

    ax2.set_xlabel("x")
    ax2.set_ylabel("y")
    ax2.set_zlabel("z")
    ax2.set_title("Vectors of STARLINK-4423 and STARLINK-4416's positions and velocities")
    ax2.set_xlim(-2, 2)
    ax2.set_ylim(-2, 2)
    ax2.set_zlim(-2, 2)
    # ax2.set_xlim(-10_00_000, 10_00_000)
    # ax2.set_ylim(-10_00_000, 10_00_000)
    # ax2.set_zlim(-10_00_000, 10_00_000)
    
    ax2.legend()
    
    # -------------------------------------

    fig.tight_layout()
    plt.show()
    # ============================================ Plotting ============================================
