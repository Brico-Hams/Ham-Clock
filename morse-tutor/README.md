# Bricohams Morse Tutor v2 — firmware independiente

Nueva implementación Bricohams: interfaz por tarjetas, aprendizaje Morse,
manipulador/decodificador y sala local de mensajes para múltiples pantallas.
**No se ejecuta junto a Ham-Clock:** flashear uno sustituye al otro, sin arranque
dual. El reloj mantiene su código y sus perfiles separados.

- [Instalar el reloj](https://anatolbricoham.github.io/Ham-Clock/)
- [Instalar Morse Tutor](https://anatolbricoham.github.io/Ham-Clock/morse-tutor/)
- [Autoría, procedencia y licencias](THIRD_PARTY_NOTICES.md)

## Alcance y validación

Se comprueban compilaciones, protocolo, límites de mensajes, deduplicación
simulada entre tres nodos, codec, llave recta, squeeze A/B, tiempos PARIS,
Farnsworth y desbordamiento de `millis()`. El audio y muestreo de llave tienen
una tarea dedicada de 1 ms; el dibujo TFT y las peticiones HTTP no los ejecutan.

**No se ha probado físicamente ninguna pantalla ni la recepción entre placas
reales en esta sesión.** Compilar y simular no certifica el funcionamiento al
100 %, el alcance, el ajuste mecánico, el amplificador ni la autonomía.
La interfaz web se puede comprobar con respuestas simuladas, no equivale a
validar la radio ESP-NOW. No fabricar una serie sin probar un prototipo.

## Pantallas y Wokwi

| Perfil PlatformIO | Hardware objetivo | Táctil | Llave externa por defecto |
|---|---|---|---|
| `esp32-2432s028r` | CYD 2,8" ILI9341 | XPT2046, SPI software | DIT22 / DAH27 |
| `esp32-2432s028r-st7789` | CYD 2,8" ST7789, experimental | XPT2046, SPI software | DIT22 / DAH27 |
| `esp32-4in-st7796` | CYD 4" ST7796 | XPT2046, HSPI compartido | Deshabilitada |
| `freenove-32-st7789` | Freenove 3,2" FNK0114L, ST7789 | XPT2046, HSPI compartido | Deshabilitada |

El [proyecto Wokwi](https://wokwi.com/projects/479153720097609729) es un
**prototipo de menú**, no un tutor completo: sus actividades estaban pendientes.
Su ILI9341 + FT6206 y sus pines simulados **no corresponden** a la Freenove.
La navegación v2 es una implementación nueva de tarjetas RECIBIR/ENVIAR/CONFIG,
con actividades reales; no se ha importado su sketch ni `logo.h`.

El perfil Freenove se basa en el
[repositorio oficial FNK0103](https://github.com/Freenove/Freenove_ESP32_Display/tree/6da6dae20f2d0b0bb85854691ac52dc5479f5c51),
setup `FNK0114L_3.2_240x320_ST7789.h` del ZIP `TFT_eSPI_Setups_v1.4.zip`.
Comprobar **FNK0114L** en la placa: una descripción comercial parecida no
garantiza que un clon tenga el mismo cableado.

### Pines Freenove 3,2"

- ESP32 clásico; panel ST7789 240×320, horizontal 320×240, BGR/inversión.
- TFT HSPI: MISO12 / MOSI13 / CLK14 / CS15 / DC2 / RST−1; backlight27.
- XPT2046 en el mismo bus, CS33; se sondea presión sin asumir un IRQ36.
- SD VSPI independiente: CLK18 / MISO19 / MOSI23 / CS5.
- Audio: las fuentes oficiales habilitan `AUDIO_EN=4` en LOW y usan ambos DAC
  internos. El tutor genera un tono de baja excursión (112/144, reposo128) en
  GPIO25 y GPIO26, con temporizador independiente; comprobar el conector
  de altavoz y comenzar con volumen bajo. No conectar un altavoz a un GPIO.
- No se habilitan pines de palas por suponer un conector libre.
  **GPIO27 es backlight**, no la segunda pala del adaptador CYD de 2,8".
  Un cambio de pines necesita comprobar el esquema/conector de la placa real.

Para CYD de 2,8", [el adaptador y cableado documentados](hardware/README.md)
continúan siendo prototipos. [Las cajas y palas](mechanical/README.md) son para
una PCB nominal de 86×50 mm; **no se certifican para Freenove ni para 4"**.
La batería prevista sigue siendo un power bank USB protegido comercial,
no una celda de litio desnuda ni un cargador/BMS de diseño propio.

## Uso del tutor

1. En el primer inicio tocar las tres cruces; la calibración se guarda por
   perfil. Para repetirla mantener BOOT cuando arranca la aplicación, **no**
   durante el encendido/reset que puede activar el bootloader.
2. Tocar una tarjeta o usar `-`, `+`, `OK` para elegir categoría. Los submenús
   tienen cuatro tarjetas por página y flechas para cambiar de página.
   `< ATR` vuelve; la pulsación es individual, no autorrepetida.
3. **RECIBIR:** Koch, letras, palabras españolas sin tildes, números, mixto,
   archivos TXT, QSO e indicativos generados. `OK` pausa/reanuda; `-`/`+`
   cambian la velocidad del siguiente grupo. No se repiten indicativos reales
   como identidad de fábrica: los indicativos de ejercicio son ficticios.
4. **ENVIAR:** práctica libre, copiar uno/dos caracteres, palabra o indicativo,
   tarjetas, copia mental y Red CW. En los ejercicios de copiar se muestra el
   objetivo; tarjetas/copia mental lo ocultan durante la escucha. Responder
   con llave y dejar una pausa de siete unidades para evaluar. `OK` pasa al
   siguiente objetivo; en tarjetas revela la solución antes de continuar.
5. `DIT`/`DAH` táctiles actúan como palas virtuales. El táctil resistivo es
   de un punto: para squeeze simultáneo se necesitan dos palas físicas.
   En CONFIG elegir llave recta, iámbico A o B para los contactos externos.
   Llave recta usa DIT a masa; palas usan DIT y DAH a masa, contactos secos.
6. CONFIG permite velocidad de carácter 3–50 WPM, Farnsworth 3–velocidad de
   carácter, tono300–1800 Hz, llave, indicativo, brillo10–100 %, nivel Koch,
   tamaño de grupo y restaurar los ajustes **del tutor**. El indicativo se
   introduce con teclado táctil, hasta9 caracteres A–Z/0–9/`/`.
   `GUAR` guarda; volver sin guardar conserva el cambio solo en RAM.
7. La SD utiliza FAT y TXT ASCII en la raíz: hasta20 archivos, nombres
   menores de64 bytes. Se omiten caracteres no Morse con diagnóstico Serial.
   Al terminar el archivo se muestra el fin; para releerlo volver a seleccionarlo.
   No quitar la tarjeta durante la lectura.

## Red CW: todos los tutores, sin emparejar

Cada pantalla crea desde el arranque:

- **SSID:** `Bricohams-CW-XXXXXX` (sufijo único derivado de su MAC).
- **Clave WiFi:** `BricohamsCW`.
- **Web local:** `http://192.168.4.1`, una vez conectado a esa pantalla.
- **Canal común:** 1, con difusión ESP-NOW directa entre tutores v2.

Los tutores **no tienen que conectarse al AP de otro tutor**: escuchan la
difusión automáticamente. El AP es para teléfonos/ordenadores (hasta4 clientes).
No necesitan router, Internet ni buscar/emparejar una MAC de destino.
No se asocian a un router que pueda cambiar el canal común.

1. Flashear v2 en todas las unidades, encenderlas y mantenerlas dentro del
   alcance directo de 2,4 GHz.
2. Abrir **ENVIAR → Red CW** para escuchar la cola de mensajes; el origen se
   identifica por su indicativo. `TXT` activa texto+audio; `CW` deja el texto
   oculto y conserva el audio. Cambiar el modo limpia la transcripción local.
3. Manipular con palas/llave: se decodifica localmente y cada palabra se pone
   en difusión tras siete unidades de silencio. `ENV` envía el fragmento ya
   decodificado sin esperar otra pausa. La recepción de audio espera a que
   termine la palabra local; no es una mezcla simultánea de operadores.
4. Desde el móvil, abrir la web, escribir indicativo y hasta120 caracteres
   Morse/ASCII y pulsar **Enviar a todos**. El móvil no requiere una app.
   El equipo emisor no se reproduce a sí mismo; otros tutores sí reciben.
5. El historial web conserva los últimos12 mensajes. La recepción está
   activa también fuera de Red CW; hay12 plazas de reproducción,12 datagramas
   pendientes y8 transmisiones. Al llenarse se notifican descartes en Serial
   y en la web: no hay almacenamiento ilimitado ni histórico persistente.
   `CW`/checkbox desactiva la exposición de texto RX en la web; al reactivarlo
   se revela el historial todavía disponible.

Cada mensaje lleva MAC, sesión aleatoria de arranque, secuencia, indicativo y
checksum. Se envía tres veces con separación variable y se deduplica una
ventana de64 mensajes. Secuencias y sesiones evitan confundir reintentos o
reinicios. El formato v2 **no es compatible** con la adaptación v1/W8BH ni con
Morserino. Los ajustes/calibración v1 no se importan.

**Límites importantes:** no es una red mesh, no retransmite entre saltos,
no usa bandas de radioaficionado, no es LoRa ni Internet. El checksum detecta
errores, **no autentica** al remitente. El indicativo no se verifica como
identidad legal. El AP tiene clave conocida de fábrica; la difusión ESP-NOW
no está cifrada. No enviar secretos ni datos personales. No se garantizan
entrega, orden global o alcance; «en cola» no significa «recibido por todos».
La decodificación es desde mensajes digitales/llave, no reconocimiento de
audio de micrófono o de un receptor de radio. No hay salida para manipular RF.

## Compilación y publicación reproducible

Desde la raíz, con el entorno PlatformIO local del proyecto:

```powershell
.\.venv\Scripts\platformio.exe run -d morse-tutor -e esp32-2432s028r -e esp32-2432s028r-st7789 -e esp32-4in-st7796 -e freenove-32-st7789
node morse-tutor\tools\package-firmware.mjs
node morse-tutor\tools\stage-installer.mjs
node morse-tutor\tools\validate-release.mjs
```

Para crear un entorno nuevo sin instalar globalmente, usar `python -m venv .venv`
y el intérprete de ese entorno para instalar `morse-tutor/requirements.txt`.
Se fija `espressif32@6.13.0` / Arduino-ESP32 2.0.17 y TFT_eSPI2.5.43.
No sustituir por core3.x sin portar callbacks ESP-NOW y LEDC.
Para grabar por USB añadir `-t upload --upload-port COM5` al perfil elegido.

El empaquetador genera cuatro manifiestos v2 y16 imágenes con offsets
`0x1000/0x8000/0xe000/0x10000`, más SHA-256 y avisos de dependencias.
No se incluyen binarios generados en Git. El workflow de Pages construye el
reloj y los cuatro tutores, prueba, empaqueta y publica ambos instaladores en
cambios relevantes de `main`, etiquetas `v*` o ejecución manual.
El navegador instalador requiere HTTPS/localhost, Chrome/Edge de escritorio
con Web Serial y cable USB de datos.

### Pruebas nativas

En un sistema con g++ (también se ejecutan en CI):

```text
g++ -std=c++11 -Wall -Wextra -Werror -I morse-tutor/include morse-tutor/test/protocol_morse.cpp -o morse-tests
```

Ejecutar el binario resultante. En Windows se ha utilizado el compilador C++
de Zig ya instalado, sin añadir dependencias al proyecto.
`test/control_logic.cpp` contiene además comprobaciones estáticas de coordenadas
y conflictos de GPIO para los cuatro paneles.

La matriz pendiente de banco incluye TFT/color/orientación, calibración,
audio a bajo nivel, brillo sin cambiar tono, llave recta, squeeze A/B,
SD, tres o más placas con reintentos y congestión, recuperación de alimentación
y autonomía real. Los Gerber/STL conservan su condición de prototipos.
