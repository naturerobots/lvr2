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

#ifndef TIMERCALLBACK_HPP_
#define TIMERCALLBACK_HPP_

#include <vtkSmartPointer.h>
#include <vtkCamera.h>
#include <vtkCameraRepresentation.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkWindowToImageFilter.h>
#include <vtkFFMPEGWriter.h>
#include <vtkTesting.h>

namespace lvr
{

class LVRTimerCallback : public vtkCommand
{
public:
    static LVRTimerCallback* New();
    void setWindowToImageFilter(vtkSmartPointer<vtkWindowToImageFilter> w2i);
    void setFFMPEGWriter(vtkSmartPointer<vtkFFMPEGWriter> videoWriter);
    void Execute(vtkObject* caller, unsigned long eventId, void* callData);
private:
    vtkSmartPointer<vtkWindowToImageFilter> m_w2i;
    vtkSmartPointer<vtkFFMPEGWriter>        m_videoWriter;
};

} /* namespace lvr */

#endif /* TIMERCALLBACK_HPP_ */
