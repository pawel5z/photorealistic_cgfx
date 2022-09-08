# photorealistic_cgfx
Path tracer developed during the Photorealistic Computer Graphics course @ Computer Science studies @ University of Wrocław.

## Features
- Physically correct path tracing.
- Fast ray-triangle intersection computation using k-d tree.
- Output in EXR format.
- Adjustable number of threads used during the rendering.
- Adjustable render resolution and camera parameters using RTC (Rendering Task Configuration) file.
- Scene preview with option to set new camera position.

## RTC (Rendering Task Configuration) file format

Source: https://ii.uni.wroc.pl/~anl/dyd/RGK/.

RTC file consists of the following lines. The first seven are mandatory.

```
comment line
obj/file/path
output/file/path
k (recursion level)
xres yres (image resolution)
VPx VPy VPz (camera position)
LAx LAy LAz (point for camera to look at)
UPx UPy UPz (camera's up direction, optional)
yview (image height to focal length ratio, optional)
```

In absence of `UP` vector, one will be inferred from `VP` and `LA`.
`yview` defaults to `1.0`.

<details>
  <summary>Example</summary>
  
  ```
  #
  CornellBox-Original.obj
  CornellBox-Original.exr
  16
  600 400
  0.0 1.0 3.0
  0.0 1.0 0.0
  0 1 0
  1
  ```
</details>
