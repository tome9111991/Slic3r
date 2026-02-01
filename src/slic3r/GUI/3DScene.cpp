#include "3DScene.hpp"

namespace Slic3r {

// caller is responsible for supplying NO lines with zero length
void
_3DScene::_extrusionentity_to_verts_do(const Lines &lines, const std::vector<double> &widths,
        const std::vector<double> &heights, bool closed, double top_z, const Point &copy,
        GLVertexArray* qverts, GLVertexArray* tverts)
{
    if (lines.empty()) return;
    
    Line prev_line;
    Pointf prev_TR, prev_TL, prev_BR, prev_BL;
    Pointf prev_center_top, prev_center_bot;
    
    // loop once more in case of closed loops
    bool first_done = false;
    for (size_t i = 0; i <= lines.size(); ++i) {
        if (i == lines.size()) i = 0;
        
        const Line &line = lines.at(i);
        if (i == 0 && first_done && !closed) break;
        
        double len = line.length();
        double unscaled_len = unscale(len);
        
        double bottom_z = top_z - heights.at(i);
        double dist = widths.at(i)/2;  // scaled
        
        Vectorf v = Vectorf::new_unscale(line.vector());
        v.scale(1/unscaled_len);
        
        // Calculate points for the start (a) and end (b) of the segment
        Pointf a = Pointf::new_unscale(line.a);
        Pointf b = Pointf::new_unscale(line.b);
        
        // Displacements
        Vectorf right_vec = Vectorf(v.y, -v.x);
        Vectorf left_vec = Vectorf(-v.y, v.x);
        
        Pointf a_TR = a; a_TR.translate(right_vec.x * dist, right_vec.y * dist);
        Pointf a_TL = a; a_TL.translate(left_vec.x * dist, left_vec.y * dist);
        Pointf a_BR = a_TR;
        Pointf a_BL = a_TL;
        
        Pointf b_TR = b; b_TR.translate(right_vec.x * dist, right_vec.y * dist);
        Pointf b_TL = b; b_TL.translate(left_vec.x * dist, left_vec.y * dist);
        Pointf b_BR = b_TR;
        Pointf b_BL = b_TL;

        // Normals
        Vector n = line.normal();
        Vectorf3 xy_right_normal = Vectorf3::new_unscale(n.x, n.y, 0);
        xy_right_normal.scale(1/unscaled_len);
        Vectorf3 xy_left_normal = xy_right_normal;
        xy_left_normal.scale(-1);
        
        if (first_done) {
            // Corner handling
            double ccw = line.b.ccw(prev_line);
            if (ccw > EPSILON) {
                // Left Turn - Outer is Right side
                // Fill Top Gap
                tverts->push_norm(0,0,1); tverts->push_vert((float)prev_center_top.x, (float)prev_center_top.y, (float)top_z);
                tverts->push_norm(0,0,1); tverts->push_vert((float)prev_TR.x, (float)prev_TR.y, (float)top_z);
                tverts->push_norm(0,0,1); tverts->push_vert((float)a_TR.x, (float)a_TR.y, (float)top_z);

                // Fill Bottom Gap
                tverts->push_norm(0,0,-1); tverts->push_vert((float)prev_center_bot.x, (float)prev_center_bot.y, (float)bottom_z);
                tverts->push_norm(0,0,-1); tverts->push_vert((float)a_BR.x, (float)a_BR.y, (float)bottom_z);
                tverts->push_norm(0,0,-1); tverts->push_vert((float)prev_BR.x, (float)prev_BR.y, (float)bottom_z);
                
                // Fill Side Wall Gap (Right) - Quad as 2 triangles
                // Triangle 1
                Vectorf3 n1((float)(prev_TR.x - prev_center_top.x), (float)(prev_TR.y - prev_center_top.y), 0); // Approx normal
                tverts->push_norm(n1); tverts->push_vert((float)prev_TR.x, (float)prev_TR.y, (float)top_z);
                tverts->push_norm(n1); tverts->push_vert((float)prev_BR.x, (float)prev_BR.y, (float)bottom_z);
                tverts->push_norm(n1); tverts->push_vert((float)a_BR.x, (float)a_BR.y, (float)bottom_z);
                // Triangle 2
                Vectorf3 n2((float)(a_TR.x - a.x), (float)(a_TR.y - a.y), 0); // Approx normal
                tverts->push_norm(n2); tverts->push_vert((float)a_BR.x, (float)a_BR.y, (float)bottom_z);
                tverts->push_norm(n2); tverts->push_vert((float)a_TR.x, (float)a_TR.y, (float)top_z);
                tverts->push_norm(n2); tverts->push_vert((float)prev_TR.x, (float)prev_TR.y, (float)top_z);

            } else if (ccw < -EPSILON) {
                // Right Turn - Outer is Left side
                // Fill Top Gap
                tverts->push_norm(0,0,1); tverts->push_vert((float)prev_center_top.x, (float)prev_center_top.y, (float)top_z);
                tverts->push_norm(0,0,1); tverts->push_vert((float)a_TL.x, (float)a_TL.y, (float)top_z);
                tverts->push_norm(0,0,1); tverts->push_vert((float)prev_TL.x, (float)prev_TL.y, (float)top_z);

                // Fill Bottom Gap
                tverts->push_norm(0,0,-1); tverts->push_vert((float)prev_center_bot.x, (float)prev_center_bot.y, (float)bottom_z);
                tverts->push_norm(0,0,-1); tverts->push_vert((float)prev_BL.x, (float)prev_BL.y, (float)bottom_z);
                tverts->push_norm(0,0,-1); tverts->push_vert((float)a_BL.x, (float)a_BL.y, (float)bottom_z);
                
                // Fill Side Wall Gap (Left)
                // Triangle 1
                Vectorf3 n1((float)(prev_TL.x - prev_center_top.x), (float)(prev_TL.y - prev_center_top.y), 0);
                tverts->push_norm(n1); tverts->push_vert((float)prev_TL.x, (float)prev_TL.y, (float)top_z);
                tverts->push_norm(n1); tverts->push_vert((float)a_BL.x, (float)a_BL.y, (float)bottom_z);
                tverts->push_norm(n1); tverts->push_vert((float)prev_BL.x, (float)prev_BL.y, (float)bottom_z);
                // Triangle 2
                Vectorf3 n2((float)(a_TL.x - a.x), (float)(a_TL.y - a.y), 0);
                tverts->push_norm(n2); tverts->push_vert((float)a_BL.x, (float)a_BL.y, (float)bottom_z);
                tverts->push_norm(n2); tverts->push_vert((float)prev_TL.x, (float)prev_TL.y, (float)top_z);
                tverts->push_norm(n2); tverts->push_vert((float)a_TL.x, (float)a_TL.y, (float)top_z);
            }
        }
        
        if (first_done && i == 0) break;
        
        prev_line = line;
        prev_TR = b_TR;
        prev_TL = b_TL;
        prev_BR = b_BR;
        prev_BL = b_BL;
        prev_center_top = b;
        prev_center_bot = b;
        
        if (!closed) {
             if (i == 0) {
                // Start Cap (Back) - CCW
                qverts->push_norm(0,0,-1); qverts->push_vert((float)a_BL.x, (float)a_BL.y, (float)bottom_z);
                qverts->push_norm(0,0,-1); qverts->push_vert((float)a_BR.x, (float)a_BR.y, (float)bottom_z);
                qverts->push_norm(0,0,-1); qverts->push_vert((float)a_TR.x, (float)a_TR.y, (float)top_z);
                qverts->push_norm(0,0,-1); qverts->push_vert((float)a_TL.x, (float)a_TL.y, (float)top_z);
            }
            if (i == lines.size()-1) {
                // End Cap (Front) - CCW
                qverts->push_norm(0,0,-1); qverts->push_vert((float)b_TR.x, (float)b_TR.y, (float)top_z);
                qverts->push_norm(0,0,-1); qverts->push_vert((float)b_BR.x, (float)b_BR.y, (float)bottom_z);
                qverts->push_norm(0,0,-1); qverts->push_vert((float)b_BL.x, (float)b_BL.y, (float)bottom_z);
                qverts->push_norm(0,0,-1); qverts->push_vert((float)b_TL.x, (float)b_TL.y, (float)top_z);
            }
        }
        
        // Top Face (Up) - CCW: a_TL -> a_TR -> b_TR -> b_TL
        {
            qverts->push_norm(0,0,1); qverts->push_vert((float)a_TL.x, (float)a_TL.y, (float)top_z);
            qverts->push_norm(0,0,1); qverts->push_vert((float)a_TR.x, (float)a_TR.y, (float)top_z);
            qverts->push_norm(0,0,1); qverts->push_vert((float)b_TR.x, (float)b_TR.y, (float)top_z);
            qverts->push_norm(0,0,1); qverts->push_vert((float)b_TL.x, (float)b_TL.y, (float)top_z);
        }

        // Bottom Face (Down) - CCW: a_BR -> a_BL -> b_BL -> b_BR
        {
            qverts->push_norm(0,0,-1); qverts->push_vert((float)a_BR.x, (float)a_BR.y, (float)bottom_z);
            qverts->push_norm(0,0,-1); qverts->push_vert((float)a_BL.x, (float)a_BL.y, (float)bottom_z);
            qverts->push_norm(0,0,-1); qverts->push_vert((float)b_BL.x, (float)b_BL.y, (float)bottom_z);
            qverts->push_norm(0,0,-1); qverts->push_vert((float)b_BR.x, (float)b_BR.y, (float)bottom_z);
        }
        
        // Right Face (Right) - CCW: a_TR -> a_BR -> b_BR -> b_TR
        {
            qverts->push_norm(xy_right_normal); qverts->push_vert((float)a_TR.x, (float)a_TR.y, (float)top_z);
            qverts->push_norm(xy_right_normal); qverts->push_vert((float)a_BR.x, (float)a_BR.y, (float)bottom_z);
            qverts->push_norm(xy_right_normal); qverts->push_vert((float)b_BR.x, (float)b_BR.y, (float)bottom_z);
            qverts->push_norm(xy_right_normal); qverts->push_vert((float)b_TR.x, (float)b_TR.y, (float)top_z);
        }

        // Left Face (Left) - CCW: a_TL -> b_TL -> b_BL -> a_BL
        {
            qverts->push_norm(xy_left_normal); qverts->push_vert((float)a_TL.x, (float)a_TL.y, (float)top_z);
            qverts->push_norm(xy_left_normal); qverts->push_vert((float)b_TL.x, (float)b_TL.y, (float)top_z);
            qverts->push_norm(xy_left_normal); qverts->push_vert((float)b_BL.x, (float)b_BL.y, (float)bottom_z);
            qverts->push_norm(xy_left_normal); qverts->push_vert((float)a_BL.x, (float)a_BL.y, (float)bottom_z);
        }
        
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