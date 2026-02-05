#include "ExtrusionGeometry.hpp"

namespace Slic3r {

// caller is responsible for supplying NO lines with zero length
void
ExtrusionGeometry::_extrusionentity_to_verts_do(const Lines &lines, const std::vector<double> &widths,
        const std::vector<double> &heights, bool closed, double top_z, const Point &copy,
        GLVertexArray* qverts, GLVertexArray* tverts, double nozzle_dm, float flatness)
{
    if (lines.empty()) return;
    
    const int N = 8;
    const double angles[N] = { 0, M_PI/4.0, M_PI/2.0, 3.0*M_PI/4.0, M_PI, 5.0*M_PI/4.0, 3.0*M_PI/2.0, 7.0*M_PI/4.0 };
    const float tube_values[N] = { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f, 0.0f, 0.5f };

    struct Ring {
        Pointf3 p[N];
        Pointf3 n[N];
    };

    auto get_ring = [&](const Pointf &center, const Vectorf &dir, double w, double h) {
        Ring r;
        Vectorf right_vec = Vectorf(dir.y, -dir.x);
        double rw = w / 2.0;
        double rh = h / 2.0;
        double z_mid = top_z - rh;
        
        for (int i = 0; i < N; ++i) {
            double c = cos(angles[i]);
            double s = sin(angles[i]);
            
            // Apply flatness: if flatness > 0, we pull the top/bottom vertices closer to the z bounds
            // and keep the side vertices where they are.
            double s_flat = s;
            if (flatness > 0.001f) {
                // Towards the top/bottom (s=1 or s=-1), we make the curve "boxier"
                s_flat = std::pow(std::abs(s), 1.0 - 0.7 * flatness) * (s > 0 ? 1.0 : -1.0);
            }

            r.p[i] = Pointf3((float)(center.x + c * rw * right_vec.x), 
                             (float)(center.y + c * rw * right_vec.y), 
                             (float)(z_mid + s_flat * rh));
            
            // Recalculate normal for flattened shape
            Vectorf3 n_vec((float)(c * right_vec.x), (float)(c * right_vec.y), (float)(s * (1.0 - flatness * 0.5)));
            r.n[i] = n_vec.normalize();
        }
        return r;
    };

    Ring prev_ring;
    const Line* prev_line = nullptr;
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
        
        // Removed the nozzle_dm cap to prevent "holey" infill.
        // The slicer's width w is the correct spacing between centerlines.

        Vectorf v = Vectorf::new_unscale(line.vector());
        v.scale(1.0 / unscale(len));
        
        Pointf a = Pointf::new_unscale(line.a);
        Pointf b = Pointf::new_unscale(line.b);
        Pointf center_a = a;
        Pointf center_b = b;

        Ring ring_a = get_ring(center_a, v, w, h);
        Ring ring_b = get_ring(center_b, v, w, h);

        if (first_done) {
            // Corner Gap Filling
            Pointf3 jc((float)center_a.x, (float)center_a.y, (float)(top_z - h/2.0));
            
            for (int j = 0; j < N; ++j) {
                tverts->push_norm(prev_ring.n[j]); tverts->push_vert(jc);              tverts->push_tube(0.0f);
                tverts->push_norm(prev_ring.n[j]); tverts->push_vert(prev_ring.p[j]); tverts->push_tube(tube_values[j]);
                tverts->push_norm(ring_a.n[j]);    tverts->push_vert(ring_a.p[j]);    tverts->push_tube(tube_values[j]);
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

        // Caps
        if (!closed) {
            if (i == 0) {
                Pointf3 jc((float)center_a.x, (float)center_a.y, (float)(top_z - h/2.0));
                Vectorf3 back_norm((float)-v.x, (float)-v.y, 0);
                for (int j = 0; j < N; ++j) {
                    int next_j = (j + 1) % N;
                    tverts->push_norm(back_norm); tverts->push_vert(jc);              tverts->push_tube(0.0f);
                    tverts->push_norm(back_norm); tverts->push_vert(ring_a.p[next_j]); tverts->push_tube(tube_values[next_j]);
                    tverts->push_norm(back_norm); tverts->push_vert(ring_a.p[j]);      tverts->push_tube(tube_values[j]);
                }
            }
            if (i == n_segments - 1) {
                Pointf3 jc((float)center_b.x, (float)center_b.y, (float)(top_z - h/2.0));
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
        prev_line = &line;
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

}