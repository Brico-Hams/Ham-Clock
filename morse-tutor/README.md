# Bricohams Morse Tutor v2.1 — firmware independiente

Nueva implementación Bricohams: interfaz por tarjetas, aprendizaje Morse,
manipulador/decodificador y sala local de mensajes para múltiples pantallas.
**No se ejecuta junto a Ham-Clock:** flashear uno sustituye al otro, sin arranque
dual. El reloj mantiene su código y sus perfiles separados.

- [Instalar el reloj](https://brico-hams.github.io/Ham-Clock/)
- [Instalar Morse Tutor](https://brico-hams.github.io/Ham-Clock/morse-tutor/)
- [Autoría, procedencia y licencias](THIRD_PARTY_NOTICES.md)

## Novedades v2.1: cinco idiomas y entrenamiento ampliado

### v2.1.6: transición de portada a menú sin fotograma incompleto

Al terminar la portada de arranque, el primer menú se dibuja completo
con la retroiluminación temporalmente apagada y se vuelve a activar con el
brillo configurado. Esto evita que en el panel Freenove de 3,2 pulgadas se
vea una pantalla oscura a medio dibujar, con letras del menú apareciendo
de forma escalonada. El brillo 0 se permite solo internamente para esta
transición; los ajustes del usuario siguen entre 10 y 100 %. No se toca
la actualización parcial de los ejercicios, la clave CW ni ESP-NOW.

### v2.1.5: imagen de cabecera y botones de recepción

Para la pantalla Freenove FNK0114L de 3,2 pulgadas se baja el reloj del bus
TFT de 80 a 40 MHz para mejorar el margen de las transferencias SPI. La
miniatura BricoHams compone su fondo al mismo color de la interfaz (RGB565):
no usa la variante de `pushImage` con clave de transparencia, y sobrescribe
todos los píxeles, eliminando el rectángulo blanco y evitando restos previos.
El logo de arranque conserva el mismo diseño.

DIT/DAH solo aparecen en ejercicios de envío con manipulación iámbica.
No aparecen en RECIBIR ni como botón inerte dentro de Configuración. La
limpieza completa al cambiar de actividad y el refresco parcial dentro de
ella se mantienen. Es una corrección de software, pero si persisten sombras
tras un borrado completo conviene diferenciarlo de retención temporal del
panel real. Se mantiene íntegro el indicativo configurable y el audio.

### v2.1.4: logo sin fondo blanco y limpieza de pantallas

En el menú de Inicio se elimina el rectángulo blanco del logotipo. El
RGB565 del logotipo usa el blanco como clave de transparencia y adapta
los trazos oscuros a un tono claro para que se vean sobre el fondo del
tutor. La portada de inicio también utiliza el mismo fondo oscuro.
TFT_eSPI restaura el ajuste de bytes tras renderizar la imagen.

Se invalida la actualización parcial al cambiar de actividad, para que
no persistan textos de un ejercicio anterior. En cambios de vista se
mantiene un borrado completo; entre letras dentro de un mismo ejercicio
solo se redibujan las líneas necesarias, para evitar el parpadeo.
Si tras esta corrección se ven sombras incluso cuando el fondo ya se ha
borrado, revisar retención de imagen del panel físico, cableado y tensión
de alimentación; el software no puede descartar esa causa sin pruebas
sobre la placa real.

### v2.1.3: portada BricoHams e indicativo configurable

En un arranque normal se muestra durante un mínimo de 1,9 segundos una
portada con el **logotipo real** que funciona en la Freenove FNK0114L,
renderizado directamente por TFT_eSPI con `pushImage` y
`setSwapBytes(true)`. El RGB565 se empaqueta en RLE (4 tonos, unos 2,3 KiB)
para conservar memoria, sin depender de LVGL, SD ni archivos externos.
Se ha contrastado con el ejemplo 19 de Freenove para esa placa. El menú principal conserva las tarjetas de navegación,
sitúa la marca BricoHams arriba a la izquierda y muestra arriba a la derecha
el **indicativo real guardado en CONFIG → Indicativo** (por ejemplo, AO5BH).
La etiqueta de versión deja de ocupar la cabecera. Cambiar el indicativo en
CONFIG actualiza la cabecera al volver a Inicio sin borrar los ajustes.
En la primera instalación puede aparecer antes la calibración táctil.

### v2.1.2: refresco parcial de pantalla

Se corrige el parpadeo detectado en el prototipo: el fondo solo se borra al
cambiar de pantalla, no cada vez que se reproduce/decodifica una letra Morse.
Durante un ejercicio se actualizan únicamente las líneas que cambian y, cuando
el texto crece sin desplazarse, solo se pintan los caracteres nuevos. Sin
cambios en la lógica de audio, ESP-NOW, los pines ni las preferencias.

### Revisión de código (pendiente de nueva versión)

Correcciones y robustez, sin cambiar el manejo descrito abajo:

- **Antirrebote de 5 ms en la llave o palas físicas:** los rebotes del contacto
  ya no producen chasquidos ni elementos falsos. El retardo es igual al pulsar
  y al soltar, así que la duración de punto/raya se conserva.
- **Reproducción sin solapes:** si una reproducción terminaba justo cuando
  empezaba otra (por ejemplo, al tocar de nuevo una memoria), el tutor podía
  creer que el audio estaba libre y lanzar encima el siguiente ejercicio.
- **Tarjeta SD por palabras completas:** cada bloque de unos 110 bytes termina
  en un espacio, de modo que una palabra ya no se parte entre dos bloques
  (solo se divide una palabra más larga que el bloque entero).
- **Sin bloqueo al fallar el guardado:** si no se pueden guardar las
  estadísticas, Atrás avisa una vez y el segundo toque sale igualmente; las
  estadísticas siguen en memoria y se reintenta al salir de nuevo.
- **Inicio sin parpadeo:** la portada se redibuja solo cuando cambia el
  contador de mensajes de red, no cada 1,5 s.
- Código: ajustes identificados por nombre y no por posición en el menú, clave
  WiFi en una única constante y `/api/state` con una sola reserva de memoria.
  Nuevas pruebas nativas de antirrebote y de bloques SD.

### v2.1.1: pulsador táctil sin llave externa

En **Ajustes → Manipulador → Llave recta**, las actividades de envío
(incluida Red CW) muestran un botón ancho **Llave CW** en lugar de DIT/DAH.
Mantener pulsado genera una marca continua; soltar produce silencio:
la duración determina punto o raya, no un envío automático de un punto.
No requiere activar pines externos y funciona también en Freenove/4".
Salir del botón deslizando cancela la marca; hay que soltar antes de empezar
otra. Un toque iniciado fuera del botón o durante reproducción no se convierte
en marca al arrastrar dentro o al acabar el audio. En Red CW, la palabra
decodificada se difunde tras la pausa o con Enviar. Elegir Iambic A/B conserva
las palas virtuales DIT/DAH anteriores. El resistivo no permite pulsarlas juntas.

**Documentación ilustrada y descargas:**
[tutorial y manual detallado](https://brico-hams.github.io/Ham-Clock/morse-tutor/docs/manual.html) ·
[catálogo de cajas](https://brico-hams.github.io/Ham-Clock/morse-tutor/docs/cajas.html).
El manual incluye capturas de la web real con API simulada y vistas TFT
explícitamente ilustrativas, además de instalación, todos los ajustes,
ejercicios, red, conexiones, SD, batería y diagnóstico.

Se toman como referencia funciones generales de
[Morserino-32](https://www.morserino.info/), con código, vocabularios y
traducciones propios de Bricohams, **no una copia ni un firmware compatible
con Morserino**.

- **Español, inglés, alemán, francés e italiano** para menús, avisos de pantalla
  y web local del móvil. Elegir en Ajustes → Idioma pantalla o en el selector
  superior de la web. El cambio se guarda en la pantalla y se comparte entre
  sus clientes web; cada pantalla de la red mantiene su propio idioma.
  Los nombres de los idiomas siempre se muestran en su lengua nativa.
- **Vocabulario independiente:** Ajustes → Idioma palabras o el formulario de
  ajustes de la web. Incluye veinte palabras básicas originales por idioma;
  no se presenta como un diccionario completo. Cambia las actividades de
  palabras, copiar palabra y copia mental, no los indicativos, abreviaturas
  internacionales de QSO o archivos del usuario.
- **Entrenador eco:** Enviar → Entrenador eco. Escuchar un grupo del nivel Koch
  sin ver la solución; repetirlo con llave/palas. Tras siete unidades de
  silencio, o pulsando OK, se compara y se muestra el resultado. Si no se
  responde en 60 segundos tras terminar el audio, se registra como fallo.
  No es reconocimiento de voz o audio de micrófono.
- **Koch configurable:** Bricohams, letras primero, números primero o conjunto
  propio de 2–40 símbolos Morse ASCII distintos. Ajustes → Secuencia Koch;
  Ajustes → Conjunto propio abre el teclado y selecciona automáticamente ese
  conjunto al guardarlo. La web permite editarlo y seleccionarlo por separado.
  El nivel Koch limita el prefijo usado; se ajusta al cambiar a una secuencia
  más corta. No se declaran cursos LCWO/CW Academy/LICW copiados.
- **Adaptativo:** Enviar → Adaptativo funciona como eco, eligiendo solo
  caracteres del nivel Koch actual y dando más peso a los fallados.
  El peso está acotado: base1 más0–4 según el porcentaje de error;
  los caracteres aún no intentados tienen peso2. No modifica automáticamente
  la velocidad ni promete un sistema equivalente al de Morserino.
- **Estadísticas:** Enviar → Estadísticas o el panel de la web muestra grupos
  correctos/intentos y errores por carácter. Las respuestas se comparan por
  posición con el objetivo: omisiones y sustituciones penalizan el carácter
  esperado; caracteres sobrantes hacen fallar el grupo sin sumar intentos de
  caracteres ajenos al objetivo. Incluye todos los ejercicios evaluados,
  no solo el adaptativo. Se guardan al salir con Atrás: cortar la alimentación
  antes puede perder los resultados de esa sesión. Los contadores se reducen
  proporcionalmente al saturarse para evitar desbordamiento.
- **Ocho memorias:** Enviar → Memorias, dos páginas de cuatro posiciones.
  Tocar una reproduce su contenido; Editar abre el teclado. `...` cambia
  página de símbolos, `_` introduce espacio, Borrar elimina el último carácter
  y Guardar persiste. También se editan desde el panel Memorias de la web.
  Máximo120 caracteres Morse **tras normalizar**; guardar vacío vacía esa
  posición. La reproducción es local, no transmite a RF ni a la red.
- **Wordsworth:** Ajustes → Pausa palabras o el formulario web.
  Multiplicador1–5 aplicado al espacio estándar de siete unidades entre
  palabras, además de Farnsworth; no cambia el tono, las marcas o los
  espacios entre letras. En Red CW se conserva la separación normal para no
  modificar la reproducción de mensajes recibidos.

Las traducciones visibles conservan los acentos mediante conversión al juego
de caracteres de la fuente GLCD. Para enviar mensajes, editar memorias o leer
TXT UTF-8 se normaliza al Morse básico: `Ñ→N`, `Ä→A`, `É→E`, `ß→SS`,
`œ→OE`, `æ→AE`. Se rechazan caracteres sin equivalencia y UTF-8 inválido.
Los ejercicios integrados usan ASCII normalizado. No se añade Morse nacional
extendido ni cambia el formato de paquete: **v2.1 sigue comunicándose con v2.0**.
Un texto cuya expansión supere120 caracteres se rechaza; en SD se pausa el
ejercicio con un aviso si un bloque contiene texto no admitido.

El idioma, secuencia, pausas, memorias y estadísticas tienen su propio espacio
de preferencias. Se conservan los ajustes v2 anteriores. Restaurar cambia los
ajustes del tutor, pero **conserva memorias, estadísticas y calibración**.
La configuración del reloj y su firmware siguen separados.

No se incluyen en esta entrega juegos, bot QSO interactivo, modos Ultimatic
o no-squeeze, Bluetooth, iCW, Internet, LoRa, filtros/entrada de audio desde
un receptor ni salida de manipulación para transmisor. Algunos exigen
hardware extra y otros desarrollo/validación adicional.

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
continúan siendo prototipos. [Las cajas y palas](mechanical/README.md) incluyen
la caja nominal original 86×50 mm y una nueva familia de nueve variantes con
36 bases/tapas de reloj/tutor, incluidas Freenove y 4". Sus envolventes son
provisionales salvo las cotas de módulo expresamente documentadas:
**ningún ajuste físico está certificado**. Las nuevas Sunton N/R/C y LCDWIKI
tienen diseños mecánicos, no perfiles de firmware publicados.
La batería prevista sigue siendo un power bank USB protegido comercial,
no una celda de litio desnuda ni un cargador/BMS de diseño propio.

## Uso del tutor

1. En el primer inicio tocar las tres cruces; la calibración se guarda por
   perfil. Para repetirla mantener BOOT cuando arranca la aplicación, **no**
   durante el encendido/reset que puede activar el bootloader.
2. Tocar una tarjeta o usar `-`, `+`, `OK` para elegir categoría. Los submenús
   tienen cuatro tarjetas por página y flechas para cambiar de página.
   `< ATR` vuelve; la pulsación es individual, no autorrepetida.
3. **RECIBIR:** Koch, letras, palabras del idioma de vocabulario, números, mixto,
   archivos TXT, QSO e indicativos generados. `OK` pausa/reanuda; `-`/`+`
   cambian la velocidad del siguiente grupo. No se repiten indicativos reales
   como identidad de fábrica: los indicativos de ejercicio son ficticios.
4. **ENVIAR:** práctica libre, copiar uno/dos caracteres, palabra o indicativo,
   tarjetas, copia mental y Red CW. En los ejercicios de copiar se muestra el
   objetivo; tarjetas/copia mental lo ocultan durante la escucha. Responder
   con llave y dejar una pausa de siete unidades para evaluar. `OK` pasa al
   siguiente objetivo; en tarjetas revela la solución antes de continuar.
5. En modo llave recta, `Llave CW` táctil permite manipular manteniendo y
   soltando, sin hardware externo. En modos iámbicos, `DIT`/`DAH` actúan como
   palas virtuales. El táctil resistivo es
   de un punto: para squeeze simultáneo se necesitan dos palas físicas.
   En CONFIG elegir llave recta, iámbico A o B para los contactos externos.
   Llave recta usa DIT a masa; palas usan DIT y DAH a masa, contactos secos.
6. CONFIG permite velocidad de carácter 3–50 WPM, Farnsworth 3–velocidad de
   carácter, tono300–1800 Hz, llave, indicativo, brillo10–100 %, nivel Koch,
   tamaño de grupo y restaurar los ajustes **del tutor**. El indicativo se
   introduce con teclado táctil, hasta9 caracteres A–Z/0–9/`/`.
   `GUAR` guarda; volver sin guardar conserva el cambio solo en RAM.
7. La SD utiliza FAT y TXT ASCII/UTF-8 en la raíz: hasta20 archivos, nombres
   menores de64 bytes. Se normalizan los acentos latinos y se rechazan otros
   caracteres con diagnóstico Serial y aviso en pantalla.
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

El empaquetador genera cuatro manifiestos v2.1.0 y16 imágenes con offsets
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

Ejecutar el binario resultante; además de paquetes y temporización, cubre el
antirrebote de la llave física. En Windows se ha utilizado el compilador C++
de Zig ya instalado, sin añadir dependencias al proyecto (con `zig c++`,
cambiar `-fsyntax-only` por `-c -o <archivo>.o` en las comprobaciones estáticas).
Las suites adicionales se compilan con:

```text
g++ -std=c++11 -Wall -Wextra -Werror -I morse-tutor/include morse-tutor/test/training.cpp -o training-tests
g++ -std=c++11 -Wall -Wextra -Werror -I morse-tutor/include -I morse-tutor/src -I morse-tutor/test/support morse-tutor/test/trainer_settings.cpp morse-tutor/src/trainer_settings.cpp -o settings-tests
```

La primera comprueba los cinco catálogos y vocabularios, glifos, UTF-8,
normalización, conjuntos Koch, puntuación adaptativa, tiempos exactos Wordsworth
y el corte de bloques de la SD en palabras completas.
La segunda utiliza un almacenamiento simulado para probar ocho memorias,
independencia de idiomas, persistencia, escrituras fallidas y datos corruptos.
Los tres ejecutables se ejecutan en CI. Para probar el HTML y catálogo reales
con una API **simulada**, ejecutar `node morse-tutor\test\browser-fixture.mjs`
y abrir `http://127.0.0.1:8174`; terminar el proceso al acabar.
No usar esa fixture como prueba del firmware HTTP, NVS o radio ESP-NOW reales.

`test/control_logic.cpp` contiene además comprobaciones estáticas de coordenadas
y conflictos de GPIO para los cuatro paneles.

La matriz pendiente de banco incluye TFT/color/orientación, calibración,
audio a bajo nivel, brillo sin cambiar tono, llave recta, squeeze A/B,
SD, tres o más placas con reintentos y congestión, recuperación de alimentación
y autonomía real. Los Gerber/STL conservan su condición de prototipos.
