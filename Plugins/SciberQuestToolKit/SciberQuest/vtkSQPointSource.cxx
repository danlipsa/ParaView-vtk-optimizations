/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQPointSource.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <float.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

vtkStandardNewMacro(vtkSQPointSource);

//----------------------------------------------------------------------------
vtkSQPointSource::vtkSQPointSource()
{
  this->NumberOfPoints=1;

  this->Center[0]=
  this->Center[1]=
  this->Center[2]=0.0;

  this->Radius=1;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkSQPointSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output
    = dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));


  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  if ( (pieceNo>=nPieces) || (pieceNo>=this->NumberOfPoints) )
    {
    output->Initialize();
    return 1;
    }

  // domain decomposition
  int nLocal=1;
  if (this->NumberOfPoints>nPieces)
    {
    int pieceSize=((int)this->NumberOfPoints)/nPieces;
    int nLarge=((int)this->NumberOfPoints)%nPieces;
    nLocal=pieceSize+(pieceNo<nLarge?1:0);
    }

  vtkFloatArray *pa=vtkFloatArray::New();
  pa->SetNumberOfComponents(3);
  pa->SetNumberOfTuples(nLocal);
  float *ppa=pa->GetPointer(0);

  vtkCellArray *cells=vtkCellArray::New();
  cells->Reserve(nLocal, 1);

  srand((unsigned int)pieceNo+(unsigned int)time(0));

  for (int i=0; i<nLocal; ++i)
    {
    float pi=3.14159265358979f;
    float rho=((float)this->Radius)*((float)rand())/((float)RAND_MAX);
    float theta=2.0f*pi*((float)rand())/((float)RAND_MAX);
    float phi=pi*((float)rand())/((float)RAND_MAX);
    float sin_theta=(float)sin(theta);
    float cos_theta=(float)cos(theta);
    float rho_sin_phi=rho*(float)sin(phi);
    ppa[0]=((float)this->Center[0])+rho_sin_phi*cos_theta;
    ppa[1]=((float)this->Center[1])+rho_sin_phi*sin_theta;
    ppa[2]=((float)this->Center[2])+rho*(float)cos(phi);
    ppa+=3;

    cells->InsertNextCell(1);
    cells->InsertCellPoint(i);
    }
  output->SetVerts(cells);

  vtkPoints *points=vtkPoints::New();
  points->SetData(pa);
  pa->Delete();
  output->SetPoints(points);
  points->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";
}
