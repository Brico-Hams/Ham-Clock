# Bricohams Morse Tutor — firmware independiente

Adaptación del [Morse Tutor ESP32 de Bruce E. Hall, W8BH](https://github.com/bhall66/morse-tutor),
basada en el commit `682b3b5e614bda9ff282fe0c1519cd0f2e25eb6b`.
Convive en este repositorio con Ham-Clock, **pero no se ejecutan juntos**.
El instalador permite elegir qué firmware grabar. No es un arranque dual.

## Estado real

- Firmware con perfiles PlatformIO para CYD ILI9341 de 2,8", ST7789 de 2,8"
  y ST7796 de 4". El perfil ST7789 sigue siendo experimental.
- Conserva recepción Koch, letras, palabras inglesas, números, mixto, SD, QSO,
  indicativos de práctica, envío, copia, tarjetas, copia mental y ESP-NOW.
- Pantalla de arranque Bricohams y menús principales en español.
- Control táctil sin encoder externo: `-`, `+`, `OK`, `DIT` y `DAH`.
- Llave recta y palas físicas: GPIO22/GPIO27 en la CYD de 2,8".
  En la variante de 4" **no se habilitan pines externos sin comprobar el PCB**:
  GPIO27 se utiliza para la retroiluminación.
- Audio PWM en GPIO26 y brillo PWM independiente, sin compartir temporizador.
- Calibración táctil de tres puntos, ajustes persistentes y lectura de la SD
  en su bus independiente. Sin tarjeta se muestra un aviso y se vuelve al menú.
- [Adaptador original con Gerber, esquema, BOM y alimentación](hardware/README.md).
- [Dos cajas imprimibles y palas mecánicas originales](mechanical/README.md).

**No se ha probado físicamente esta adaptación.** Compilar, verificar los STL y
comprobar la continuidad geométrica del PCB no certifica el funcionamiento de una
pantalla concreta, la compatibilidad de audio ni el ajuste mecánico.
No se declara «100 % validado» ni se recomienda fabricar una serie todavía.

## Compilar

Desde esta carpeta, con PlatformIO instalado:

```powershell
pio run -e esp32-2432s028r
pio run -e esp32-2432s028r-st7789
pio run -e esp32-4in-st7796
```

Si no está instalado, usar un entorno local, sin instalación global:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m platformio run
```

La plataforma está fijada a `espressif32@6.13.0` (Arduino-ESP32 2.0.17);
los callbacks ESP-NOW y LEDC corresponden a esa API. No sustituir el core
por Arduino-ESP32 3.x sin portar ambas interfaces.

Para grabar por USB, sustituir el puerto por el de la placa:

```powershell
pio run -e esp32-2432s028r -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

**La grabación sustituye el reloj.** No se ha grabado ninguna placa desde esta sesión.

## Instalador independiente y elección del suscriptor

El flasher existente del reloj se conserva en `../webflasher/index.html`.
Después de compilar los tres perfiles:

```powershell
node tools\package-firmware.mjs
node tools\stage-installer.mjs
```

El segundo comando prepara `../webflasher/morse-tutor/`, que se publica junto
al reloj mediante el workflow de GitHub Pages. El primer comando copia los cuatro
binarios con offsets explícitos, genera tres manifiestos y un archivo SHA-256.
El hook de PlatformIO obtiene `boot_app0.bin` del framework realmente usado.
Los binarios generados no se incluyen en Git.
El workflow se ejecuta en cambios de tutor/instalador en `main`, en etiquetas
`v*` o manualmente; publica ambos instaladores en el mismo sitio.

El instalador requiere HTTPS o localhost, Chrome/Edge de escritorio y un cable USB
de datos. La selección de un panel no detecta automáticamente el modelo.

## Uso

1. En el primer inicio, tocar las tres cruces de calibración con un lápiz romo.
   Para repetirla, mantener `BOOT` al iniciar la aplicación, una vez terminado
   el arranque del ESP32. No mantenerlo al encender: eso puede activar el bootloader.
2. `-` y `+` sustituyen el giro del encoder; mantenerlos repite el movimiento.
   `OK` confirma, termina una actividad o avanza un ajuste según el menú.
3. `DIT` y `DAH` son contactos virtuales. El táctil resistivo es de un solo punto:
   **no permite apretar ambas palas a la vez**. Para practicar squeeze iámbico
   hay que conectar las dos palas físicas.
4. En `Config > Llave`, enviar un punto para elegir la orientación; soltar;
   enviar punto para llave recta o raya para palas; soltar; elegir iámbico A/B.
5. `Config > Indicati` permite introducir el indicativo en Morse (máximo 9
   caracteres). El valor de fábrica es `NOCALL`, no un indicativo de terceros.
6. La SD utiliza FAT y archivos de texto ASCII en la raíz. Se reconocen hasta
   20 archivos, con nombres menores de 64 bytes.
7. El modo `Dos vias` usa ESP-NOW entre **dos unidades Bricohams**: SSID
   `Bricohams Tutor`, canal 1. No es una red para Internet ni un canal cifrado.
   No se garantiza el emparejamiento con firmware W8BH sin modificar.

## Pines

`include/BoardConfig.h` reúne perfiles y restricciones. Los GPIO de pantalla,
táctil, SD, audio y backlight están reservados; la compilación rechaza conflictos
en los pines de llave. Véase [el cableado completo](hardware/README.md).
El LED del ESP32 no se usa como salida de llave: en esta placa GPIO2 es el DC
del TFT. **No hay salida para manipular un transmisor de radio.**

## Reproducir comprobaciones

Desde la raíz del repositorio:

```powershell
node morse-tutor\tools\generate-adapter.mjs
node morse-tutor\tools\validate-models.mjs
node morse-tutor\tools\validate-release.mjs
```

`test/control_logic.cpp` contiene aserciones estáticas de coordenadas, límites,
calibración y separación de temporizadores. Se compila con el compilador
Xtensa instalado por PlatformIO. La matriz de validación física sigue abierta:
pantalla, calibración, brillo sin modificar el tono, llave recta, squeeze A/B,
altavoz a bajo volumen, SD vacía/llena, dos unidades ESP-NOW y autonomía real.

## Licencias y marca

Se conserva el aviso del autor y la [licencia MIT original](LICENSE).
Los nuevos diseños de caja, interfaz y wordmark textual Bricohams son originales,
incluidos bajo MIT junto a este subproyecto. El reloj mantiene su propia licencia.
No se han copiado diseños ni firmware de Morserino-32: la inspiración es
funcional (equipo autónomo, palas y conectores), no una compatibilidad declarada
con sus protocolos, LoRa o hardware. No se dispone de un logotipo oficial:
las piezas llevan un wordmark textual `BRICOHAMS`, no un logo oficial inventado.
