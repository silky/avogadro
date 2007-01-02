/**********************************************************************
  WireframeEngine - Engine for wireframe display

  Copyright (C) 2006 by Geoffrey R. Hutchison
  Some portions Copyright (C) 2006 by Donald E. Curtis

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

  Some code is based on Open Babel
  For more information, see <http://openbabel.sourceforge.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

 This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "WireframeEngine.h"
#include "Primitives.h"
#include "color.h"
#include "vertexarray.h"

#include <openbabel/obiter.h>

#include <QtGui>

using namespace std;
using namespace OpenBabel;
using namespace Avogadro;

bool WireframeEngine::render(Atom *a)
{
   glPushAttrib(GL_ALL_ATTRIB_BITS);

   glPushName(atomType);
   glPushName(a->GetIdx());

  if (a->isSelected()) {
    Color( 0.3, 0.6, 1.0, 0.7 ).applyAsMaterials();
    glPointSize(etab.GetVdwRad(a->GetAtomicNum()) * 4.0);
    glBegin(GL_POINTS);
    glVertex3d(a->GetX(), a->GetY(), a->GetZ());
    glEnd();
  }
  else {
    Color(a).applyAsMaterials();
    glPointSize(etab.GetVdwRad(a->GetAtomicNum()) * 3.0);
    glBegin(GL_POINTS);
    glVertex3d(a->GetX(), a->GetY(), a->GetZ());
    glEnd();
  }

   glPopName();
   glPopName();
   glPopAttrib();

   return true;
}

bool WireframeEngine::render(Bond *b)
{
  OBAtom *atom1 = static_cast<OBAtom *>( b->GetBeginAtom() );
  OBAtom *atom2 = static_cast<OBAtom *>( b->GetEndAtom() );
  std::vector<double> rgb;

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glLineWidth(1.0);
  glBegin(GL_LINES);

  // hard to separate atoms from bonds in this view
  // so we let the user always select atoms
  glPushName( atomType);
  glPushName( atom1->GetIdx() );
  Color(atom1).applyAsMaterials();
  glVertex3d(atom1->GetX(), atom1->GetY(), atom1->GetZ());
  glPopName();

  glPushName( atom2->GetIdx() );
  Color(atom2).applyAsMaterials();
  glVertex3d(atom2->GetX(), atom2->GetY(), atom2->GetZ());
  glPopName();
  glPopName();

  glEnd();
  glPopAttrib();

  return true;
}

Q_EXPORT_PLUGIN2(WireframeEngine, WireframeEngineFactory)
