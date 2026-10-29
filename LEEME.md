# SantiStrip - plugin VST3 multiefecto para FL Studio

Cadena de efectos en un solo plugin, en este orden:

1. **Entrada** (ganancia)
2. **Ecualizador** de 4 bandas: low shelf, 2 medios con Q, high shelf
3. **Compresor** (umbral, ratio, ataque, release, makeup y mezcla paralela)
4. **Saturador** (drive y mezcla)
5. **Delay** (tiempo, feedback, mezcla)
6. **Reverb** (tamano, damping, mezcla)
7. **Limitador** (techo) y ganancia de **salida**

Cada seccion tiene su boton de encendido. Delay, reverb y saturador arrancan con la mezcla en 0 para que el plugin suene limpio al cargarlo.

## Como obtener el .vst3 (sin instalar nada)

1. Crea una cuenta gratis en https://github.com y un repositorio nuevo (por ejemplo `SantiStrip`).
2. Subi **todo el contenido de esta carpeta** al repositorio (boton *Add file > Upload files*, arrastra todo, incluida la carpeta `.github`).
3. Entra a la pestana **Actions**. Va a empezar solo una compilacion llamada "Compilar SantiStrip (Windows)". Tarda unos 10-15 minutos la primera vez.
4. Cuando termine (tilde verde), abri esa ejecucion y baja **SantiStrip-Windows** de la seccion *Artifacts*. Es un zip.

## Instalarlo en FL Studio

1. Descomprimi el zip. Adentro hay una carpeta `SantiStrip.vst3` y un `SantiStrip.exe` (version suelta para probar sin FL).
2. Copia la carpeta `SantiStrip.vst3` a `C:\Program Files\Common Files\VST3\`
3. En FL Studio: *Options > Manage plugins > Find plugins* (o "Verify plugins").
4. Cargalo en un slot de mezclador: aparece como **SantiStrip**.

## Si falla la compilacion

Copia el error de la pestana Actions (el texto en rojo) y pasamelo.
