# photorealistic_cgfx
Path tracer developed during the Photorealistic Computer Graphics course @ Computer Science studies @ University of Wrocław.

## Features
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

## Example renders

![bmw-16384](https://user-images.githubusercontent.com/45500957/189182694-bca21816-6b21-47b2-8abf-315a50bb0246.png)

![CornellBox-Original-4096](https://user-images.githubusercontent.com/45500957/189183893-79a3ce5b-4799-4176-802d-c4674e33b7cd.png)

![CornellBox-Glossy-16384](https://user-images.githubusercontent.com/45500957/189184035-b2c55447-c248-4de1-ae56-03800b0cfc08.png)

![conference-8192-0](https://user-images.githubusercontent.com/45500957/189184566-5e93ab54-fbe4-4a52-ae62-fae7b6c26781.png)
