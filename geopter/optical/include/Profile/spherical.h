#ifndef SPHERICAL_H
#define SPHERICAL_H

#include "surface_profile.h"

namespace geopter {

/** Standard spherical shape */
class Spherical : public SurfaceProfile
{
public:
    Spherical(double c=0.0);
    ~Spherical();

    double f(const Eigen::Vector3d& p) const override;
    Eigen::Vector3d df(const Eigen::Vector3d& p) const override;
    double sag(double x, double y) const override;

    void intersect(Eigen::Vector3d& pt, double& s, const Eigen::Vector3d& p0, const Eigen::Vector3d& d, double eps=1.0e-12, double z_dir=1.0) override;

};

} //namespace geopter

#endif // SPHERICAL_H