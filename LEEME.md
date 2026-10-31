# CHETO - plugin VST3 para FL Studio

Un solo plugin con todo adentro. Cadena de efectos, en este orden:

1. **Entrada** (ganancia)
2. **Anti-ruido** (reductor de ruido)
3. **Autotune** (nota, escala, velocidad, cantidad)
4. **EQ Pro** estilo Pro-Q: 6 bandas + corte de graves (HP) + corte de agudos (LP), con espectro en vivo
5. **Compresor** (con mezcla paralela)
6. **Saturador**
7. **Bitcrusher** (bits, reduccion de muestreo, mezcla)
8. **Delay**
9. **Reverb**
10. **MEJORAR TODO** (la perilla grande): una mini masterizacion automatica
11. **Salida** y **Limitador**

Cada modulo tiene su interruptor de encendido. El plugin arranca "limpio": autotune y anti-ruido apagados, y saturador / bitcrusher / delay / reverb con la mezcla en 0.

## Como usar lo principal

- **MEJORAR TODO**: subila de a poco. Limpia graves que no sirven, baja el "barro", suma presencia y aire, pega un poco con compresion suave y agrega un toque de saturacion. Es un arreglo general, no hace magia: si el sonido tiene un problema grande, conviene corregirlo con el EQ.
- **EQ Pro**: arrastra los puntos (izquierda/derecha = frecuencia, arriba/abajo = ganancia). Rueda del mouse sobre un punto = ancho (Q). Doble click en un punto = prender/apagar esa banda. Abajo salen las perillas de la banda elegida.
- **Anti-ruido**: prendelo y dejalo aprender el ruido de fondo (soplido, zumbido, ventilador). Ideal si al principio hay un momento de solo ruido. **Reduccion** = cuantos dB baja el ruido. **Sensibilidad** = que tan agresivo es (muy alto puede hacer sonar la voz "metalica"). **FIJAR** congela lo que aprendio: usalo cuando el ruido ya esta aprendido y tenes sonidos sostenidos (notas largas, pads) que no queres que se coma. Mete unos 23 ms de latencia mientras esta prendido; FL lo compensa solo.
- **Autotune**: prendelo, elegi la nota y la escala de tu tema. **Velocidad** al 0% = efecto robot (estilo T-Pain); mas alto = correccion mas natural. **Cantidad** = cuanto se corrige. Funciona para una sola voz (monofonica). Mete unos 23 ms de latencia mientras esta prendido; FL lo compensa solo.

## Como obtener el .vst3 (sin instalar nada)

1. Entra a tu repositorio de GitHub (el mismo de SantiStrip sirve).
2. Subi **todo el contenido de esta carpeta** reemplazando lo que habia (Add file > Upload files, arrastra todo, incluida la carpeta `.github` y la carpeta `Assets`). Si querés dejarlo prolijo, borra antes los archivos viejos.
3. Entra a la pestana **Actions**. Empieza solo una compilacion llamada "Compilar CHETO (Windows)". Tarda unos 10-15 minutos.
4. Cuando termine (tilde verde), abri esa ejecucion y baja **CHETO-Windows** de la seccion *Artifacts*.

## Instalarlo en FL Studio

1. Descomprimi el zip. Adentro hay una carpeta `CHETO.vst3` y un `CHETO.exe` (version suelta para probar sin FL).
2. Copia la carpeta `CHETO.vst3` a `C:\Program Files\Common Files\VST3\`
3. En FL Studio: *Options > Manage plugins > Find plugins*.
4. Cargalo en un slot del mezclador: aparece como **CHETO**.

## Si falla la compilacion

Copia el error de la pestana Actions (el texto en rojo) y pasamelo.

## Notas

- El logo esta metido dentro del plugin (`Assets/logo.png`).
- Las letras grandes de **CHETO** estan dibujadas a mano con trazos dentro del codigo (no usan ninguna fuente). Los titulos chicos usan Impact y Bahnschrift, que vienen con Windows. Si queres otras fuentes, pasame los archivos .ttf y las embebo.
