/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightReaderInterface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVEnSightReaderInterface.h"
#include "vtkGenericEnSightReader.h"
#include <ctype.h>

int vtkPVEnSightReaderInterfaceCommand(ClientData cd, Tcl_Interp *interp,
				       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVEnSightReaderInterface::vtkPVEnSightReaderInterface()
{
  this->CommandFunction = vtkPVEnSightReaderInterfaceCommand;
  this->CaseFileName = NULL;
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderInterface* vtkPVEnSightReaderInterface::New()
{
  return new vtkPVEnSightReaderInterface();
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVEnSightReaderInterface::CreateCallback()
{
  char tclName[100], outputTclName[100], srcTclName[100];
  vtkDataSet *d;
  vtkPVData *pvd;
  vtkGenericEnSightReader *reader;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numOutputs, i;
  char fullPath[256];
  char *extension;
  int position;
  char *endingSlash = NULL;
  char *newTclName;
  char *result;
  
  // Create the vtkSource.
  if (!this->GetDataFileName())
    {
    pvApp->Script("set newFileName [tk_getOpenFile -filetypes {{{EnSight files} {.case}} {{All Files} {.*}}}]");
    result = pvApp->GetMainInterp()->result;
    if (strcmp(result, "") == 0)
      {
      return NULL;
      }
    this->SetDataFileName(result);
    }
  
  extension = strrchr(this->DataFileName, '.');
  position = extension - this->DataFileName;
  strncpy(tclName, this->DataFileName, position);
  tclName[position] = '\0';
  
  if ((endingSlash = strrchr(tclName, '/')))
    {
    position = endingSlash - tclName + 1;
    newTclName = new char[strlen(tclName) - position + 1];
    strcpy(newTclName, tclName + position);
    strcpy(tclName, "");
    strcat(tclName, newTclName);
    delete [] newTclName;
    }
  if (isdigit(tclName[0]))
    {
    // A VTK object names beginning with a digit is invalid.
    newTclName = new char[strlen(tclName) + 3];
    sprintf(newTclName, "PV%s", tclName);
    strcpy(tclName, "");
    strcat(tclName, newTclName);
    delete [] newTclName;
    }

  sprintf(tclName, "%s%d", tclName, this->InstanceCount);
  
  // Create the object through tcl on all processes.
  reader = (vtkGenericEnSightReader *)
    (pvApp->MakeTclObject(this->SourceClassName, tclName));
  if (reader == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  pvApp->Script("%s SetCaseFileName %s", tclName, this->GetDataFileName());

  if (strcmp(reader->GetCaseFileName(), "") == 0)
    {
    pvApp->BroadcastScript("%s Delete", tclName);
    return NULL;
    }

  pvApp->BroadcastScript("%s SetFilePath %s", tclName,
			 reader->GetFilePath());
  pvApp->BroadcastScript("%s SetCaseFileName %s", tclName,
			 reader->GetCaseFileName());
  sprintf(fullPath, "%s%s", reader->GetFilePath(), reader->GetCaseFileName());
  this->SetCaseFileName(fullPath);
  
  pvApp->BroadcastScript("%s Update", tclName);
  
  numOutputs = reader->GetNumberOfOutputs();
  
  for (i = 0; i < numOutputs; i++)
    {
    sprintf(outputTclName, "%sOutput%d", tclName, i);
    d = (vtkDataSet*)(pvApp->MakeTclObject(reader->GetOutput(i)->GetClassName(),
					   outputTclName));
    pvApp->BroadcastScript("%s ShallowCopy [%s GetOutput %d]",
			   outputTclName, tclName, i);
    pvd = vtkPVData::New();
    pvd->SetApplication(pvApp);
    pvd->SetVTKData(d, outputTclName);

    pvs = vtkPVSource::New();
    pvs->SetPropertiesParent(this->PVWindow->GetMainView()->GetPropertiesParent());
    pvs->SetApplication(pvApp);
    pvs->SetInterface(this);

    this->PVWindow->GetMainView()->AddComposite(pvs);
    pvs->CreateProperties();
    this->PVWindow->SetCurrentPVSource(pvs);

    sprintf(srcTclName, "%s_%d", tclName, i+1);
    pvs->SetName(srcTclName);
    pvs->SetNthPVOutput(0, pvd);

    pvs->AcceptCallback();
    
    pvs->Delete();
    pvd->Delete();
    }

  pvApp->BroadcastScript("%s Delete", tclName);
  
  ++this->InstanceCount;
  return pvs;
}

void vtkPVEnSightReaderInterface::Save(ofstream *file, const char *sourceName)
{
  static int sourceCount = 0;
  
  if (sourceCount == 0)
    {
    *file << "vtkGenericEnSightReader " << sourceName << "\n";
    *file << "\t" << sourceName << " SetCaseFileName " 
          << this->CaseFileName << "\n";
    *file << "\t" << sourceName << " Update\n\n";
    sourceCount++;
    }
}
