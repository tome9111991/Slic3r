#ifndef slic3r_ExtrusionGeometry_hpp_
#define slic3r_ExtrusionGeometry_hpp_

#include "../../libslic3r/libslic3r.h"
#include "../../libslic3r/Point.hpp"
#include "../../libslic3r/Line.hpp"
#include "../../libslic3r/TriangleMesh.hpp"

namespace Slic3r {

class GLVertexArray {
    public:
    std::vector<float> verts, norms, tube_coords;
    
    void reserve(size_t len) {
        this->verts.reserve(len);
        this->norms.reserve(len);
        this->tube_coords.reserve(len/3);
    };
    void reserve_more(size_t len) {
        len += this->verts.size();
        this->reserve(len);
    };
    void push_vert(const Pointf3 &point) {
        this->verts.push_back(point.x);
        this->verts.push_back(point.y);
        this->verts.push_back(point.z);
    };
    void push_vert(float x, float y, float z) {
        this->verts.push_back(x);
        this->verts.push_back(y);
        this->verts.push_back(z);
    };
    void push_norm(const Pointf3 &point) {
        this->norms.push_back(point.x);
        this->norms.push_back(point.y);
        this->norms.push_back(point.z);
    };
    void push_norm(float x, float y, float z) {
        this->norms.push_back(x);
        this->norms.push_back(y);
        this->norms.push_back(z);
    };
    void push_tube(float t) {
        this->tube_coords.push_back(t);
    }
    void load_mesh(const TriangleMesh &mesh);
    Pointf3 get_point(int i){
        return Pointf3(this->verts.at(i*3),this->verts.at(i*3+1),this->verts.at(i*3+2));
    }
};

class ExtrusionGeometry
{
    public:
    static void _extrusionentity_to_verts_do(const Lines &lines, const std::vector<double> &widths,
        const std::vector<double> &heights, bool closed, double top_z, const Point &copy,
        GLVertexArray* qverts, GLVertexArray* tverts, float flatness = 0.0f);
    
    struct InstanceData {
        float posA[3];
        float width;
        float posB[3];
        float height;
        float color[4];
    };

    static void extrusion_to_instanced(const Lines &lines, const std::vector<double> &widths,
        const std::vector<double> &heights, double top_z, const Point &copy,
        const float* color, std::vector<InstanceData>& instances);

    static double get_stadium_width(double mm3_per_mm, double height, double default_width);
};

}

#endif