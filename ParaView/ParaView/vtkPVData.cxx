/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVData.h"

#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPVProcessModule.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkCollection.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkImageData.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVColorMap.h"
#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkTreeComposite.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVArrayInformation.h"

// Just for the definition of VTK_POINT_DATA_FIELD ...
#include "vtkFieldDataToAttributeDataFilter.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVData);
vtkCxxRevisionMacro(vtkPVData, "1.209.2.2");

int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  static int instanceCount = 0;

  this->CommandFunction = vtkPVDataCommand;

  this->PVSource = NULL;
  
  this->PropertiesCreated = 0;
  
  this->PropertiesParent = NULL; 

  this->CubeAxesTclName = NULL;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
    
  this->Properties = vtkKWFrame::New();
  this->InformationFrame = vtkKWFrame::New();

  this->ColorFrame = vtkKWLabeledFrame::New();
  this->DisplayStyleFrame = vtkKWLabeledFrame::New();
  this->StatsFrame = vtkKWLabeledFrame::New();
  this->ViewFrame = vtkKWLabeledFrame::New();
  
  this->TypeLabel = vtkKWLabel::New();
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  this->BoundsDisplay->ShowHideFrameOn();
  
  this->ExtentDisplay = vtkKWBoundsDisplay::New();
  this->ExtentDisplay->ShowHideFrameOn();
  
  this->AmbientScale = vtkKWScale::New();

  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->MapScalarsCheck = vtkKWCheckButton::New();
  this->EditColorMapButton = vtkKWPushButton::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWOptionMenu::New();
  
  this->PointSizeLabel = vtkKWLabel::New();
  this->PointSizeThumbWheel = vtkKWThumbWheel::New();
  this->LineWidthLabel = vtkKWLabel::New();
  this->LineWidthThumbWheel = vtkKWThumbWheel::New();
  
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();

  this->VisibilityCheck = vtkKWCheckButton::New();
  this->Visibility = 1;

  this->ResetCameraButton = vtkKWPushButton::New();

  this->ActorControlFrame = vtkKWLabeledFrame::New();
  this->TranslateLabel = vtkKWLabel::New();
  this->ScaleLabel = vtkKWLabel::New();
  this->OrientationLabel = vtkKWLabel::New();
  this->OriginLabel = vtkKWLabel::New();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc] = vtkKWThumbWheel::New();
    this->ScaleThumbWheel[cc] = vtkKWThumbWheel::New();
    this->OrientationScale[cc] = vtkKWScale::New();
    this->OriginThumbWheel[cc] = vtkKWThumbWheel::New();
    }

  this->OpacityLabel = vtkKWLabel::New();
  this->OpacityScale = vtkKWScale::New();
  
  this->PreviousAmbient = 0.0;
  this->PreviousSpecular = 0.1;
  this->PreviousDiffuse = 1.0;
  this->PreviousWasSolid = 1;

  this->PVColorMap = NULL;

  this->ColorSetByUser = 0;
}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{  
  if (this->PVColorMap)
    {
    // Use count to manage color map visibility.
    if (this->Visibility)
      {
      this->PVColorMap->DecrementUseCount();
      }
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = 0;
    }

  this->SetPVSource(NULL);

  // Used to be in vtkPVActorComposite........
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->PropertiesParent)
    {
    this->PropertiesParent->UnRegister(this);
    this->PropertiesParent = NULL;
    }
    
  this->TypeLabel->Delete();
  this->TypeLabel = NULL;
  
  this->NumCellsLabel->Delete();
  this->NumCellsLabel = NULL;
  
  this->NumPointsLabel->Delete();
  this->NumPointsLabel = NULL;
  
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;
  
  this->ExtentDisplay->Delete();
  this->ExtentDisplay = NULL;
  
  this->AmbientScale->Delete();
  this->AmbientScale = NULL;
  
  this->ColorMenuLabel->Delete();
  this->ColorMenuLabel = NULL;
  
  this->ColorMenu->Delete();
  this->ColorMenu = NULL;

  this->EditColorMapButton->Delete();
  this->EditColorMapButton = NULL;

  this->MapScalarsCheck->Delete();
  this->MapScalarsCheck = NULL;  
    
  this->ColorButton->Delete();
  this->ColorButton = NULL;
  
  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->InterpolationMenuLabel->Delete();
  this->InterpolationMenuLabel = NULL;
  this->InterpolationMenu->Delete();
  this->InterpolationMenu = NULL;
  
  this->PointSizeLabel->Delete();
  this->PointSizeLabel = NULL;
  this->PointSizeThumbWheel->Delete();
  this->PointSizeThumbWheel = NULL;
  this->LineWidthLabel->Delete();
  this->LineWidthLabel = NULL;
  this->LineWidthThumbWheel->Delete();
  this->LineWidthThumbWheel = NULL;

  this->ActorControlFrame->Delete();
  this->TranslateLabel->Delete();
  this->ScaleLabel->Delete();
  this->OrientationLabel->Delete();
  this->OriginLabel->Delete();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->Delete();
    this->ScaleThumbWheel[cc]->Delete();
    this->OrientationScale[cc]->Delete();
    this->OriginThumbWheel[cc]->Delete();
    }

  this->OpacityLabel->Delete();
  this->OpacityScale->Delete();
 
  if (this->CubeAxesTclName)
    {
    if ( pvApp )
      {
      pvApp->Script("%s Delete", this->CubeAxesTclName);
      }
    this->SetCubeAxesTclName(NULL);
    }
  
 
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;  
  
  this->CubeAxesCheck->Delete();
  this->CubeAxesCheck = NULL;
  
  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  this->ColorFrame->Delete();
  this->ColorFrame = NULL;
  this->DisplayStyleFrame->Delete();
  this->DisplayStyleFrame = NULL;
  this->StatsFrame->Delete();
  this->StatsFrame = NULL;
  this->ViewFrame->Delete();
  this->ViewFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;

  this->Properties->Delete();
  this->Properties = NULL;

  this->InformationFrame->Delete();
  this->InformationFrame = NULL;
}



//----------------------------------------------------------------------------
void vtkPVData::SetVisibilityCheckState(int v)
{
  vtkPVApplication *pvApp;
  pvApp = (vtkPVApplication*)(this->Application);

  if (this->Visibility != v)
    {
    this->Visibility = v;
    // Use count to manage color map visibility.
    if (this->PVColorMap)
      {
      if (v)
        {
        this->PVColorMap->IncrementUseCount();
        }
      else
        {
        this->PVColorMap->DecrementUseCount();
        }
      }
    }

  if (this->VisibilityCheck->GetApplication())
    {
    if (this->VisibilityCheck->GetState() != v)
      {
      this->VisibilityCheck->SetState(v);
      }
    }
}



//----------------------------------------------------------------------------
void vtkPVData::SetPVColorMap(vtkPVColorMap *colorMap)
{
  if (this->PVColorMap == colorMap)
    {
    return;
    }

  if (this->PVColorMap)
    {
    // Use count to manage color map visibility.
    if (this->Visibility)
      {
      this->PVColorMap->DecrementUseCount();
      }
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = NULL;
    }

  this->PVColorMap = colorMap;
  if (this->PVColorMap)
    {
    if (this->Visibility)
      {
      this->PVColorMap->IncrementUseCount();
      }
    this->PVColorMap->Register(this);
    
    if (this->ScalarBarCheck->IsCreated())
      {
      // Let's make those 2 chekbuttons use the same variable name
      this->ScalarBarCheck->SetVariableName(
        this->PVColorMap->GetScalarBarCheck()->GetVariableName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->vtkKWObject::SetApplication(pvApp);
  
  // Nothing to do here since we created vtkPVPart.  
  // I did not remove method in case we need to create Cube Axes in parallel.
  // I am having issues rendering on client.  I do not want to send
  // the zbuffer.
}

//----------------------------------------------------------------------------
void vtkPVData::DeleteCallback()
{
  this->SetCubeAxesVisibility(0);
}

//----------------------------------------------------------------------------
void vtkPVData::SetPVApplication(vtkPVApplication *pvApp)
{
  this->CreateParallelTclObjects(pvApp);
  this->vtkKWObject::SetApplication(pvApp);
}




//----------------------------------------------------------------------------
// WE DO NOT REFERENCE COUNT HERE BECAUSE OF CIRCULAR REFERENCES.
// THE SOURCE OWNS THE DATA.
void vtkPVData::SetPVSource(vtkPVSource *source)
{
  if (this->PVSource == source)
    {
    return;
    }
  this->Modified();

  this->PVSource = source;

  this->SetTraceReferenceObject(source);
  this->SetTraceReferenceCommand("GetPVOutput");
}





// ============= Use to be in vtkPVActorComposite ===================

//----------------------------------------------------------------------------
void vtkPVData::CreateProperties()
{
  if (this->PropertiesCreated)
    {
    return;
    }

  // Properties (aka Display page)

  if (this->GetPVSource()->GetHideDisplayPage())
    {
    // We use the parameters frame as a bogus parent.
    // This is not a problem since we never pack the
    // properties frame in this case.
    this->Properties->SetParent(
      this->GetPVSource()->GetParametersParent());
    }
  else
    {
    this->Properties->SetParent(
      this->GetPVSource()->GetNotebook()->GetFrame("Display"));
    }
  this->Properties->ScrollableOn();
  this->Properties->Create(this->Application,0);

  // We are going to 'grid' most of it, so let's define some const

  int col_1_padx = 2;
  int button_pady = 1;
  int col_0_weight = 0;
  int col_1_weight = 1;
  float col_0_factor = 1.5;
  float col_1_factor = 1.0;

  // View frame

  this->ViewFrame->SetParent(this->Properties->GetFrame());
  this->ViewFrame->ShowHideFrameOn();
  this->ViewFrame->Create(this->Application, 0);
  this->ViewFrame->SetLabel("View");
 
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create(this->Application, "-text Data");
  this->Application->Script(
    "%s configure -command {%s VisibilityCheckCallback}",
    this->VisibilityCheck->GetWidgetName(),
    this->GetTclName());
  this->VisibilityCheck->SetState(1);
  this->VisibilityCheck->SetBalloonHelpString(
    "Toggle the visibility of this dataset's geometry.");

  this->ResetCameraButton->SetParent(this->ViewFrame->GetFrame());
  this->ResetCameraButton->Create(this->Application, "");
  this->ResetCameraButton->SetLabel("Set View to Data");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");
  this->ResetCameraButton->SetBalloonHelpString(
    "Change the camera location to best fit the dataset in the view window.");

  this->ScalarBarCheck->SetParent(this->ViewFrame->GetFrame());
  this->ScalarBarCheck->Create(this->Application, "-text {Scalar bar}");
  this->ScalarBarCheck->SetBalloonHelpString(
    "Toggle the visibility of the scalar bar for this data.");
  this->Application->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->CubeAxesCheck->SetParent(this->ViewFrame->GetFrame());
  this->CubeAxesCheck->Create(this->Application, "-text CubeAxes");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  this->CubeAxesCheck->SetBalloonHelpString(
    "Toggle the visibility of X,Y,Z scales for this dataset.");

  this->Script("grid %s %s -sticky wns",
               this->VisibilityCheck->GetWidgetName(),
               this->ResetCameraButton->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ResetCameraButton->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -sticky wns",
               this->ScalarBarCheck->GetWidgetName());

  this->Script("grid %s -sticky wns",
               this->CubeAxesCheck->GetWidgetName());
  // Color

  this->ColorFrame->SetParent(this->Properties->GetFrame());
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->Application, 0);
  this->ColorFrame->SetLabel("Color");

  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->Application, "");
  this->ColorMenuLabel->SetLabel("Color by:");
  this->ColorMenuLabel->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");
  
  this->ColorMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenu->Create(this->Application, "");   
  this->ColorMenu->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->SetText("Actor Color");
  this->ColorButton->Create(this->Application, "");
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  this->ColorButton->SetBalloonHelpString(
    "Edit the constant color for the geometry.");


  this->MapScalarsCheck->SetParent(this->ColorFrame->GetFrame());
  this->MapScalarsCheck->Create(this->Application, "-text {Map Scalars}");
  this->MapScalarsCheck->SetState(0);
  this->MapScalarsCheck->SetBalloonHelpString(
    "Pass attriubte through color map or use unsigned char values as color.");
  this->Application->Script(
    "%s configure -command {%s MapScalarsCheckCallback}",
    this->MapScalarsCheck->GetWidgetName(),
    this->GetTclName());

  this->EditColorMapButton->SetParent(this->ColorFrame->GetFrame());
  this->EditColorMapButton->Create(this->Application, "");
  this->EditColorMapButton->SetLabel("Edit Color Map...");
  this->EditColorMapButton->SetCommand(this,"EditColorMapCallback");
  this->EditColorMapButton->SetBalloonHelpString(
    "Edit the table used to map data attributes to pseudo colors.");

  
  this->Script("grid %s %s -sticky wns",
               this->ColorMenuLabel->GetWidgetName(),
               this->ColorMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ColorMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->ColorButton->GetWidgetName(),
               col_1_padx, button_pady);
  this->ColorButton->EnabledOff();

  this->Script("grid %s %s -sticky wns",
               this->MapScalarsCheck->GetWidgetName(),
               this->EditColorMapButton->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->EditColorMapButton->GetWidgetName(),
               col_1_padx, button_pady);
  this->MapScalarsCheck->EnabledOff();
  this->EditColorMapButton->EnabledOff();

  // Display style

  this->DisplayStyleFrame->SetParent(this->Properties->GetFrame());
  this->DisplayStyleFrame->ShowHideFrameOn();
  this->DisplayStyleFrame->Create(this->Application, 0);
  this->DisplayStyleFrame->SetLabel("Display Style");

  this->AmbientScale->SetParent(this->Properties->GetFrame());
  this->AmbientScale->Create(this->Application, "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  
  this->RepresentationMenuLabel->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuLabel->Create(this->Application, "");
  this->RepresentationMenuLabel->SetLabel("Representation:");

  this->RepresentationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenu->Create(this->Application, "");
  this->RepresentationMenu->AddEntryWithCommand("Wireframe", this,
                                                "DrawWireframe");
  this->RepresentationMenu->AddEntryWithCommand("Surface", this,
                                                "DrawSurface");
  this->RepresentationMenu->AddEntryWithCommand("Points", this,
                                                "DrawPoints");
  this->RepresentationMenu->SetValue("Surface");
  this->RepresentationMenu->SetBalloonHelpString(
    "Choose what geometry should be used to represent the dataset.");

  this->InterpolationMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenuLabel->Create(this->Application, "");
  this->InterpolationMenuLabel->SetLabel("Interpolation:");

  this->InterpolationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenu->Create(this->Application, "");
  this->InterpolationMenu->AddEntryWithCommand("Flat", this,
                                               "SetInterpolationToFlat");
  this->InterpolationMenu->AddEntryWithCommand("Gouraud", this,
                                               "SetInterpolationToGouraud");
  this->InterpolationMenu->SetValue("Gouraud");
  this->InterpolationMenu->SetBalloonHelpString(
    "Choose the method used to shade the geometry and interpolate point attributes.");

  this->PointSizeLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeLabel->Create(this->Application, "");
  this->PointSizeLabel->SetLabel("Point size:");
  this->PointSizeLabel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->PointSizeThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeThumbWheel->PopupModeOn();
  this->PointSizeThumbWheel->SetValue(1.0);
  this->PointSizeThumbWheel->SetResolution(1.0);
  this->PointSizeThumbWheel->SetMinimumValue(1.0);
  this->PointSizeThumbWheel->ClampMinimumValueOn();
  this->PointSizeThumbWheel->Create(this->Application, "");
  this->PointSizeThumbWheel->DisplayEntryOn();
  this->PointSizeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PointSizeThumbWheel->SetBalloonHelpString("Set the point size.");
  this->PointSizeThumbWheel->GetEntry()->SetWidth(5);
  this->PointSizeThumbWheel->SetCommand(this, "ChangePointSize");
  this->PointSizeThumbWheel->SetEndCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetEntryCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->LineWidthLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthLabel->Create(this->Application, "");
  this->LineWidthLabel->SetLabel("Line width:");
  this->LineWidthLabel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");
  
  this->LineWidthThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthThumbWheel->PopupModeOn();
  this->LineWidthThumbWheel->SetValue(1.0);
  this->LineWidthThumbWheel->SetResolution(1.0);
  this->LineWidthThumbWheel->SetMinimumValue(1.0);
  this->LineWidthThumbWheel->ClampMinimumValueOn();
  this->LineWidthThumbWheel->Create(this->Application, "");
  this->LineWidthThumbWheel->DisplayEntryOn();
  this->LineWidthThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->LineWidthThumbWheel->SetBalloonHelpString("Set the line width.");
  this->LineWidthThumbWheel->GetEntry()->SetWidth(5);
  this->LineWidthThumbWheel->SetCommand(this, "ChangeLineWidth");
  this->LineWidthThumbWheel->SetEndCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetEntryCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");

  this->Script("grid %s %s -sticky wns",
               this->RepresentationMenuLabel->GetWidgetName(),
               this->RepresentationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->RepresentationMenu->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->InterpolationMenuLabel->GetWidgetName(),
               this->InterpolationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->InterpolationMenu->GetWidgetName(),
               col_1_padx, button_pady);
  
  this->Script("grid %s %s -sticky wns",
               this->PointSizeLabel->GetWidgetName(),
               this->PointSizeThumbWheel->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->PointSizeThumbWheel->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->LineWidthLabel->GetWidgetName(),
               this->LineWidthThumbWheel->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->LineWidthThumbWheel->GetWidgetName(),
               col_1_padx, button_pady);

  // Now synchronize all those grids to have them aligned

  const char *widgets[3];
  widgets[0] = this->ViewFrame->GetFrame()->GetWidgetName();
  widgets[1] = this->ColorFrame->GetFrame()->GetWidgetName();
  widgets[2] = this->DisplayStyleFrame->GetFrame()->GetWidgetName();

  int weights[2];
  weights[0] = col_0_weight;
  weights[1] = col_1_weight;

  float factors[2];
  factors[0] = col_0_factor;
  factors[1] = col_1_factor;

  vtkKWTkUtilities::SynchroniseGridsColumnMinimumSize(
    this->GetPVApplication()->GetMainInterp(), 3, widgets, factors, weights);
  
  // Actor Control

  this->ActorControlFrame->SetParent(this->Properties->GetFrame());
  this->ActorControlFrame->ShowHideFrameOn();
  this->ActorControlFrame->Create(this->Application, 0);
  this->ActorControlFrame->SetLabel("Actor Control");

  this->TranslateLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->TranslateLabel->Create(this->Application, 0);
  this->TranslateLabel->SetLabel("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->ScaleLabel->Create(this->Application, 0);
  this->ScaleLabel->SetLabel("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OrientationLabel->Create(this->Application, 0);
  this->OrientationLabel->SetLabel("Orientation:");
  this->OrientationLabel->SetBalloonHelpString(
    "Orient the geometry relative to the dataset origin.");

  this->OriginLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OriginLabel->Create(this->Application, 0);
  this->OriginLabel->SetLabel("Origin:");
  this->OriginLabel->SetBalloonHelpString(
    "Set the origin point about which rotations take place.");

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->TranslateThumbWheel[cc]->PopupModeOn();
    this->TranslateThumbWheel[cc]->SetValue(0.0);
    this->TranslateThumbWheel[cc]->Create(this->Application, 0);
    this->TranslateThumbWheel[cc]->DisplayEntryOn();
    this->TranslateThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->TranslateThumbWheel[cc]->ExpandEntryOn();
    this->TranslateThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->TranslateThumbWheel[cc]->SetCommand(this, "ActorTranslateCallback");
    this->TranslateThumbWheel[cc]->SetEndCommand(this, 
                                                 "ActorTranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetEntryCommand(this,
                                                   "ActorTranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetBalloonHelpString(
      "Translate the geometry relative to the dataset location.");

    this->ScaleThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->ScaleThumbWheel[cc]->PopupModeOn();
    this->ScaleThumbWheel[cc]->SetValue(1.0);
    this->ScaleThumbWheel[cc]->SetMinimumValue(0.0);
    this->ScaleThumbWheel[cc]->ClampMinimumValueOn();
    this->ScaleThumbWheel[cc]->SetResolution(0.05);
    this->ScaleThumbWheel[cc]->Create(this->Application, 0);
    this->ScaleThumbWheel[cc]->DisplayEntryOn();
    this->ScaleThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->ScaleThumbWheel[cc]->ExpandEntryOn();
    this->ScaleThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->ScaleThumbWheel[cc]->SetCommand(this, "ActorScaleCallback");
    this->ScaleThumbWheel[cc]->SetEndCommand(this, "ActorScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetEntryCommand(this, "ActorScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetBalloonHelpString(
      "Scale the geometry relative to the size of the dataset.");

    this->OrientationScale[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->OrientationScale[cc]->PopupScaleOn();
    this->OrientationScale[cc]->Create(this->Application, 0);
    this->OrientationScale[cc]->SetRange(0, 360);
    this->OrientationScale[cc]->SetResolution(1);
    this->OrientationScale[cc]->SetValue(0);
    this->OrientationScale[cc]->DisplayEntry();
    this->OrientationScale[cc]->DisplayEntryAndLabelOnTopOff();
    this->OrientationScale[cc]->ExpandEntryOn();
    this->OrientationScale[cc]->GetEntry()->SetWidth(5);
    this->OrientationScale[cc]->SetCommand(this, "ActorOrientationCallback");
    this->OrientationScale[cc]->SetEndCommand(this, 
                                              "ActorOrientationEndCallback");
    this->OrientationScale[cc]->SetEntryCommand(this, 
                                                "ActorOrientationEndCallback");
    this->OrientationScale[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");

    this->OriginThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->OriginThumbWheel[cc]->PopupModeOn();
    this->OriginThumbWheel[cc]->SetValue(0.0);
    this->OriginThumbWheel[cc]->Create(this->Application, 0);
    this->OriginThumbWheel[cc]->DisplayEntryOn();
    this->OriginThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->OriginThumbWheel[cc]->ExpandEntryOn();
    this->OriginThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->OriginThumbWheel[cc]->SetCommand(this, "ActorOriginCallback");
    this->OriginThumbWheel[cc]->SetEndCommand(this, "ActorOriginEndCallback");
    this->OriginThumbWheel[cc]->SetEntryCommand(this,"ActorOriginEndCallback");
    this->OriginThumbWheel[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");
    }

  this->OpacityLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityLabel->Create(this->Application, 0);
  this->OpacityLabel->SetLabel("Opacity:");
  this->OpacityLabel->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geomtry "
    "because primatives are not sorted.");

  this->OpacityScale->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityScale->PopupScaleOn();
  this->OpacityScale->Create(this->Application, 0);
  this->OpacityScale->SetRange(0, 1);
  this->OpacityScale->SetResolution(0.1);
  this->OpacityScale->SetValue(1);
  this->OpacityScale->DisplayEntry();
  this->OpacityScale->DisplayEntryAndLabelOnTopOff();
  this->OpacityScale->ExpandEntryOn();
  this->OpacityScale->GetEntry()->SetWidth(5);
  this->OpacityScale->SetCommand(this, "OpacityChangedCallback");
  this->OpacityScale->SetEndCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetEntryCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geomtry "
    "because primatives are not sorted.");

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->TranslateLabel->GetWidgetName(),
               this->TranslateThumbWheel[0]->GetWidgetName(),
               this->TranslateThumbWheel[1]->GetWidgetName(),
               this->TranslateThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->TranslateLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->ScaleLabel->GetWidgetName(),
               this->ScaleThumbWheel[0]->GetWidgetName(),
               this->ScaleThumbWheel[1]->GetWidgetName(),
               this->ScaleThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->ScaleLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->OrientationLabel->GetWidgetName(),
               this->OrientationScale[0]->GetWidgetName(),
               this->OrientationScale[1]->GetWidgetName(),
               this->OrientationScale[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OrientationLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->OriginLabel->GetWidgetName(),
               this->OriginThumbWheel[0]->GetWidgetName(),
               this->OriginThumbWheel[1]->GetWidgetName(),
               this->OriginThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OriginLabel->GetWidgetName());

  this->Script("grid %s %s -sticky news -pady %d",
               this->OpacityLabel->GetWidgetName(),
               this->OpacityScale->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OpacityLabel->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  if (!this->GetPVSource()->GetHideDisplayPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
                 this->Properties->GetWidgetName());
    }

  // Pack

  this->Script("pack %s %s %s %s -fill x -expand t -pady 2", 
               this->ViewFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName(),
               this->DisplayStyleFrame->GetWidgetName(),
               this->ActorControlFrame->GetWidgetName());

  // Information page

  if (this->GetPVSource()->GetHideInformationPage())
    {
    // We use the parameters frame as a bogus parent.
    // This is not a problem since we never pack the
    // properties frame in this case.
    this->InformationFrame->SetParent(
      this->GetPVSource()->GetParametersParent());
    }
  else
    {
    this->InformationFrame->SetParent(
      this->GetPVSource()->GetNotebook()->GetFrame("Information"));
    }
  this->InformationFrame->ScrollableOn();
  this->InformationFrame->Create(this->Application,0);

  this->StatsFrame->SetParent(this->InformationFrame->GetFrame());
  this->StatsFrame->ShowHideFrameOn();
  this->StatsFrame->Create(this->Application, 0);
  this->StatsFrame->SetLabel("Statistics");

  this->TypeLabel->SetParent(this->StatsFrame->GetFrame());
  this->TypeLabel->Create(this->Application, "");

  this->NumCellsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumCellsLabel->Create(this->Application, "");

  this->NumPointsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumPointsLabel->Create(this->Application, "");
  
  this->BoundsDisplay->SetParent(this->InformationFrame->GetFrame());
  this->BoundsDisplay->Create(this->Application, "");
  
  this->ExtentDisplay->SetParent(this->InformationFrame->GetFrame());
  this->ExtentDisplay->Create(this->Application, "");
  this->ExtentDisplay->SetLabel("Extents");
  
  this->Script("pack %s %s %s -side top -anchor nw",
               this->TypeLabel->GetWidgetName(),
               this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName());

  this->Script("pack %s %s -fill x -expand t -pady 2", 
               this->StatsFrame->GetWidgetName(),
               this->BoundsDisplay->GetWidgetName());

  if (!this->GetPVSource()->GetHideInformationPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
                 this->InformationFrame->GetWidgetName());
    }

  // OK, all done

  this->PropertiesCreated = 1;
}

//----------------------------------------------------------------------------
void vtkPVData::EditColorMapCallback()
{
  if (this->PVColorMap == NULL)
    {
    // We could get the color map from the window,
    // but it must already be set for this button to be visible.
    vtkErrorMacro("Expecting a color map.");
    return;
    }
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
          this->PVColorMap->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVData::UpdateProperties()
{
  // This call makes sure the information is up to date.
  // If not, it gathers information and updates the properties (internal).
  this->GetPVSource()->GetDataInformation();
  this->UpdatePropertiesInternal();
}

//----------------------------------------------------------------------------
void vtkPVData::UpdatePropertiesInternal()
{
  vtkPVSource* source = this->GetPVSource();

  if (!source)
    {
    return;
    }

  vtkPVDataInformation* dataInfo = source->GetDataInformation();
  char tmp[350], cmd[1024], defCmd[350];
  float bounds[6];
  int i, numArrays, numComps;
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *arrayInfo;
  const char *currentColorBy;
  int currentColorByFound = 0;
  vtkPVWindow *window;
  int defPoint = 0;
  vtkPVArrayInformation *defArray;

  // Default is the scalars to use when current color is not found.
  // This is sort of a mess, and should be handled by a color selection widget.
  defCmd[0] = '\0'; 
  defArray = NULL;

  if (this->PVColorMap)
    {
    if (this->PVColorMap->GetScalarBarVisibility())
      {
      this->ScalarBarCheck->SetState(1);
      }
    else
      {
      this->ScalarBarCheck->SetState(0);
      }
    }

  if (source->GetHideDisplayPage())
    {
    return;
    }

  window = this->GetPVApplication()->GetMainWindow();

  // Update actor control resolutions

  this->UpdateActorControlResolutions();
  
  ostrstream type;
  type << "Type: ";

  // Put the data type as the label of the top frame.
  int dataType = dataInfo->GetDataSetType();
  if (dataType == VTK_POLY_DATA)
    {
    type << "Polygonal";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    type << "Unstructured Grid";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_STRUCTURED_GRID)
    {
    type << "Curvilinear";
    this->ExtentDisplay->SetExtent(dataInfo->GetExtent());
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_RECTILINEAR_GRID)
    {
    type << "Nonuniform Rectilinear";
    this->ExtentDisplay->SetExtent(dataInfo->GetExtent());
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_IMAGE_DATA)
    {
    int *ext = dataInfo->GetExtent();
    if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
      {
      type << "Image (Uniform Rectilinear)";
      }
    else
      {
      type << "Volume (Uniform Rectilinear)";
      }
    this->ExtentDisplay->SetExtent(ext);
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else
    {
    type << "Unknown";
    }
  type << ends;
  this->TypeLabel->SetLabel(type.str());
  type.rdbuf()->freeze(0);
  
  sprintf(tmp, "Number of cells: %d", 
          (int)(dataInfo->GetNumberOfCells()));
  this->NumCellsLabel->SetLabel(tmp);
  sprintf(tmp, "Number of points: %d", 
          (int)(dataInfo->GetNumberOfPoints()));
  this->NumPointsLabel->SetLabel(tmp);
  
  dataInfo->GetBounds(bounds);
  this->BoundsDisplay->SetBounds(bounds);
  if (this->CubeAxesTclName)
    {  
    this->Script("%s SetBounds %f %f %f %f %f %f",
                 this->CubeAxesTclName, bounds[0], bounds[1], bounds[2],
                 bounds[3], bounds[4], bounds[5]);
    }

  currentColorBy = this->ColorMenu->GetValue();
  this->ColorMenu->ClearEntries();
  this->ColorMenu->AddEntryWithCommand("Property",
                                       this, "ColorByProperty");

  attrInfo = dataInfo->GetPointDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    numComps = arrayInfo->GetNumberOfComponents();
    sprintf(cmd, "ColorByPointField {%s} %d", 
            arrayInfo->GetName(), arrayInfo->GetNumberOfComponents());
    if (arrayInfo->GetNumberOfComponents() > 1)
      {
      sprintf(tmp, "Point %s (%d)", arrayInfo->GetName(),
              arrayInfo->GetNumberOfComponents());
      }
    else
      {
      sprintf(tmp, "Point %s", arrayInfo->GetName());
      }
    this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
    if (strcmp(tmp, currentColorBy) == 0)
      {
      currentColorByFound = 1;
      }
    if (attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      strcpy(defCmd, tmp);
      defPoint = 1;
      defArray = arrayInfo;
      }
    }

  attrInfo = dataInfo->GetCellDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    sprintf(cmd, "ColorByCellField {%s} %d", 
            arrayInfo->GetName(), arrayInfo->GetNumberOfComponents());
    if (arrayInfo->GetNumberOfComponents() > 1)
      {
      sprintf(tmp, "Cell %s (%d)", arrayInfo->GetName(),
              arrayInfo->GetNumberOfComponents());
      }
    else
      {
      sprintf(tmp, "Cell %s", arrayInfo->GetName());
      }
    this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
    if (strcmp(tmp, currentColorBy) == 0)
      {
      currentColorByFound = 1;
      }
    if (defArray == NULL && attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      strcpy(defCmd, tmp);
      defPoint = 0;
      defArray = arrayInfo;
      }
    }

  // Current color by will never be NULL ....
  if (currentColorBy != NULL && strcmp(currentColorBy, "Property") == 0 && this->ColorSetByUser)
    {
    return;
    }

  // If the current array we are coloring by has disappeared,
  // then default back to the property.
  if ( ! currentColorByFound)
    {
    this->ColorSetByUser = 0;
    if (defArray != NULL)
      {
      this->ColorMenu->SetValue(defCmd);
      if (defPoint)
        {
        this->ColorByPointFieldInternal(defArray->GetName(), 
                                        defArray->GetNumberOfComponents());
        }
      else
        {
        this->ColorByCellFieldInternal(defArray->GetName(), 
                                       defArray->GetNumberOfComponents());
        }
      }
    else
      {
      this->ColorMenu->SetValue("Property");
      this->ColorByPropertyInternal();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorColor(float r, float g, float b)
{
  vtkPVPart *part;
  int idx, num;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->SetColor(r, g, b);
    }
}  

//----------------------------------------------------------------------------
void vtkPVData::ChangeActorColor(float r, float g, float b)
{
  vtkPVPart *part;
  part = this->GetPVSource()->GetPart();

  this->AddTraceEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  this->SetActorColor(r, g, b);
  this->ColorButton->SetColor(r, g, b);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}



//----------------------------------------------------------------------------
void vtkPVData::SetColorRange(float min, float max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    }

  this->PVColorMap->SetScalarRange(min, max);
}

//----------------------------------------------------------------------------
// Hack for now.
void vtkPVData::SetColorRangeInternal(float min, float max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    }

  this->PVColorMap->SetScalarRangeInternal(min, max);
}

//----------------------------------------------------------------------------
void vtkPVData::ResetColorRange()
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    return;
    }

  this->PVColorMap->ResetScalarRange();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByProperty()
{
  this->ColorSetByUser = 1;
  this->AddTraceEntry("$kw(%s) ColorByProperty", this->GetTclName());
  this->ColorMenu->SetValue("Property");
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPropertyInternal()
{
  vtkPVPart *part;
  int idx, num;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->SetScalarVisibility(0);
    }

  float *color = this->ColorButton->GetColor();
  this->SetActorColor(color[0], color[1], color[2]);

  this->SetPVColorMap(NULL);

  this->MapScalarsCheck->EnabledOff();
  this->EditColorMapButton->EnabledOff();
  this->ScalarBarCheck->EnabledOff();

  this->ColorButton->EnabledOn();


  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::ColorByPointField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ColorByPointField {%s} %d", 
                      this->GetTclName(), name, numComps);

  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Point %s", name);
    }
  else
    {
    sprintf(str, "Point %s (%d)", name, numComps);
    }
  this->ColorMenu->SetValue(str);
  delete [] str;

  this->ColorByPointFieldInternal(name, numComps);
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPointFieldInternal(const char *name, int numComps)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int num, idx;
  vtkPVPart *part;

  this->SetPVColorMap(pvApp->GetMainWindow()->GetPVColorMap(name, numComps));
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->ColorByArray(this->PVColorMap, VTK_POINT_DATA_FIELD);
    } 
  this->ColorButton->EnabledOff();
  this->UpdateMapScalarsCheck(
           this->PVSource->GetDataInformation()->GetPointDataInformation(),
           name);


  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByCellField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ColorByCellField {%s} %d", 
                      this->GetTclName(), name, numComps);
  // Set the menu value.
  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Cell %s", name);
    }
  else
    {
    sprintf(str, "Cell %s (%d)", name, numComps);
    }
  this->ColorMenu->SetValue(str);
  delete [] str;

  this->ColorByCellFieldInternal(name, numComps);
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByCellFieldInternal(const char *name, int numComps)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;

  this->SetPVColorMap(pvApp->GetMainWindow()->GetPVColorMap(name, numComps));
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->ColorByArray(this->PVColorMap, VTK_CELL_DATA_FIELD);
    }

  this->ColorButton->EnabledOff();
  this->UpdateMapScalarsCheck(
           this->PVSource->GetDataInformation()->GetCellDataInformation(),
           name);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::UpdateMapScalarsCheck(vtkPVDataSetAttributesInformation* info,
                                       const char* name)
{
  vtkPVArrayInformation* arrayInfo;
  
  arrayInfo = info->GetArrayInformation(name);
  
  // First set of conditions.
  if (arrayInfo == NULL || 
      arrayInfo->GetDataType() != VTK_UNSIGNED_CHAR)
    { // Direct mapping not an option.
    this->MapScalarsCheck->SetState(1);
    this->MapScalarsCheck->EnabledOff();
    this->ScalarBarCheck->EnabledOn();
    this->EditColorMapButton->EnabledOn();
    return;
    }

  // Number of component restriction.
  if (arrayInfo->GetNumberOfComponents() != 1 &&
      arrayInfo->GetNumberOfComponents() != 3)
    { // I would like to have two as an option also ...
    this->MapScalarsCheck->SetState(1);
    this->MapScalarsCheck->EnabledOff();
    this->ScalarBarCheck->EnabledOn();
    this->EditColorMapButton->EnabledOn();
    return;
    }

  // Direct color map is an option.
  this->MapScalarsCheck->EnabledOn();

  // I might like to store the default in another variable.
  if (this->MapScalarsCheck->GetState())
    {
    this->ScalarBarCheck->EnabledOn();
    this->EditColorMapButton->EnabledOn();
    }
  else
    {
    this->ScalarBarCheck->EnabledOff();
    this->EditColorMapButton->EnabledOff();
    }
    
}

//----------------------------------------------------------------------------
void vtkPVData::SetRepresentation(const char* repr)
{
  if ( vtkString::Equals(repr, "Wireframe") )
    {
    this->DrawWireframe();
    }
  else if ( vtkString::Equals(repr, "Surface") )
    {
    this->DrawSurface();
    }
  else if ( vtkString::Equals(repr, "Points") )
    {
    this->DrawPoints();
    }
  else
    {
    vtkErrorMacro("Don't know the representation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawWireframe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;
  
  this->AddTraceEntry("$kw(%s) DrawWireframe", this->GetTclName());

  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // We could move the property into vtkPVData so parts would share one object.
  // Well, this would complicate initialization.  We would have to pass
  // the property into the part for the mappers.
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      if (this->PreviousWasSolid)
        {
        this->PreviousAmbient = part->GetPartDisplay()->GetProperty()->GetAmbient();
        this->PreviousDiffuse = part->GetPartDisplay()->GetProperty()->GetDiffuse();
        this->PreviousSpecular = part->GetPartDisplay()->GetProperty()->GetSpecular();
        }
      this->PreviousWasSolid = 0;
      pvApp->BroadcastScript("%s SetAmbient 1", part->GetPartDisplay()->GetPropertyTclName());
      pvApp->BroadcastScript("%s SetDiffuse 0", part->GetPartDisplay()->GetPropertyTclName());
      pvApp->BroadcastScript("%s SetSpecular 0", part->GetPartDisplay()->GetPropertyTclName());
      pvApp->BroadcastScript("%s SetRepresentationToWireframe",
                             part->GetPartDisplay()->GetPropertyTclName());
      }
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;

  this->AddTraceEntry("$kw(%s) DrawPoints", this->GetTclName());
  this->RepresentationMenu->SetValue("Points");

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      if (this->PreviousWasSolid)
        {
        this->PreviousAmbient = part->GetPartDisplay()->GetProperty()->GetAmbient();
        this->PreviousDiffuse = part->GetPartDisplay()->GetProperty()->GetDiffuse();
        this->PreviousSpecular = part->GetPartDisplay()->GetProperty()->GetSpecular();
        }
      this->PreviousWasSolid = 0;
      pvApp->BroadcastScript("%s SetAmbient 1", part->GetPartDisplay()->GetPropertyTclName());
      pvApp->BroadcastScript("%s SetDiffuse 0", part->GetPartDisplay()->GetPropertyTclName());
      pvApp->BroadcastScript("%s SetSpecular 0", part->GetPartDisplay()->GetPropertyTclName());
      pvApp->BroadcastScript("%s SetRepresentationToPoints",
                             part->GetPartDisplay()->GetPropertyTclName());
      }
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawSurface()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int num, idx;
  
  this->AddTraceEntry("$kw(%s) DrawSurface", this->GetTclName());
  this->RepresentationMenu->SetValue("Surface");


  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      if (!this->PreviousWasSolid)
        {
        pvApp->BroadcastScript("%s SetAmbient %f",
                               part->GetPartDisplay()->GetPropertyTclName(), this->PreviousAmbient);
        pvApp->BroadcastScript("%s SetDiffuse %f",
                               part->GetPartDisplay()->GetPropertyTclName(), this->PreviousDiffuse);
        pvApp->BroadcastScript("%s SetSpecular %f",
                               part->GetPartDisplay()->GetPropertyTclName(), this->PreviousSpecular);
        pvApp->BroadcastScript("%s SetRepresentationToSurface",
                               part->GetPartDisplay()->GetPropertyTclName());
        }
      }
    }
  this->PreviousWasSolid = 1;

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetInterpolation(const char* repr)
{
  if ( vtkString::Equals(repr, "Flat") )
    {
    this->SetInterpolationToFlat();
    }
  else if ( vtkString::Equals(repr, "Gouraud") )
    {
    this->SetInterpolationToGouraud();
    }
  else
    {
    vtkErrorMacro("Don't know the interpolation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToFlat()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToFlat", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Flat");

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      pvApp->BroadcastScript("%s SetInterpolationToFlat",
                             part->GetPartDisplay()->GetPropertyTclName());
      }
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToGouraud()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int num, idx;
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToGouraud", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Gouraud");

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      pvApp->BroadcastScript("%s SetInterpolationToGouraud",
                             part->GetPartDisplay()->GetPropertyTclName());
      }
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}



//----------------------------------------------------------------------------
void vtkPVData::AmbientChanged()
{
  // This doesn't currently need to do anything since we aren't actually
  //packing the ambient scale.
/*  vtkPVApplication *pvApp = this->GetPVApplication();
  float ambient = this->AmbientScale->GetValue();
  
  //pvApp->BroadcastScript("%s SetAmbient %f", this->GetTclName(), ambient);

  
  this->SetAmbient(ambient);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
*/
}

//----------------------------------------------------------------------------
void vtkPVData::SetAmbient(float ambient)
{
  vtkPVPart *part;
  int num, idx;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    part->GetPartDisplay()->GetProperty()->SetAmbient(ambient);
    }
}


//----------------------------------------------------------------------------
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// I would like to get rid of this method.  
// It is no longer needed to set scalar range.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void vtkPVData::Initialize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  float bounds[6];
  char *tclName;
  char newTclName[100];

  vtkDebugMacro( << "Initialize --------")
  
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  tclName = pvApp->GetRenderModule()->GetRendererTclName();
  
  sprintf(newTclName, "CubeAxes%d", this->InstanceCount);
  this->SetCubeAxesTclName(newTclName);
  this->Script("vtkCubeAxesActor2D %s", this->GetCubeAxesTclName());
  this->Script("%s SetFlyModeToOuterEdges", this->GetCubeAxesTclName());
  this->Script("[%s GetProperty] SetColor 1 1 1",
               this->GetCubeAxesTclName());
  
  this->Script("%s SetBounds %f %f %f %f %f %f",
               this->GetCubeAxesTclName(), bounds[0], bounds[1], bounds[2],
               bounds[3], bounds[4], bounds[5]);
  this->Script("%s SetCamera [%s GetActiveCamera]",
               this->GetCubeAxesTclName(), tclName);
  this->Script("%s SetInertia 20", this->GetCubeAxesTclName());
  
}


//----------------------------------------------------------------------------
void vtkPVData::CenterCamera()
{
  float bounds[6];
  vtkPVApplication *pvApp = this->GetPVApplication();
  char* tclName;
  
  tclName = pvApp->GetRenderModule()->GetRendererTclName();
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    pvApp->Script("%s ResetCamera %f %f %f %f %f %f",
                  tclName, bounds[0], bounds[1], bounds[2],
                  bounds[3], bounds[4], bounds[5]);
    pvApp->Script("%s ResetCameraClippingRange", tclName);
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::VisibilityCheckCallback()
{
  this->GetPVSource()->SetVisibility(this->VisibilityCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetVisibility(int v)
{
  this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), v);
  this->GetPVSource()->SetVisibilityInternal(v);
  this->Script("%s SetVisibility %d", this->GetCubeAxesTclName(), v);
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVData::GetPVRenderView()
{
  return this->GetPVSource()->GetPVRenderView();
}



//----------------------------------------------------------------------------
vtkPVApplication* vtkPVData::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVData::SetScalarBarVisibility(int val)
{
  if (this->PVColorMap)
    {
    this->PVColorMap->SetScalarBarVisibility(val);
    }
  
  if (this->ScalarBarCheck->GetState() != val)
    {
    this->ScalarBarCheck->SetState(val);
    }
}


// This should really be a part of vtkPVPartDisplay object.
//----------------------------------------------------------------------------
void vtkPVData::SetCubeAxesVisibility(int val)
{
  vtkRenderer *ren;
  char *tclName;
  
  if (!this->GetPVRenderView())
    {
    return;
    }
  
  ren = this->GetPVRenderView()->GetRenderer();
  tclName = this->GetPVApplication()->GetRenderModule()->GetRendererTclName();
  
  if (ren == NULL)
    {
    return;
    }
  
  if (this->CubeAxesCheck->GetState() != val)
    {
    this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(), val);
    this->CubeAxesCheck->SetState(val);
    }

  // I am going to add and remove it from the renderer instead of using visibility.
  // Composites should really have multiple props.
  
  if (ren)
    {
    if (val)
      {
      this->Script("%s AddProp %s", tclName, this->GetCubeAxesTclName());
      }
    else
      {
      this->Script("%s RemoveProp %s", tclName, this->GetCubeAxesTclName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ScalarBarCheckCallback()
{
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::CubeAxesCheckCallback()
{
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(),
                      this->CubeAxesCheck->GetState());
  this->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::MapScalarsCheckCallback()
{
  this->SetMapScalarsFlag(this->MapScalarsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetMapScalarsFlag(int val)
{
  if (this->MapScalarsCheck->GetState() != val)
    {
    this->AddTraceEntry("$kw(%s) SetMapScalarsFlag %d", this->GetTclName(), val);
    this->MapScalarsCheck->SetState(val);
    }

  if (val)
    {
    this->EditColorMapButton->EnabledOn();
    this->ScalarBarCheck->EnabledOn();
    }
  else
    {
    this->EditColorMapButton->EnabledOff();
    this->ScalarBarCheck->EnabledOff();
    }

  int num, idx;
  num = this->PVSource->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    this->PVSource->GetPart(idx)->GetPartDisplay()->SetDirectColorFlag(!val);
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetPointSize(int size)
{
  if ( this->PointSizeThumbWheel->GetValue() == size )
    {
    return;
    }
  // The following call with trigger the ChangePointSize callback (below)
  // but won't add a trace entry. Let's do it. A trace entry is also
  // added by the ChangePointSizeEndCallback but this callback is only
  // called when the interaction on the scale is stopped.
  this->PointSizeThumbWheel->SetValue(size);
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVData::ChangePointSize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      pvApp->BroadcastScript("%s SetPointSize %f",
                             part->GetPartDisplay()->GetPropertyTclName(),
                             this->PointSizeThumbWheel->GetValue());
      }
    }
 
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
} 

//----------------------------------------------------------------------------
void vtkPVData::ChangePointSizeEndCallback()
{
  this->ChangePointSize();
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
} 

//----------------------------------------------------------------------------
void vtkPVData::SetLineWidth(int width)
{
  if ( this->LineWidthThumbWheel->GetValue() == width )
    {
    return;
    }
  // The following call with trigger the ChangeLineWidth callback (below)
  // but won't add a trace entry. Let's do it. A trace entry is also
  // added by the ChangeLineWidthEndCallback but this callback is only
  // called when the interaction on the scale is stopped.
  this->LineWidthThumbWheel->SetValue(width);
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVData::ChangeLineWidth()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  int idx, num;
  
  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    if (part->GetPartDisplay()->GetPropertyTclName())
      {
      pvApp->BroadcastScript("%s SetLineWidth %f",
                             part->GetPartDisplay()->GetPropertyTclName(),
                             this->LineWidthThumbWheel->GetValue());
      }
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ChangeLineWidthEndCallback()
{
  this->ChangeLineWidth();
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVData::SetPropertiesParent(vtkKWWidget *parent)
{
  if (this->PropertiesParent == parent)
    {
    return;
    }
  if (this->PropertiesParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->PropertiesParent = parent;
  parent->Register(this);
}





//----------------------------------------------------------------------------
void vtkPVData::SaveInBatchScript(ofstream *file)
{
  float range[2];
  const char* scalarMode;
  char* result;
  char* renTclName;
  vtkPVPart *part;
  int partIdx, numParts;
  int sourceCount;
  int numSources;
  int outputCount;
  int numOutputs;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();

  renTclName = this->GetPVApplication()->GetRenderModule()->GetRendererTclName();
  if (this->GetVisibility())
    {
    if (this->PVColorMap)
      {
      this->PVColorMap->SaveInBatchScript(file);
      }
    numSources = this->GetPVSource()->GetNumberOfVTKSources();
    // Easier to initialize the output traversal this way.
    // We want to exaust each sources output, then move to the next source.
    sourceCount = -1;
    numOutputs = 0;
    outputCount = 0;
    numParts = this->GetPVSource()->GetNumberOfParts();
    for (partIdx = 0; partIdx < numParts; ++partIdx)
      {
      part = this->GetPVSource()->GetPart(partIdx);
      // Get the next output from the sequence of sources/outputs.
      if (outputCount >= numOutputs)
        { // Move to next source
        ++sourceCount;
        if (sourceCount >= numSources)
          { // sanity check
          vtkErrorMacro("We ran out of sources.");
          return;
          }
        pm->RootScript("%s GetNumberOfOutputs",
                       this->GetPVSource()->GetVTKSourceTclName(sourceCount));
        numOutputs = atoi(pm->GetRootResult());
        outputCount = 0;
        }

      *file << "vtkPVGeometryFilter " << part->GetGeometryTclName() << "\n\t"
            << part->GetGeometryTclName() << " SetInput [" 
            << this->GetPVSource()->GetVTKSourceTclName(sourceCount) 
            << " GetOutput " << outputCount << "]\n";
      // Move to next output
      ++outputCount;


      *file << "vtkPolyDataMapper " << part->GetPartDisplay()->GetMapperTclName() << "\n\t"
            << part->GetPartDisplay()->GetMapperTclName() << " SetInput ["
            << part->GetGeometryTclName() << " GetOutput]\n\t";  
      *file << part->GetPartDisplay()->GetMapperTclName() << " SetImmediateModeRendering "
            << part->GetPartDisplay()->GetMapper()->GetImmediateModeRendering() << "\n\t";
      part->GetPartDisplay()->GetMapper()->GetScalarRange(range);
      *file << part->GetPartDisplay()->GetMapperTclName() << " UseLookupTableScalarRangeOn\n\t";
      *file << part->GetPartDisplay()->GetMapperTclName() << " SetScalarVisibility "
            << part->GetPartDisplay()->GetMapper()->GetScalarVisibility() << "\n\t"
            << part->GetPartDisplay()->GetMapperTclName() << " SetScalarModeTo";
      scalarMode = part->GetPartDisplay()->GetMapper()->GetScalarModeAsString();
      *file << scalarMode << "\n";
      if (strcmp(scalarMode, "UsePointFieldData") == 0 ||
          strcmp(scalarMode, "UseCellFieldData") == 0)
        {
        *file << "\t" << part->GetPartDisplay()->GetMapperTclName() << " SelectColorArray {"
              << part->GetPartDisplay()->GetMapper()->GetArrayName() << "}\n";
        }
      if (this->PVColorMap)
        {
        *file << part->GetPartDisplay()->GetMapperTclName() << " SetLookupTable " 
              << this->PVColorMap->GetLookupTableTclName() << endl;
        }
  
      *file << "vtkActor " << part->GetPartDisplay()->GetPropTclName() << "\n\t"
            << part->GetPartDisplay()->GetPropTclName() << " SetMapper " 
            << part->GetPartDisplay()->GetMapperTclName() << "\n\t"
            << "[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetRepresentationTo"
            << part->GetPartDisplay()->GetProperty()->GetRepresentationAsString() << "\n\t"
            << "[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetInterpolationTo"
            << part->GetPartDisplay()->GetProperty()->GetInterpolationAsString() << "\n";

      *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetAmbient "
            << part->GetPartDisplay()->GetProperty()->GetAmbient() << "\n";
      *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetDiffuse "
            << part->GetPartDisplay()->GetProperty()->GetDiffuse() << "\n";
      *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetSpecular "
            << part->GetPartDisplay()->GetProperty()->GetSpecular() << "\n";
      *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetSpecularPower "
            << part->GetPartDisplay()->GetProperty()->GetSpecularPower() << "\n";
      float *color = part->GetPartDisplay()->GetProperty()->GetSpecularColor();
      *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetSpecularColor "
            << color[0] << " " << color[1] << " " << color[2] << "\n";
      if (part->GetPartDisplay()->GetProperty()->GetLineWidth() > 1)
        {
        *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetLineWidth "
            << part->GetPartDisplay()->GetProperty()->GetLineWidth() << "\n";
        }
      if (part->GetPartDisplay()->GetProperty()->GetPointSize() > 1)
        {
        *file << "\t[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetPointSize "
            << part->GetPartDisplay()->GetProperty()->GetPointSize() << "\n";
        }

      if (!part->GetPartDisplay()->GetMapper()->GetScalarVisibility())
        {
        float propColor[3];
        part->GetPartDisplay()->GetProperty()->GetColor(propColor);
        *file << "[" << part->GetPartDisplay()->GetPropTclName() << " GetProperty] SetColor "
              << propColor[0] << " " << propColor[1] << " " << propColor[2]
              << "\n";
        }

      *file << renTclName << " AddActor " << part->GetPartDisplay()->GetPropTclName() << "\n";
      }  
    }

  if (this->CubeAxesCheck->GetState())
    {
    *file << "vtkCubeAxesActor2D " << this->CubeAxesTclName << "\n\t"
          << this->CubeAxesTclName << " SetFlyModeToOuterEdges\n\t"
          << "[" << this->CubeAxesTclName << " GetProperty] SetColor 1 1 1\n\t"
          << this->CubeAxesTclName << " SetBounds ";
    this->Script("set tempResult [%s GetBounds]", this->CubeAxesTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\t"
          << this->CubeAxesTclName << " SetCamera [";
    *file << renTclName << " GetActiveCamera]\n\t"
          << this->CubeAxesTclName << " SetInertia 20\n";
    *file << renTclName << " AddProp " << this->CubeAxesTclName << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SaveState(ofstream *file)
{
  float *pf;
  float f1, f2, f3;
  int   i1;

  *file << "set kw(" << this->GetTclName() << ") [$kw(" 
        << this->PVSource->GetTclName() << ") GetPVOutput]\n";

  pf = this->ColorButton->GetColor();
  if (pf[0] != 1.0 || pf[1] != 1.0 || pf[2] != 1.0)
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeActorColor " << pf[0]
          << " " << pf[1] << " " << pf[2] << endl;
    }

  if (strcmp (this->ColorMenu->GetValue(),"Property") != 0)
    {
    const char* name = this->PVColorMap->GetArrayName();
    int comps = this->PVColorMap->GetNumberOfVectorComponents();
    if (strncmp (this->ColorMenu->GetValue(),"Point",5) == 0)
      {
      *file << "$kw(" << this->GetTclName() << ") ColorByPointField {" 
            << name << "} " << comps << endl;
      }
    if (strncmp (this->ColorMenu->GetValue(),"Cell",4) == 0)
      {
      *file << "$kw(" << this->GetTclName() << ") ColorByCellField {" 
            << name << "} " << comps << endl;
      }
    }


  if (strcmp(this->RepresentationMenu->GetValue(),"Wireframe") == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawWireframe\n";
    }
  if (strcmp(this->RepresentationMenu->GetValue(),"Points") == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") DrawPoints\n";
    }

  if (strcmp(this->InterpolationMenu->GetValue(),"Flat") == 0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetInterpolationToFlat\n";
    }


  if (this->CubeAxesCheck->GetState())
    {
    *file << "$kw(" << this->GetTclName() << ") SetCubeAxesVisibility 1\n";
    }

  i1 = (int)(this->PointSizeThumbWheel->GetValue());
  if (i1 != 1)
    {
    *file << "$kw(" << this->GetTclName() << ") SetPointSize " << i1 << endl;
    }
    
  i1 = (int)(this->LineWidthThumbWheel->GetValue());
  if (i1 != 1)
    {
    *file << "$kw(" << this->GetTclName() << ") SetLineWidth " << i1 << endl;
    }
 

  f1 = this->OpacityScale->GetValue();
  if (f1 != 1.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetOpacity " << f1 << endl;
    }

  f1 = this->TranslateThumbWheel[0]->GetValue();
  f2 = this->TranslateThumbWheel[1]->GetValue();
  f3 = this->TranslateThumbWheel[2]->GetValue();
  if (f1 != 0.0 || f2 != 0.0 || f3 != 0.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorTranslate "
          << f1 << " " << f2 << " " << f3 << endl;
    }

  f1 = this->ScaleThumbWheel[0]->GetValue();
  f2 = this->ScaleThumbWheel[1]->GetValue();
  f3 = this->ScaleThumbWheel[2]->GetValue();
  if (f1 != 1.0 || f2 != 1.0 || f3 != 1.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorScale " 
          << f1 << " " << f2 << " " << f3 << endl;
    }

  f1 = this->OrientationScale[0]->GetValue();
  f2 = this->OrientationScale[1]->GetValue();
  f3 = this->OrientationScale[2]->GetValue();
  if (f1 != 0.0 || f2 != 0.0 || f3 != 0.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorOrientation "
          << f1 << " " << f2 << " " << f3 << endl;
    }


  f1 = this->OriginThumbWheel[0]->GetValue();
  f2 = this->OriginThumbWheel[1]->GetValue();
  f3 = this->OriginThumbWheel[2]->GetValue();
  if (f1 != 0.0 || f2 != 0.0 || f3 != 0.0)
    {
    *file << "$kw(" << this->GetTclName() << ") SetActorOrigin "
          << f1 << " " << f2 << " " << f3 << endl;
    }  
}

//----------------------------------------------------------------------------
void vtkPVData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ColorMap: " << this->PVColorMap << endl;
  os << indent << "ColorMenu: " << this->ColorMenu << endl;
  os << indent << "EditColorMapButton: " << this->EditColorMapButton << endl;
  os << indent << "CubeAxesTclName: " << (this->CubeAxesTclName?this->CubeAxesTclName:"none") << endl;
  os << indent << "PVSource: " << this->GetPVSource() << endl;
  os << indent << "PropertiesParent: " << this->GetPropertiesParent() << endl;
  if (this->PVColorMap)
    {
    os << indent << "PVColorMap: " << this->PVColorMap->GetScalarBarTitle() << endl;
    }
  else
    {
    os << indent << "PVColorMap: NULL\n";
    }
  os << indent << "PropertiesCreated: " << this->PropertiesCreated << endl;
  os << indent << "CubeAxesCheck: " << this->CubeAxesCheck << endl;
  os << indent << "ScalarBarCheck: " << this->ScalarBarCheck << endl;
  os << indent << "RepresentationMenu: " << this->RepresentationMenu << endl;
  os << indent << "InterpolationMenu: " << this->InterpolationMenu << endl;
  os << indent << "Visibility: " << this->Visibility << endl;

}

//----------------------------------------------------------------------------
void vtkPVData::GetColorRange(float *range)
{
  float *tmp;
  range[0] = 0.0;
  range[1] = 1.0;
  
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map missing.");
    return;
    }
  tmp = this->PVColorMap->GetScalarRange();
  range[0] = tmp[0];
  range[1] = tmp[1];
}

//----------------------------------------------------------------------------
void vtkPVData::SetOpacity(float val)
{ 
  this->OpacityScale->SetValue(val);
}

//----------------------------------------------------------------------------
void vtkPVData::OpacityChangedCallback()
{
  vtkPVPart *part;
  int idx, num;

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    this->GetPVApplication()->BroadcastScript("[ %s GetProperty ] SetOpacity %f",
                                              part->GetPartDisplay()->GetPropTclName(), 
                                              this->OpacityScale->GetValue());
    }

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::OpacityChangedEndCallback()
{
  this->OpacityChangedCallback();
  this->AddTraceEntry("$kw(%s) SetOpacity %f", 
                      this->GetTclName(), this->OpacityScale->GetValue());
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorTranslate(float* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);  
  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetPosition(point);
    }
  else
    {
    point[0] = this->TranslateThumbWheel[0]->GetValue();
    point[1] = this->TranslateThumbWheel[1]->GetValue();
    point[2] = this->TranslateThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslateNoTrace(float x, float y, float z)
{
  vtkPVPart *part;
  int idx, num;

  this->TranslateThumbWheel[0]->SetValue(x);
  this->TranslateThumbWheel[1]->SetValue(y);
  this->TranslateThumbWheel[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    this->GetPVApplication()->BroadcastScript("%s SetPosition %f %f %f",
                                              part->GetPartDisplay()->GetPropTclName(), x, y, z);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslate(float x, float y, float z)
{
  this->SetActorTranslateNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorTranslate %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslate(float* point)
{
  this->SetActorTranslate(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorTranslateCallback()
{
  float point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslateNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ActorTranslateEndCallback()
{
  float point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslate(point);
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorScale(float* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);
  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetScale(point);
    }
  else
    {
    point[0] = this->ScaleThumbWheel[0]->GetValue();
    point[1] = this->ScaleThumbWheel[1]->GetValue();
    point[2] = this->ScaleThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScaleNoTrace(float x, float y, float z)
{
  vtkPVPart *part;
  int idx, num;

  this->ScaleThumbWheel[0]->SetValue(x);
  this->ScaleThumbWheel[1]->SetValue(y);
  this->ScaleThumbWheel[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    this->GetPVApplication()->BroadcastScript("%s SetScale %f %f %f",
                                              part->GetPartDisplay()->GetPropTclName(), x, y, z);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScale(float x, float y, float z)
{
  this->SetActorScaleNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorScale %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScale(float* point)
{
  this->SetActorScale(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorScaleCallback()
{
  float point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScaleNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ActorScaleEndCallback()
{
  float point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScale(point);
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorOrientation(float* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);

  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetOrientation(point);
    }
  else
    {
    point[0] = this->OrientationScale[0]->GetValue();
    point[1] = this->OrientationScale[1]->GetValue();
    point[2] = this->OrientationScale[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrientationNoTrace(float x, float y, float z)
{
  vtkPVPart *part;
  int idx, num;

  this->OrientationScale[0]->SetValue(x);
  this->OrientationScale[1]->SetValue(y);
  this->OrientationScale[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    this->GetPVApplication()->BroadcastScript("%s SetOrientation %f %f %f",
                                              part->GetPartDisplay()->GetPropTclName(), x, y, z);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrientation(float x, float y, float z)
{
  this->SetActorOrientationNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorOrientation %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrientation(float* point)
{
  this->SetActorOrientation(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorOrientationCallback()
{
  float point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientationNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ActorOrientationEndCallback()
{
  float point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientation(point);
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorOrigin(float* point)
{
  vtkPVPart *part;

  part = this->GetPVSource()->GetPart(0);
  vtkProp3D *prop = vtkProp3D::SafeDownCast(part->GetPartDisplay()->GetProp());
  if (prop)
    {
    prop->GetOrigin(point);
    }
  else
    {
    point[0] = this->OriginThumbWheel[0]->GetValue();
    point[1] = this->OriginThumbWheel[1]->GetValue();
    point[2] = this->OriginThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOriginNoTrace(float x, float y, float z)
{
  vtkPVPart *part;
  int idx, num;

  this->OriginThumbWheel[0]->SetValue(x);
  this->OriginThumbWheel[1]->SetValue(y);
  this->OriginThumbWheel[2]->SetValue(z);

  num = this->GetPVSource()->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVSource()->GetPart(idx);
    this->GetPVApplication()->BroadcastScript("%s SetOrigin %f %f %f",
                                              part->GetPartDisplay()->GetPropTclName(), x, y, z);
    }

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrigin(float x, float y, float z)
{
  this->SetActorOriginNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorOrigin %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorOrigin(float* point)
{
  this->SetActorOrigin(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::ActorOriginCallback()
{
  float point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOriginNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ActorOriginEndCallback()
{
  float point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOrigin(point);
}

//----------------------------------------------------------------------------
void vtkPVData::UpdateActorControlResolutions()
{
  float bounds[6];
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  float res, oneh, half;

  // Update the resolution according to the bounds
  // Set res to 1/20 of the range, rounding to nearest .1 or .5 form.

  int i;
  for (i = 0; i < 3; i++)
    {
    float delta = bounds[i * 2 + 1] - bounds[i * 2];
    if (delta <= 0)
      {
      res = 0.1;
      }
    else
      {
      oneh = log10(delta * 0.051234);
      half = 0.5 * pow(10, ceil(oneh));
      res = (oneh > log10(half) ? half : pow(10, floor(oneh)));
      // cout << "up i: " << i << ", delta: " << delta << ", oneh: " << oneh << ", half: " << half << ", res: " << res << endl;
      }
    this->TranslateThumbWheel[i]->SetResolution(res);
    this->OriginThumbWheel[i]->SetResolution(res);
    }
}

  




