# Cajas imprimibles Bricohams con batería

## Familia ampliada: mismo diseño para nueve variantes

[Catálogo público ilustrado y ZIP por variante](https://anatolbricoham.github.io/Ham-Clock/morse-tutor/docs/cajas.html) ·
[Manual de montaje y configuración](https://anatolbricoham.github.io/Ham-Clock/morse-tutor/docs/manual.html) ·
[Descargar toda la familia](downloads/bricohams-display-family.zip).

La familia nueva está en [family/](family/): **36 STL**, una base y tapa de
reloj y una base y tapa de tutor por variante. Palas, abrazadera y cupón
siguen siendo las cuatro piezas comunes originales de [stl/](stl/).
Cada ZIP individual contiene sus cuatro STL y esas cuatro piezas auxiliares,
perfil JSON, fuente SCAD, catálogo de parámetros, este manual e informe de mallas.
**No confundir con el ZIP antiguo de ocho piezas**, conservado debajo.

| Variante | Envolvente PCB de diseño, horizontal (mm) | Ventana centrada (mm) | Firmware tutor publicado |
|---|---|---|---|
| CYD 2,8 ILI9341/ST7789 | 86 × 50 | 70 × 43 | Dos perfiles 2,8 actuales |
| Freenove FNK0114L 3,2 | 96 × 60 | 68 × 51 | Perfil Freenove actual |
| CYD 4 ST7796 | 120 × 70 | 87 × 59 | Perfil 4 actual |
| ESP32-2432S032N | 96 × 60 | 68 × 51 | Ninguno |
| ESP32-2432S032R | 96 × 60 | 68 × 51 | Ninguno |
| ESP32-2432S032C | 96 × 60 | 78 × 55 | Ninguno |
| ESP32-3248S035R | 106 × 65 | 80 × 52 | Ninguno |
| LCDWIKI E32R35T/N35T | 101,50 × 55,50 | 79,24 × 51,50 | Ninguno |
| LCDWIKI E32R40T/N40T | 111,11 × 60,88 | 87,22 × 58,88 | Ninguno |

Todas son **envolventes de diseño pendientes de contraste con la placa**.
Sólo las dos últimas usan dimensiones de módulo publicadas por LCDWIKI:
[3,5"](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display) y
[4"](https://www.lcdwiki.com/4.0inch_ESP32-32E_Display).
Sus ventanas añaden 2 mm al área visual táctil, pero el centrado y la altura
de componentes siguen sin estar verificados. Las otras medidas no se presentan
como cotas del fabricante. Procedencia por perfil en [display-profiles.json](display-profiles.json).
Son familias conocidas/documentadas, no un ranking de ventas contrastado.

N/R/C requieren atención: las
[definiciones Sunton](https://github.com/rzeldent/platformio-espressif32-sunton)
identifican ST7789 en las tres, sin táctil en N, XPT2046 en R y GT911 en C.
La caja no implementa estos controladores. N necesita navegación sin táctil;
la web actual no sustituye todos los menús. C no obtiene gestos/multitáctil
por disponer de ese sensor. **No usar el perfil Freenove por semejanza.**

En todos los conjuntos se conserva la estética: esquinas redondeadas, tapa
con labio y wordmark BRICOHAMS grabado, cuatro tornillos, soportes perimetrales,
power bank USB cerrado y, en tutor, plataforma común de palas.
`family=true` calcula dimensiones exteriores desde PCB y batería; no escala
los agujeros M3 ni las palas. El vidrio se deja accesible por la ventana,
no se aprieta entre tapa y PCB. Los pasos de cable siguen siendo genéricos.

**Medir antes de imprimir:** PCB, vidrio/área táctil, desplazamiento respecto
al centro, altura de componentes, posición y acceso USB/SD, soldaduras,
aislamiento y retención. Si el LCD no está centrado hay que modificar la
posición de la ventana en el SCAD. No basta con agrandar la ventana.
Las bases requieren soportes bajo las ménsulas. Un perfil de caja para
Freenove/4" no habilita los pines de las palas desactivados en el firmware.

Para regenerar (Node 22, OpenSCAD y PowerShell; desde la raíz del repositorio):

```powershell
$env:OPENSCAD = 'C:\ruta\openscad.exe'
node morse-tutor\tools\export-case-family.mjs
# Opcional: un único perfil
node morse-tutor\tools\export-case-family.mjs sunton-32c
node morse-tutor\tools\validate-models.mjs --family
.\morse-tutor\tools\package-case-family.ps1
node morse-tutor\tools\generate-case-catalog.mjs
```

La validación exige cuatro piezas por perfil, superficies cerradas, una sola
componente, volumen positivo y dimensiones exteriores calculadas. El informe
está en [family-validation.json](family-validation.json). No comprueba ajuste,
estabilidad, ergonomía, térmica ni aislamiento.

## Diseño original de 2,8" (conservado)

Diseños originales paramétricos en [bricohams.scad](bricohams.scad).
Los ocho STL de [stl/](stl/) se han exportado con OpenSCAD y comprobado como
mallas cerradas, de un solo componente y volumen positivo.
**No se han impreso ni comprobado con la placa del usuario.**
Descarga conjunta: [bricohams-cases-stl.zip](bricohams-cases-stl.zip).

## Modelos y envolventes de referencia

| Pieza | Dimensiones exteriores aproximadas |
|---|---|
| Reloj: base | 116 × 90 × 48 mm |
| Reloj: tapa | 116 × 90 mm, espesor 2,4 mm más labio |
| Tutor: base con plataforma de palas | 174 × 132 × 60 mm |
| Tutor: tapa posterior | 174 × 76 mm, espesor 2,4 mm más labio |
| Cada pala | 14 × 47 × 16 mm |
| Cupón de ajuste | 40 × 25 × 3 mm |

Parámetros iniciales: PCB **86 × 50 mm**, ventana **70 × 43 mm**, conjunto
pantalla/PCB de **9 mm** y power bank cerrado de hasta **100 × 60 × 22 mm**.
Son envolventes de diseño, **no cotas certificadas del fabricante**.
La ventana permite acceso táctil pero no garantiza el bisel de cada CYD.
Los STL suministrados no sirven sin adaptar para una pantalla de 4"
ni se ha comprobado el ajuste de la Freenove 3,2" FNK0114L.

Medir PCB, vidrio, USB, microSD, conectores y altura de componentes antes de
imprimir. Cambiar `pcb_w`, `pcb_h`, `screen_w`, `screen_h`, `screen_stack`,
`case_w/h/z` y los pasos de cable según la revisión real. No modificar sólo
la ventana para montar otra placa. Las aserciones avisan de envolventes imposibles,
pero no comprueban todo el montaje.

## Impresión y montaje de la pantalla

1. Imprimir primero `bricohams-fit_coupon.stl`: comprobar agujeros M3 y hueco
   de jack de 6,5 mm. Ajustar compensación de agujeros/holgura en el slicer.
2. PETG es preferible para la caja; PLA puede deformarse con el calor.
   Referencia inicial: capa 0,2 mm, tres o cuatro perímetros, 20–30 % de relleno.
3. Imprimir la base apoyada en el suelo. **Necesita soportes** bajo las ménsulas
   de pantalla y los travesaños del adaptador; retirarlos antes de montar.
4. Imprimir la tapa con la cara exterior y la marca grabada hacia la cama;
   el labio queda hacia arriba. No orientar la tapa invirtiendo la ventana.
5. Los apoyos sostienen las esquinas de PCB sin suponer un patrón de agujeros.
   Comprobar que no presionan componentes o soldaduras. Usar espuma aislante
   fina y fijación no conductora en el perímetro del PCB, nunca sobre el vidrio.
   Las pequeñas abrazaderas STL son auxiliares, no taladrar una placa desconocida
   para montarlas. Si no se consigue una retención segura, adaptar los soportes.
6. La tapa se cierra con cuatro tornillos M3 autorroscantes para plástico de
   longitud adecuada al agujero piloto. No forzarlos ni dejar extremos que
   puedan alcanzar la batería. No hay insertos térmicos dimensionados.
7. Sujetar el power bank y el amplificador con correas/almohadillas aislantes,
   sin comprimir la envolvente del pack. El hueco no sustituye una certificación
   térmica. El diseño no incluye anclajes universales para todos los amplificadores.

## Palas integradas del tutor

La interfaz se inspira en la idea de un tutor autónomo con palas y conexiones
externas, **no reproduce el Morserino-32**. Son palas mecánicas de contacto,
no sensores capacitivos; se puede apretar DIT y DAH simultáneamente.

Material adicional orientativo:

- Dos pivotes M3 con tuercas y arandelas, longitud ajustada al conjunto
  (alrededor de 25 mm; comprobar antes de comprar).
- Dos tornillos M3 para contacto móvil y dos M3 para contacto fijo, con
  tuercas de bloqueo; longitudes de 16–25 mm según el ajuste.
- Dos gomas o muelles suaves de retorno y cable flexible.
- Jack TRS de panel de 3,5 mm para llave externa; jack de audio de panel opcional.
- Cuatro tornillos M3 y separadores/aislantes para el adaptador.

Colocar cada pala con su pivote sobre los apoyos `(60,43)` y `(114,43)` de la base,
extendiendo el brazo hacia el frontal. El brazo apoya sobre una arandela; dejar
holgura para girar sin rozar. Su agujero lateral de contacto queda hacia los
postes interiores `(74,23)` y `(100,23)`. La altura nominal de contacto es 12 mm;
ajustarla con arandelas y comprobar que ambos tornillos se enfrentan.

Conectar eléctricamente el tornillo móvil mediante terminal flexible a GND,
y el tornillo fijo de cada lado a KEY_DIT/KEY_DAH. Un pivote de plástico no hace
contacto eléctrico. Tensar suavemente el retorno hacia los postes exteriores;
ajustar la separación inicial a una fracción de milímetro y bloquear las tuercas.
Comprobar con multímetro que cada contacto cierra sólo su entrada y que ambos
se pueden cerrar juntos. Evitar cables tensos que dificulten el retorno.

El adaptador se sitúa sobre travesaños a 32 mm; su patrón de agujeros es
74 × 34 mm. El power bank queda debajo, con al menos unos milímetros de
separación respecto a soldaduras; la pantalla queda por encima. Comprobar la
longitud de los terminales THT y no colocar el amplificador suelto en ese hueco.
Los pasos de cable son genéricos, no agujeros garantizados para conectores concretos.

## Personalización y exportación

La marca es un wordmark textual original `BRICOHAMS` grabado en ambas tapas.
No se ha supuesto ni copiado un logotipo gráfico oficial. Se puede sustituir
el módulo `logo()` por un SVG autorizado cuando esté disponible.

Ejemplo con OpenSCAD en PATH, desde esta carpeta:

```powershell
openscad -o stl\bricohams-clock-base.stl -D 'product="clock"' -D 'part="base"' bricohams.scad
openscad -o stl\bricohams-clock-lid.stl -D 'product="clock"' -D 'part="lid"' bricohams.scad
openscad -o stl\bricohams-tutor-base.stl -D 'product="tutor"' -D 'part="base"' bricohams.scad
openscad -o stl\bricohams-tutor-lid.stl -D 'product="tutor"' -D 'part="lid"' bricohams.scad
node ..\tools\validate-models.mjs
```

La validación de malla detecta superficies abiertas y piezas desconectadas.
No prueba ajuste de tornillos, recorrido de palas, ergonomía, aislamiento,
ventilación ni colocación real de componentes. Fabricar y comprobar un prototipo
antes de publicar estos archivos como un kit listo para montar.
