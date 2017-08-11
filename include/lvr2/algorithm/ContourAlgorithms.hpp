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
 * ContourAlgorithms.hpp
 */

#ifndef LVR2_ALGORITHM_CONTOURALGORITHMS_H_
#define LVR2_ALGORITHM_CONTOURALGORITHMS_H_


#include <lvr2/geometry/BaseMesh.hpp>
#include <lvr2/geometry/Handles.hpp>

namespace lvr2
{

/**
 * @brief Walks on a boundary contour starting at `startH`.
 *
 * To make the method more generic, it has a `exists` function object which can
 * filter out faces. Each face for which `exists` returns `false` is treated as
 * if it wouldn't exist in the mesh at all! If you don't want to ignore faces,
 * you can just not provide the `exists` parameter to use the other overload.
 *
 * The given `startH` as well as all other edges on the contour mustn't be a
 * lonely edge (when ignoring faces according to `exists`).
 *
 * @param visitor A function object taking two parameters: a `VertexHandle`
 *                and an `EdgeHandle`. The vertex is the vertex of the edge
 *                that comes "before" the edge, speaking about the
 *                direction of visiting the edges.
 * @param exists A function object taking one FaceHandle as parameter and
 *               returning bool. This function decides whether a face should be
 *               treated as existing or not. This is mainly used to walk on the
 *               boundary of clusters. This can be achieved by making `exists`
 *               return `false` for all faces not in the cluster.
 */
template<typename BaseVecT, typename VisitorF, typename PredF>
void walkContour(
    const BaseMesh<BaseVecT>& mesh,
    EdgeHandle startH,
    VisitorF visitor,
    PredF exists
);

/**
 * @brief Like the other overload, but without ignoring any faces.
 */
template<typename BaseVecT, typename VisitorF>
void walkContour(
    const BaseMesh<BaseVecT>& mesh,
    EdgeHandle startH,
    VisitorF visitor
);

} // namespace lvr2

#include <lvr2/algorithm/ContourAlgorithms.tcc>

#endif /* LVR2_ALGORITHM_CONTOURALGORITHMS_H_ */
