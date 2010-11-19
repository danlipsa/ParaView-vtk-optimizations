/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraManipulator.h"

#include "vtkCamera.h"
#include "vtkCameraInterpolator.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVCameraKeyFrame.h"

vtkStandardNewMacro(vtkPVCameraManipulator);
//------------------------------------------------------------------------------
vtkPVCameraManipulator::vtkPVCameraManipulator()
{
  this->Mode = PATH;
  this->CameraInterpolator = vtkCameraInterpolator::New();
}

//------------------------------------------------------------------------------
vtkPVCameraManipulator::~vtkPVCameraManipulator()
{
  this->CameraInterpolator->Delete();
}

//------------------------------------------------------------------------------
void vtkPVCameraManipulator::Initialize(vtkPVAnimationCue* cue)
{
  this->Superclass::Initialize(cue);
  int nos = this->GetNumberOfKeyFrames();
  this->CameraInterpolator->Initialize();
  this->CameraInterpolator->SetInterpolationTypeToSpline();
  if (nos < 2)
    {
    vtkErrorMacro("Too few keyframes to animate.");
    return;
    }

  if (this->Mode == PATH)
    {
    // No need to initialize this->CameraInterpolator in PATH mode.
    return;
    }

  // Set up this->CameraInterpolator.
  for(int i=0; i < nos; i++)
    {
    vtkPVCameraKeyFrame* kf;
    kf = vtkPVCameraKeyFrame::SafeDownCast(this->GetKeyFrameAtIndex(i));

    if (!kf)
      {
      vtkErrorMacro("All keyframes in a vtkPVCameraKeyFrame must be "
                    "vtkPVCameraKeyFrame");
      continue;
      }
    this->CameraInterpolator->AddCamera(kf->GetKeyTime(), kf->GetCamera());
    }
}

//------------------------------------------------------------------------------
void vtkPVCameraManipulator::Finalize(vtkPVAnimationCue* cue)
{
  this->Superclass::Finalize(cue);
}

//------------------------------------------------------------------------------
void vtkPVCameraManipulator::UpdateValue(double currenttime,
                                         vtkPVAnimationCue* cue)
{
  // FIXME +++++++++++++++++++++++++++++++++++++++++++++
//  if (this->Mode == CAMERA)
//    {
//    vtkSMProxy* renderViewProxy = cue->GetAnimatedProxy();
//    vtkCamera* camera = vtkCamera::New();
//    this->CameraInterpolator->InterpolateCamera(currenttime, camera);
//    vtkSMPropertyHelper(renderViewProxy, "CameraPosition").Set(camera->GetPosition(), 3);
//    vtkSMPropertyHelper(renderViewProxy, "CameraFocalPoint").Set(camera->GetFocalPoint(), 3);
//    vtkSMPropertyHelper(renderViewProxy, "CameraViewUp").Set(camera->GetViewUp(), 3);
//    vtkSMPropertyHelper(renderViewProxy, "CameraViewAngle").Set(0, camera->GetViewAngle());
//    vtkSMPropertyHelper(renderViewProxy,
//      "CameraClippingRange").Set(camera->GetClippingRange(), 2);
//    vtkSMPropertyHelper(renderViewProxy, "CameraParallelScale").Set(0,
//      camera->GetParallelScale());
//    camera->Delete();
//    renderViewProxy->UpdateVTKObjects();

//#ifdef FIXME
//    if (vtkSMRenderViewProxy::SafeDownCast(renderViewProxy))
//      {
//      vtkSMRenderViewProxy::SafeDownCast(renderViewProxy)->ResetCameraClippingRange();
//      }
//#endif
//    }
//  else
//    {
//    this->Superclass::UpdateValue(currenttime, cue);
//    }
}

//------------------------------------------------------------------------------
void vtkPVCameraManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode:" << this->Mode << endl;
}
