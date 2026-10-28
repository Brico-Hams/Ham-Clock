# Cajas imprimibles Bricohams con batería

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
