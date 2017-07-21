/* Copyright (C) 2011 Uni Osnabrück
 * This file is part of the LAS VEGAS Reconstruction Toolkit,
 *
 * LAS VEGAS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LAS VEGAS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */


/*
 * Handles.hpp
 *
 *  @date 15.06.2017
 *  @author Lukas Kalbertodt <lukas.kalbertodt@gmail.com>
 */

#ifndef LVR2_GEOMETRY_HANDLES_H_
#define LVR2_GEOMETRY_HANDLES_H_

#include <cstdint>
#include <functional>

#include "BaseHandle.hpp"

namespace lvr2
{

/**
 * @brief Datatype used as index for each vertex, face and edge.
 *
 * This index is used within {Edge, Face, Vertex}-Handles. Since those
 * handles are also used within each {Edge, Face, Vertex} reducing the
 * size of this type can greatly decrease memory usage, which in
 * turn might increase performance due to cache locality.
 *
 * When we assume a basic half-edge structure, we have to deal with the
 * following struct sizes:
 * - Edge: 4 handles
 * - Face: 1 handle + 1 vector
 * - Vertex: 1 handle + 1 vector
 *
 * Assuming the most common case of `float` vectors, this results in the
 * following sizes (in bytes):
 * - 16 bit handles: Edge (8), Face (14), Vertex (14)
 * - 32 bit handles: Edge (16), Face (16), Vertex (16)
 * - 64 bit handles: Edge (32), Face (20), Vertex (20)
 *
 * Using another approximation of the number of faces, edges and vertices
 * in a triangle-mesh described at [1], we can calculate how much RAM we
 * would need in order to run out of handles. The approximation: for each
 * vertex, we have three edges and two faces. The de-facto cost per vertex
 * can be calculated from that resulting in
 *
 * - 16 bit handles: 14 + 2*14 + 3*8 = 66 bytes/vertex = 22 bytes/edge
 *   ==> 22 * 2^16 = 1.4 MiB RAM necessary to exhaust handle space
 *
 * - 32 bit handles: 16 + 2*16 + 3*16 = 96 bytes/vertex = 32 bytes/edge
 *   ==> 32 * 2^32 = 137 GiB RAM necessary to exhaust handle space
 *
 * - 16 bit handles: 20 + 2*20 + 3*32 = 156 bytes/vertex = 52 bytes/edge
 *   ==> 52 * 2^64 = 1.1 ZiB RAM necessary to exhaust handle space
 *       (it's called zetta or zebi and is ≈ 1 million tera bytes)
 *   ==> Note: funnily enough, the estimated disk (not RAM!) capacity of
 *       the whole  world (around 2015) comes very close to this number.
 *
 *
 * Also note that this accounts for the mesh only and ignores all other
 * data that might need to be stored in RAM. So you will need even more
 * RAM.
 *
 * From this, I think, we can safely conclude: 16 bit handles are way too
 * small; 32 bit handles are probably fine for the next few years, even
 * when working on a medium-sized cluster and 64 bit handles will be fine
 * until after the singularity. And by then, I probably don't care anymore.
 *
 * [1]: https://math.stackexchange.com/q/425968/340615
 */
using Index = uint32_t;

/// Handle to access edges of the mesh.
class EdgeHandle : public BaseHandle<Index>
{
    using BaseHandle<Index>::BaseHandle;
};

/// Handle to access faces of the mesh.
class FaceHandle : public BaseHandle<Index>
{
    using BaseHandle<Index>::BaseHandle;
};

/// Handle to access vertices of the mesh.
class VertexHandle : public BaseHandle<Index>
{
    using BaseHandle<Index>::BaseHandle;
};

/// Semantically equivalent to `boost::optional<EdgeHandle>`
class OptionalEdgeHandle : public BaseOptionalHandle<Index, EdgeHandle>
{
    using BaseOptionalHandle<Index, EdgeHandle>::BaseOptionalHandle;
};

/// Semantically equivalent to `boost::optional<FaceHandle>`
class OptionalFaceHandle : public BaseOptionalHandle<Index, FaceHandle>
{
    using BaseOptionalHandle<Index, FaceHandle>::BaseOptionalHandle;
};

/// Semantically equivalent to `boost::optional<VertexHandle>`
class OptionalVertexHandle : public BaseOptionalHandle<Index, VertexHandle>
{
    using BaseOptionalHandle<Index, VertexHandle>::BaseOptionalHandle;
};

std::ostream& operator<<(std::ostream& os, const EdgeHandle& h)
{
    os << "E" << h.idx();
    return os;
}

std::ostream& operator<<(std::ostream& os, const FaceHandle& h)
{
    os << "F" << h.idx();
    return os;
}

std::ostream& operator<<(std::ostream& os, const VertexHandle& h)
{
    os << "V" << h.idx();
    return os;
}

std::ostream& operator<<(std::ostream& os, const OptionalEdgeHandle& h)
{
    if (h)
    {
        os << "E" << h.unwrap().idx();
    }
    else
    {
        os << "E⊥";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const OptionalFaceHandle& h)
{
    if (h)
    {
        os << "F" << h.unwrap().idx();
    }
    else
    {
        os << "F⊥";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const OptionalVertexHandle& h)
{
    if (h)
    {
        os << "V" << h.unwrap().idx();
    }
    else
    {
        os << "V⊥";
    }
    return os;
}

} // namespace lvr2

namespace std
{

template<>
struct hash<lvr2::VertexHandle> {
    size_t operator()(const lvr2::VertexHandle& h) const
    {
        return std::hash<lvr2::Index>()(h.idx());
    }
};

template<>
struct hash<lvr2::EdgeHandle> {
    size_t operator()(const lvr2::EdgeHandle& h) const
    {
        return std::hash<lvr2::Index>()(h.idx());
    }
};

template<>
struct hash<lvr2::FaceHandle> {
    size_t operator()(const lvr2::FaceHandle& h) const
    {
        return std::hash<lvr2::Index>()(h.idx());
    }
};

} // namespace std

#endif /* LVR2_GEOMETRY_HANDLES_H_ */