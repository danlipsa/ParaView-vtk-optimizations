vtk_module(vtkStreaming
  DEPENDS
   vtkRenderingOpenGL
   vtkRenderingFreeTypeOpenGL
   vtkInteractionStyle
   vtkFiltersCore
   vtkIOCore
   vtkImagingSources
   vtkIONetCDF
   vtknetcdf
  TEST_DEPENDS
    vtkTestingRendering
  EXCLUDE_FROM_WRAP_HIERARCHY
)
