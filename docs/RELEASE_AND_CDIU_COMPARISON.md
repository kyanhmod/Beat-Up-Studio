# Portable release and Cdiu comparison — 2026-10-16

Release: dist/BeatUpStudio.exe. One executable contains DDS, six SOUND effects,
and the application icon. Music and editable SLK files remain external.
218 named resources compress from 14,870,995 bytes to 2,238,103 bytes. Resource
loading is shared by D3D textures, GDI editor sprites and effect audio; no temporary
extraction is used. Original textures take priority over legacy portable aliases.
Only bg.dds still comes from hud_clean. Static C++ runtime; Windows 10/11 x64,
Direct3D 11 and system XAudio2 are required. No installer is needed.

| Area | Current evidence versus Cdiu |
| --- | --- |
| Manual input | User preferred the current feel; timing/judgement rules retained. |
| Deployment | Single portable native x64 EXE; no VB6/DirectX8 application dependency. |
| Player/editor | Same-window toggle, preview, Space transport, independent volumes. |
| Editor layout | Grid fills available width/height; redundant horizontal scrollbar removed. |
| Files | Combined OGG/SLK picker and paired drag/drop; selected timing retained. |
| Compatibility/tools | Cdiu remains more complete: BPM/speed events, extra formats, recovery and several transformation/library tools are pending. |
| Machine compatibility | Portable launch passed locally; broader hardware/Windows testing is still needed. |

There is no evidence for complete Cdiu feature parity or universal performance
superiority. The 74-entry source inventory remains the migration checklist.

Validation: session suite with embedded resources, real editor/document suite,
resource/icon checks, and standalone EXE launch/clean exit in a folder containing
only the EXE. UI inspection uses GPU readback plus native control composition.
Cleanup removes generated compiler objects, obsolete copied runtime assets and
duplicate bin song fixtures. In the subsequent owner-requested cleanup, sample
OGG/WAV music and the Cdiu checkout were removed. Six required effect sources
were preserved in assets/sound; textures, chart samples and the accepted input
baseline remain. Building the app no longer depends on a Cdiu checkout.
