# Automatic window sizing

Startup uses the primary monitor work area in Windows logical coordinates,
including taskbar exclusions and Windows display scaling. It chooses up to a
1024x768 client, preserves 4:3, leaves a small margin and centers the complete
outer frame. Renderer initialization uses the actual resulting client size.
The app retains its previous default Windows DPI virtualization for both native
controls and the Direct3D child surface. Do not enable GDI-scaled unaware mode:
it caused incompatible clipping/painting between the GDI toggle and D3D surface
on the owner's desktop. Work-area fitting does not require changing DPI mode.
This is not a rewrite to per-monitor native pixel layouts.

Minimum tracking bounds fit the current monitor, rather than forcing 940x700.
Resolution/work-area changes and completing a window move clamp a normal window
inside the nearest monitor work area. F11 uses the monitor containing the app.
The renderer continues preserving 4:3 on resize/fullscreen. Narrow editor windows
put edit buttons below transport, and the transport shortens its seek slider.

Validation: synthetic work-area bounds at 640x440, 800x560, 1024x728, 1280x680,
1920x1040 and a negative-origin monitor; all resulting outer frames fit and
client sizes stay 4:3. Render capture covers a compact editor/player. Actual VM
and multi-monitor DPI transitions require checking on those machines.

The original synthetic/render tests skipped wWinMain startup and missed this
DPI-mode regression. tests/run_startup.bat now runs the actual product entry
point, checks its real HWND DPI context and child bounds, resizes it, then closes
that test process. It does not claim to verify desktop compositor pixels.
