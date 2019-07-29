
/**
 * Copyright (c) 2018, University Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University Osnabrück nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL University Osnabrück BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * KDTree.hpp
 *
 *  @date Apr 28, 2019
 *  @author Malte Hillmann
 */
#include <lvr2/registration/KDTree.hpp>

namespace lvr2
{

class KDNode : public KDTree
{
public:
    KDNode(int axis, float split, KDTreePtr& lesser, KDTreePtr& greater)
        : axis(axis), split(split), lesser(move(lesser)), greater(move(greater))
    { }

protected:
    virtual void nnInternal(const Vector3f& point, Vector3f*& neighbor, float& maxDist) const override
    {
        float val = point(this->axis);
        if (val < this->split)
        {
            this->lesser->nnInternal(point, neighbor, maxDist);
            if (val + maxDist >= this->split)
            {
                this->greater->nnInternal(point, neighbor, maxDist);
            }
        }
        else
        {
            this->greater->nnInternal(point, neighbor, maxDist);
            if (val - maxDist <= this->split)
            {
                this->lesser->nnInternal(point, neighbor, maxDist);
            }
        }
    }

private:
    int axis;
    float split;
    KDTreePtr lesser;
    KDTreePtr greater;
};

class KDLeaf : public KDTree
{
public:
    KDLeaf(Vector3f* points, int count)
        : points(points), count(count)
    { }

protected:
    virtual void nnInternal(const Vector3f& point, Vector3f*& neighbor, float& maxDist) const override
    {
        float maxDistSq = maxDist * maxDist;
        bool changed = false;
        for (int i = 0; i < this->count; i++)
        {
            float dist = (point - this->points[i]).squaredNorm();
            if (dist < maxDistSq)
            {
                neighbor = &this->points[i];
                maxDistSq = dist;
                changed = true;
            }
        }
        if (changed)
        {
            maxDist = sqrt(maxDistSq);
        }
    }

private:
    Vector3f* points;
    int count;
};

KDTreePtr create_recursive(Vector3f* points, int n, int maxLeafSize)
{
    if (n <= maxLeafSize)
    {
        return KDTreePtr(new KDLeaf(points, n));
    }

    AABB boundingBox(points, n);

    int splitAxis = boundingBox.longestAxis();
    float splitValue = boundingBox.avg(splitAxis);

    if (boundingBox.difference(splitAxis) == 0.0) // all points are exactly the same
    {
        // there is no need to check all of them later on, so just pretend like there is only one
        return KDTreePtr(new KDLeaf(points, 1));
    }

    int l = splitPoints(points, n, splitAxis, splitValue);

    KDTreePtr lesser, greater;

    #pragma omp task shared(lesser)
    lesser  = create_recursive(points    , l    , maxLeafSize);

    #pragma omp task shared(greater)
    greater = create_recursive(points + l, n - l, maxLeafSize);

    #pragma omp taskwait

    return KDTreePtr(new KDNode(splitAxis, splitValue, lesser, greater));
}

KDTreePtr KDTree::create(Vector3f* points, int n, int maxLeafSize)
{
    KDTreePtr ret;
    #pragma omp parallel // allows "pragma omp task"
    #pragma omp single // only execute every task once
    ret = create_recursive(points, n, maxLeafSize);

    return ret;
}

bool KDTree::nearestNeighbor(const Vector3f& point, Vector3f*& neighbor, float& distance, float maxDistance) const
{
    neighbor = nullptr;
    distance = maxDistance;
    nnInternal(point, neighbor, distance);

    return neighbor != nullptr;
}

size_t getNearestNeighbors(KDTreePtr tree, Vector3f* points, Vector3f** neighbors, size_t n, float maxDistance, Vector3f& centroid_m, Vector3f& centroid_d)
{
    size_t found = getNearestNeighbors(tree, points, neighbors, n, maxDistance);

    centroid_m = Vector3f::Zero();
    centroid_d = Vector3f::Zero();

    for (size_t i = 0; i < n; i++)
    {
        if (neighbors[i] != nullptr)
        {
            centroid_m += *neighbors[i];
            centroid_d += points[i];
        }
    }

    centroid_m /= found;
    centroid_d /= found;

    return found;
}

size_t getNearestNeighbors(KDTreePtr tree, Vector3f* points, Vector3f** neighbors, size_t n, float maxDistance)
{
    size_t found = 0;
    float distance = 0.0f;

    #pragma omp parallel for firstprivate(distance) reduction(+:found) schedule(dynamic,8)
    for (size_t i = 0; i < n; i++)
    {
        if (tree->nearestNeighbor(points[i], neighbors[i], distance, maxDistance))
        {
            found++;
        }
    }

    return found;
}

}
