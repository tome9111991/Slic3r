#include "ExtrusionGeometry.hpp"

namespace Slic3r {

// caller is responsible for supplying NO lines with zero length
void
ExtrusionGeometry::_extrusionentity_to_verts_do(const Lines &lines, const std::vector<double> &widths,
        const std::vector<double> &heights, bool closed, double top_z, const Point &copy,
        GLVertexArray* qverts, GLVertexArray* tverts, float flatness)
{
    if (lines.empty()) return;
    
    const int N = 8;
    const float tube_values[N] = { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f, 0.0f, 0.5f };

    struct Ring {
        Pointf3 p[N];
        Pointf3 n[N];
    };

    auto get_ring = [&](const Pointf &center, const Vectorf &dir, double w, double h) {
        Ring r;
        Vectorf right_vec = Vectorf(dir.y, -dir.x);
        double rh = h / 2.0;
        double z_mid = top_z - rh;
        
        // Stadium geometry: width w, height h.
        // Flat top/bottom width is (w-h).
        double d_half = (w > h) ? (w - h) / 2.0 : 0.0;
        double r_semi = (w > h) ? rh : w / 2.0;

        for (int i = 0; i < N; ++i) {
            double x_off, z_off;
            float nx, ny, nz;
            
            // Define 8 vertices for a proper stadium:
            // 0: right extreme, 1: top-right arc, 2: top-right corner, 3: top-left corner,
            // 4: left extreme, 5: bottom-left arc, 6: bottom-left corner, 7: bottom-right corner
            switch(i) {
                case 0: x_off =  d_half + r_semi;      z_off = 0;           nx = (float)right_vec.x; ny = (float)right_vec.y; nz = 0; break;
                case 1: x_off =  d_half + r_semi*0.707; z_off = rh*0.707;    nx = (float)right_vec.x*0.707f; ny = (float)right_vec.y*0.707f; nz = 0.707f; break;
                case 2: x_off =  d_half;               z_off = rh;          nx = 0; ny = 0; nz = 1.0f; break;
                case 3: x_off = -d_half;               z_off = rh;          nx = 0; ny = 0; nz = 1.0f; break;
                case 4: x_off = -d_half - r_semi;      z_off = 0;           nx = -(float)right_vec.x; ny = -(float)right_vec.y; nz = 0; break;
                case 5: x_off = -d_half - r_semi*0.707; z_off = -rh*0.707;   nx = -(float)right_vec.x*0.707f; ny = -(float)right_vec.y*0.707f; nz = -0.707f; break;
                case 6: x_off = -d_half;               z_off = -rh;         nx = 0; ny = 0; nz = -1.0f; break;
                case 7: x_off =  d_half;               z_off = -rh;         nx = 0; ny = 0; nz = -1.0f; break;
                default: x_off = 0; z_off = 0; nx=0; ny=0; nz=1.0f; break;
            }

            if (flatness > 0.001f) {
                double s = z_off / rh;
                // Towards the top/bottom, we make the curve "boxier" for infill
                double s_flat = std::pow(std::abs(s), 1.0 - 0.7 * (double)flatness) * (s > 0 ? 1.0 : -1.0);
                z_off = s_flat * rh;
            }

            r.p[i] = Pointf3((float)(center.x + x_off * right_vec.x), 
                             (float)(center.y + x_off * right_vec.y), 
                             (float)(z_mid + z_off));
            
            float n_l = std::sqrt(nx*nx + ny*ny + nz*nz);
            r.n[i] = Pointf3(nx/n_l, ny/n_l, nz/n_l);
        }
        return r;
    };

    Ring prev_ring;
    bool first_done = false;

    size_t n_segments = lines.size();
    for (size_t i = 0; i <= n_segments; ++i) {
        if (i == n_segments && !closed) break;
        size_t idx = i % n_segments;
        const Line &line = lines[idx];
        
        double len = line.length();
        if (len < EPSILON) continue;
        
        double w = widths[idx];
        double h = heights[idx];

        Vectorf v = Vectorf::new_unscale(line.vector());
        v.scale(1.0 / unscale(len));
        
        Pointf a = Pointf::new_unscale(line.a);
        Pointf b = Pointf::new_unscale(line.b);

        Ring ring_a = get_ring(a, v, w, h);
        Ring ring_b = get_ring(b, v, w, h);

        if (first_done) {
            // Joint / Corner Filling
            Pointf3 jc((float)a.x, (float)a.y, (float)(top_z - h/2.0));
            
            for (int j = 0; j < N; ++j) {
                int next_j = (j + 1) % N;
                
                // 1. Joint Axis Fan Triangle (Internal volume filler)
                tverts->push_norm(prev_ring.n[j]); tverts->push_vert(jc);              tverts->push_tube(0.0f);
                tverts->push_norm(prev_ring.n[j]); tverts->push_vert(prev_ring.p[j]); tverts->push_tube(tube_values[j]);
                tverts->push_norm(ring_a.n[j]);    tverts->push_vert(ring_a.p[j]);    tverts->push_tube(tube_values[j]);

                // 2. Miter Surface Quad (Fills the gap between segments)
                // Triangle 1
                tverts->push_norm(prev_ring.n[j]);     tverts->push_vert(prev_ring.p[j]);     tverts->push_tube(tube_values[j]);
                tverts->push_norm(ring_a.n[j]);        tverts->push_vert(ring_a.p[j]);        tverts->push_tube(tube_values[j]);
                tverts->push_norm(ring_a.n[next_j]);   tverts->push_vert(ring_a.p[next_j]);   tverts->push_tube(tube_values[next_j]);
                // Triangle 2
                tverts->push_norm(prev_ring.n[j]);     tverts->push_vert(prev_ring.p[j]);     tverts->push_tube(tube_values[j]);
                tverts->push_norm(ring_a.n[next_j]);   tverts->push_vert(ring_a.p[next_j]);   tverts->push_tube(tube_values[next_j]);
                tverts->push_norm(prev_ring.n[next_j]); tverts->push_vert(prev_ring.p[next_j]); tverts->push_tube(tube_values[next_j]);
            }
        }

        if (i == n_segments && closed) break; 

        // Segment Faces (Quads)
        for (int j = 0; j < N; ++j) {
            int next_j = (j + 1) % N;
            qverts->push_norm(ring_a.n[j]);      qverts->push_vert(ring_a.p[j]);      qverts->push_tube(tube_values[j]);
            qverts->push_norm(ring_b.n[j]);      qverts->push_vert(ring_b.p[j]);      qverts->push_tube(tube_values[j]);
            qverts->push_norm(ring_b.n[next_j]); qverts->push_vert(ring_b.p[next_j]); qverts->push_tube(tube_values[next_j]);
            qverts->push_norm(ring_a.n[next_j]); qverts->push_vert(ring_a.p[next_j]); qverts->push_tube(tube_values[next_j]);
        }

        // End Caps (Only for open paths)
        if (!closed) {
            if (i == 0) {
                Pointf3 jc((float)a.x, (float)a.y, (float)(top_z - h/2.0));
                Vectorf3 back_norm((float)-v.x, (float)-v.y, 0);
                for (int j = 0; j < N; ++j) {
                    int next_j = (j + 1) % N;
                    tverts->push_norm(back_norm); tverts->push_vert(jc);              tverts->push_tube(0.0f);
                    tverts->push_norm(back_norm); tverts->push_vert(ring_a.p[next_j]); tverts->push_tube(tube_values[next_j]);
                    tverts->push_norm(back_norm); tverts->push_vert(ring_a.p[j]);      tverts->push_tube(tube_values[j]);
                }
            }
            if (i == n_segments - 1) {
                Pointf3 jc((float)b.x, (float)b.y, (float)(top_z - h/2.0));
                Vectorf3 front_norm((float)v.x, (float)v.y, 0);
                for (int j = 0; j < N; ++j) {
                    int next_j = (j + 1) % N;
                    tverts->push_norm(front_norm); tverts->push_vert(jc);              tverts->push_tube(0.0f);
                    tverts->push_norm(front_norm); tverts->push_vert(ring_b.p[j]);      tverts->push_tube(tube_values[j]);
                    tverts->push_norm(front_norm); tverts->push_vert(ring_b.p[next_j]); tverts->push_tube(tube_values[next_j]);
                }
            }
        }

        prev_ring = ring_b;
        first_done = true;
    }
}


void
GLVertexArray::load_mesh(const TriangleMesh &mesh)
{
    this->reserve_more(3 * 3 * mesh.facets_count());
    
    for (int i = 0; i < mesh.stl.stats.number_of_facets; ++i) {
        stl_facet &facet = mesh.stl.facet_start[i];
        for (int j = 0; j <= 2; ++j) {
            this->push_norm(facet.normal.x, facet.normal.y, facet.normal.z);
            this->push_vert(facet.vertex[j].x, facet.vertex[j].y, facet.vertex[j].z);
        }
    }
}



void ExtrusionGeometry::extrusion_to_instanced(const Lines &lines, const std::vector<double> &widths,
    const std::vector<double> &heights, double top_z, const Point &copy,
    const float* color, std::vector<InstanceData>& instances)
{
    size_t count = lines.size();
    instances.reserve(instances.size() + count);

    for (size_t i = 0; i < count; ++i) {
        const Line &line = lines[i];
        double w = widths[i];
        double h = heights[i];
        double z = top_z - h * 0.5;

        Point p1 = line.a;
        Point p2 = line.b;
        p1.translate(copy);
        p2.translate(copy);

        InstanceData inst;
        inst.posA[0] = (float)unscale(p1.x); 
        inst.posA[1] = (float)unscale(p1.y); 
        inst.posA[2] = (float)z;
        inst.width   = (float)w;
        
        inst.posB[0] = (float)unscale(p2.x); 
        inst.posB[1] = (float)unscale(p2.y); 
        inst.posB[2] = (float)z;
        inst.height  = (float)h;

        inst.color[0] = color[0];
        inst.color[1] = color[1];
        inst.color[2] = color[2];
        inst.color[3] = color[3];

        instances.push_back(inst);
    }
}

double ExtrusionGeometry::get_stadium_width(double mm3_per_mm, double height, double default_width)
{
    if (mm3_per_mm <= 0) return default_width;
    // Stadium model formula: w = A/h + h(1 - PI/4)
    return mm3_per_mm / height + height * (1.0 - M_PI / 4.0);
}

}