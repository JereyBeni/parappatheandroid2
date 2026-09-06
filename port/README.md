# PaRappa the Rapper 2 - Android / PC Port Scaffold

Este es el esqueleto del port experimental.

## Target

**SOLO** la **July 12 2001 NTSC-J Prototype** (Hidden Palace dump).

| Campo  | Valor |
|--------|-------|
| Archivo | `PS2 - Parappa 7-12-07.bin` |
| Tamaño | 4159078400 bytes |
| SHA1   | `28964c33cee578ec3ce476285067044243363d08` |
| MD5    | `6ed6bc34ecfbafd2c4d9a12c85edb54f` |
| CRC32  | `F9C41366` |

Si cargás cualquier otra ISO (retail, demo, otra prototype) el checker tira **warning** y te avisa.

## Estructura

```
port/
├── common/          → rom_check (compartido Android + PC)
├── pc/              → TestForIssues (binario de desarrollo)
├── android/         → código nativo + scaffold del APK
└── .github/workflows/build.yml
```

## Builds

### PC - TestForIssues (Windows prioritario)

**Windows (Visual Studio / MSVC):**
```bash
cd port/pc
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
build\Debug\TestForIssues.exe "ruta\a\PS2 - Parappa 7-12-07.bin"
```

**Linux (opcional):**
```bash
cd port/pc
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/TestForIssues "ruta/a/PS2 - Parappa 7-12-07.bin"
```

### Android

El workflow de GitHub Actions genera la estructura mínima del proyecto Android + el native lib con el checker de ROM.

Todavía falta:
- Frontend real de selección de ISO
- Renderer (Vulkan / software GS)
- Port del código decompilado

## Próximos pasos reales

1. Meter el código decompilado de `src/` del decomp como librería
2. Implementar o conectar un backend de GS (paraLLEl-GS sería ideal a futuro)
3. Hacer el file picker + UI de warning en Android
4. Empaquetar assets extraídos (cuando el decomp avance más)

## Warning importante

Este proyecto **no incluye** la ISO ni los assets del juego.  
Tenés que tener tu propia copia legal de la prototype.
