/***************************************************************************
 *   Copyright (C) 2005 by Robot Group Leipzig                             *
 *    martius@informatik.uni-leipzig.de                                    *
 *    fhesse@informatik.uni-leipzig.de                                     *
 *    der@informatik.uni-leipzig.de                                        *
 *    frankguettler@gmx.de                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   $Log$
 *   Revision 1.2  2006-07-14 12:23:34  martius
 *   selforg becomes HEAD
 *
 *   Revision 1.1.2.2  2006/06/29 16:35:56  robot3
 *   includes cleared up
 *
 *   Revision 1.1.2.1  2006/03/06 16:51:04  robot3
 *   written an own viewer because getCurrentCameraManipulator() was not implemented
 *
 *                                                                         *
 ***************************************************************************/
/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2005 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
 */
#ifndef EXTENDEDVIEWER_H
#define EXTENDEDVIEWER_H

#include <osgProducer/Viewer>

namespace lpzrobots {

  /** A Producer-based viewer. Just like OpenGL, the core of OSG is independent of
   *  windowing system. The integration between OSG and some windowing system is
   *  delegated to other, non-core parts of OSG (users are also allowed to
   *  integrate OSG with any exotic windowing system they happen to use).
   *  \c Viewer implements the integration between OSG and Producer, AKA Open
   *  Producer (http://www.andesengineering.com/Producer), thus offering an
   *  out-of-the-box, scalable and multi-platform abstraction of the windowing
   *  system.
   */
  class ExtendedViewer : public osgProducer::Viewer {
    public :

    ExtendedViewer();
    
    ExtendedViewer(Producer::CameraConfig *cfg);

    ExtendedViewer(const std::string& configFile);

    ExtendedViewer(osg::ArgumentParser& arguments);

    virtual ~ExtendedViewer();

    /** Get the current active camera manipulator.*/
    virtual osgGA::MatrixManipulator* getCurrentCameraManipulator();

  };

}

#endif