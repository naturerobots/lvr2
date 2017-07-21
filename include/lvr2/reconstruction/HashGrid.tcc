/*
 * HashGrid.cpp
 *
 *  Created on: Nov 27, 2014
 *      Author: twiemann
 */



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
 * HashGrid.cpp
 *
 *  Created on: 16.02.2011
 *      Author: Thomas Wiemann
 */
#include "HashGrid.hpp"
#include <lvr/geometry/BaseMesh.hpp>
#include <lvr/reconstruction/FastReconstructionTables.hpp>
// #include "SharpBox.hpp"
#include <lvr/io/Progress.hpp>

namespace lvr2
{

template<typename BaseVecT, typename BoxT>
HashGrid<BaseVecT, BoxT>::HashGrid(
    float cellSize,
    BoundingBox<BaseVecT> boundingBox,
    bool isVoxelsize,
    bool extrude
) :
    GridBase(extrude),
    m_boundingBox(boundingBox),
    m_globalIndex(0)
{
    m_coordinateScales = Vector<BaseVecT>(1, 1, 1);

    if(!m_boundingBox.isValid())
    {
        cout << lvr::timestamp << "Warning: Malformed BoundingBox." << endl;
    }

    if(!isVoxelsize)
    {
        m_voxelsize = (float) m_boundingBox.getLongestSide() / cellSize;
    }
    else
    {
        m_voxelsize = cellSize;
    }

    cout << lvr::timestamp << "Used voxelsize is " << m_voxelsize << endl;

    if(!m_extrude)
    {
        cout << lvr::timestamp << "Grid is not extruded." << endl;
    }


    BoxT::m_voxelsize = m_voxelsize;
    calcIndices();
}

template<typename BaseVecT, typename BoxT>
HashGrid<BaseVecT, BoxT>::HashGrid(string file)
{
    ifstream ifs(file.c_str());
    float minx, miny, minz, maxx, maxy, maxz, vsize;
    size_t qsize, csize;
    ifs >> minx >> miny >> minz >> maxx >> maxy >> maxz >> qsize >> vsize >> csize;

    m_boundingBox = BoundingBox<BaseVecT>(minx, miny, minz, maxx, maxy, maxz);
    m_globalIndex = 0;
    m_extrude = false; // TODO: ADD TO SERIALIZATION
    m_coordinateScales[0] = 1.0;
    m_coordinateScales[1] = 1.0;
    m_coordinateScales[2] = 1.0;
    m_voxelsize = vsize;
    BoxT::m_voxelsize = m_voxelsize;
    calcIndices();


    float  pdist;
    Point<BaseVecT> v;
    //cout << lvr::timestamp << "Creating Grid..." << endl;

    // Iterator over all points, calc lattice indices and add lattice points to the grid
    for(size_t i = 0; i < qsize; i++)
    {

        ifs >> v.x >> v.y >> v.z >> pdist;

        QueryPoint<BaseVecT> qp(v, pdist);
        m_queryPoints.push_back(qp);

    }
    //cout << lvr::timestamp << "read qpoints.. csize: " << csize << endl;
    size_t h;
    unsigned int cell[8];
    Point<BaseVecT> cell_center;
    for(size_t k = 0 ; k< csize ; k++)
    {
        //cout << "i: " << k << endl;
        ifs >> h >> cell[0] >> cell[1] >> cell[2] >> cell[3] >> cell[4] >> cell[5] >> cell[6] >> cell[7]
                 >> cell_center.x >> cell_center.y >> cell_center.z ;
        BoxT* box = new BoxT(cell_center);
        for(int j=0 ; j<8 ; j++)
        {
            box->setVertex(j,  cell[j]);
        }

        m_cells[h] = box;
    }
    cout << lvr::timestamp << "Reading cells.." << endl;
    typename HashGrid<BaseVecT, BoxT>::box_map_it it;
    typename HashGrid<BaseVecT, BoxT>::box_map_it neighbor_it;

    cout << "c size: " << m_cells.size() << endl;
    for( it = m_cells.begin() ; it != m_cells.end() ; it++)
    {
        //cout << "asdfsdfgsdfgdsfgdfg" << endl;
        BoxT* currentBox = it->second;
        int neighbor_index = 0;
        size_t neighbor_hash = 0;

        for(int a = -1; a < 2; a++)
        {
            for(int b = -1; b < 2; b++)
            {
                for(int c = -1; c < 2; c++)
                {


                    //Calculate hash value for current neighbor cell
                    neighbor_hash = this->hashValue(it->first + a,
                                                    it->first + b,
                                                    it->first + c);
                    //cout << "n hash: " << neighbor_hash  << endl;
                    //cout << " id: " << neighbor_index << endl;

                    //Try to find this cell in the grid
                    neighbor_it = this->m_cells.find(neighbor_hash);

                    //If it exists, save pointer in box
                    if(neighbor_it != this->m_cells.end())
                    {
                        currentBox->setNeighbor(neighbor_index, (*neighbor_it).second);
                        (*neighbor_it).second->setNeighbor(26 - neighbor_index, currentBox);
                    }

                    neighbor_index++;
                }
            }
        }
    }
    cout << "Finished reading grid" << endl;


}

template<typename BaseVecT, typename BoxT>
void HashGrid<BaseVecT, BoxT>::addLatticePoint(int index_x, int index_y, int index_z, float distance)
{
    size_t hash_value;

    unsigned int INVALID = BoxT::INVALID_INDEX;

    float vsh = 0.5 * this->m_voxelsize;

    // Some iterators for hash map accesses
    typename HashGrid<BaseVecT, BoxT>::box_map_it it;
    typename HashGrid<BaseVecT, BoxT>::box_map_it neighbor_it;

    // Values for current and global indices. Current refers to a
    // already present query point, global index is id that the next
    // created query point will get
    unsigned int current_index = 0;

    int dx, dy, dz;

    // Get min and max vertex of the point clouds bounding box
    auto v_min = this->m_boundingBox.getMin();
    auto v_max = this->m_boundingBox.getMax();

    int e;
    this->m_extrude ? e = 8 : e = 1;
    for(int j = 0; j < e; j++)
    {
        // Get the grid offsets for the neighboring grid position
        // for the given box corner
        dx = HGCreateTable[j][0];
        dy = HGCreateTable[j][1];
        dz = HGCreateTable[j][2];

        hash_value = this->hashValue(index_x + dx, index_y + dy, index_z +dz);

        it = this->m_cells.find(hash_value);
        if(it == this->m_cells.end())
        {
            //Calculate box center
            BaseVecT box_center(
                    (index_x + dx) * this->m_voxelsize + v_min.x,
                    (index_y + dy) * this->m_voxelsize + v_min.y,
                    (index_z + dz) * this->m_voxelsize + v_min.z);

            //Create new box
            BoxT* box = new BoxT(box_center);

            //Setup the box itself
            for(int k = 0; k < 8; k++){

                //Find point in Grid
                current_index = this->findQueryPoint(k, index_x + dx, index_y + dy, index_z + dz);
                //If point exist, save index in box
                if(current_index != INVALID) box->setVertex(k, current_index);

                    //Otherwise create new grid point and associate it with the current box
                else
                {
                    Point<BaseVecT> position(box_center.x + box_creation_table[k][0] * vsh,
                                     box_center.y + box_creation_table[k][1] * vsh,
                                     box_center.z + box_creation_table[k][2] * vsh);

                    this->m_queryPoints.push_back(QueryPoint<BaseVecT>(position, distance));
                    box->setVertex(k, this->m_globalIndex);
                    this->m_globalIndex++;

                }
            }

            //Set pointers to the neighbors of the current box
            int neighbor_index = 0;
            size_t neighbor_hash = 0;

            for(int a = -1; a < 2; a++)
            {
                for(int b = -1; b < 2; b++)
                {
                    for(int c = -1; c < 2; c++)
                    {

                        //Calculate hash value for current neighbor cell
                        neighbor_hash = this->hashValue(index_x + dx + a,
                                                        index_y + dy + b,
                                                        index_z + dz + c);

                        //Try to find this cell in the grid
                        neighbor_it = this->m_cells.find(neighbor_hash);

                        //If it exists, save pointer in box
                        if(neighbor_it != this->m_cells.end())
                        {
                            box->setNeighbor(neighbor_index, (*neighbor_it).second);
                            (*neighbor_it).second->setNeighbor(26 - neighbor_index, box);
                        }

                        neighbor_index++;
                    }
                }
            }

            this->m_cells[hash_value] = box;
        }
    }

}

template<typename BaseVecT, typename BoxT>
void HashGrid<BaseVecT, BoxT>::setCoordinateScaling(float x, float y, float z)
{
    m_coordinateScales[0] = x;
    m_coordinateScales[1] = y;
    m_coordinateScales[2] = z;
}

template<typename BaseVecT, typename BoxT>
HashGrid<BaseVecT, BoxT>::~HashGrid()
{}



template<typename BaseVecT, typename BoxT>
void HashGrid<BaseVecT, BoxT>::calcIndices()
{
    float max_size = m_boundingBox.getLongestSide();

    //Save needed grid parameters
    m_maxIndex = (int)ceil( (max_size + 5 * m_voxelsize) / m_voxelsize);
    m_maxIndexSquare = m_maxIndex * m_maxIndex;

    m_maxIndexX = (int)ceil(m_boundingBox.getXSize() / m_voxelsize) + 1;
    m_maxIndexY = (int)ceil(m_boundingBox.getYSize() / m_voxelsize) + 2;
    m_maxIndexZ = (int)ceil(m_boundingBox.getZSize() / m_voxelsize) + 3;
}

template<typename BaseVecT, typename BoxT>
unsigned int HashGrid<BaseVecT, BoxT>::findQueryPoint(
    int position,
    int x,
    int y,
    int z
)
{
    int n_x, n_y, n_z, q_v, offset;
    box_map_it it;

    for(int i = 0; i < 7; i++)
    {
        offset = i * 4;
        n_x = x + shared_vertex_table[position][offset];
        n_y = y + shared_vertex_table[position][offset + 1];
        n_z = z + shared_vertex_table[position][offset + 2];
        q_v = shared_vertex_table[position][offset + 3];

        size_t hash = hashValue(n_x, n_y, n_z);
        //cout << "i=" << i << " looking for hash: " << hash << endl;
        it = m_cells.find(hash);
        if(it != m_cells.end())
        {
        //  cout << "found hash" << endl;
            BoxT* b = it->second;
            if(b->getVertex(q_v) != BoxT::INVALID_INDEX) return b->getVertex(q_v);
        }
        //cout << "did not find hash" << endl;
    }

    return BoxT::INVALID_INDEX;
}


template<typename BaseVecT, typename BoxT>
void HashGrid<BaseVecT, BoxT>::saveGrid(string filename)
{
    cout << lvr::timestamp << "Writing grid..." << endl;

    // Open file for writing
    ofstream out(filename.c_str());

    // Write data
    if(out.good())
    {
        // Write header
        out << m_queryPoints.size() << " " << m_voxelsize << " " << m_cells.size() << endl;

        // Write query points and distances
        for(size_t i = 0; i < m_queryPoints.size(); i++)
        {
            out << m_queryPoints[i].m_position.x << " "
                    << m_queryPoints[i].m_position.y << " "
                    << m_queryPoints[i].m_position.z << " ";

            if(!isnan(m_queryPoints[i].m_distance))
            {
                out << m_queryPoints[i].m_distance << endl;
            }
            else
            {
                out << 0 << endl;
            }

        }

        // Write box definitions
        typename unordered_map<size_t, BoxT* >::iterator it;
        BoxT* box;
        for(it = m_cells.begin(); it != m_cells.end(); it++)
        {
            box = it->second;
            for(int i = 0; i < 8; i++)
            {
                out << box->getVertex(i) << " ";
            }
            out << endl;
        }
    }
}


template<typename BaseVecT, typename BoxT>
void HashGrid<BaseVecT, BoxT>::serialize(string file)
{
    ofstream out(file.c_str());

    // Write data
    if(out.good())
    {
        out <<    m_boundingBox.getMin().x << " " << m_boundingBox.getMin().y
        << " " << m_boundingBox.getMin().z << " " << m_boundingBox.getMax().x
        << " " << m_boundingBox.getMax().y << " " << m_boundingBox.getMax().z << endl;


        out << m_queryPoints.size() << " " << m_voxelsize << " " << m_cells.size() << endl;

        // Write query points and distances
        for(size_t i = 0; i < m_queryPoints.size(); i++)
        {
            out << m_queryPoints[i].m_position.x << " "
            << m_queryPoints[i].m_position.y << " "
            << m_queryPoints[i].m_position.z << " ";

            if(!isnan(m_queryPoints[i].m_distance))
            {
                out << m_queryPoints[i].m_distance << endl;
            }
            else
            {
                out << 0 << endl;
            }

        }

        // Write box definitions
        typename unordered_map<size_t, BoxT* >::iterator it;
        BoxT* box;
        for(it = m_cells.begin(); it != m_cells.end(); it++)
        {
            box = it->second;
            out << it->first << " ";
            for(int i = 0; i < 8; i++)
            {
                out << box->getVertex(i) << " ";
            }
            out << box->getCenter().x << " " << box->getCenter().y << " " << box->getCenter().z << endl;
        }
    }
    out.close();
}

} //namespace lvr2