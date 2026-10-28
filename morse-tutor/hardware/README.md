# Llave, audio y batería — Bricohams

## Alcance y límites

La primera referencia es la CYD ESP32-2432S028R de 2,8". Su pinout se ha
contrastado con [PINS.md de ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md).
Los pines TFT/táctil y la variante ST7796 proceden de los perfiles de Ham-Clock.
No se ha confirmado el conector de expansión de la CYD de 4": no trasladar
la numeración del conector de 2,8" a otra placa.

**No conectar nada con la placa encendida.** Antes de alimentar, comprobar con
multímetro polaridad, continuidad y ausencia de cortocircuito entre 3,3 V/5 V/GND.
Las entradas del ESP32 son de **3,3 V**, no toleran 5 V.

## Llave recta o palas en CYD de 2,8"

| Función | GPIO / conexión |
|---|---|
| DIT | GPIO22, disponible en P3 y CN1 |
| DAH | GPIO27, disponible en CN1 |
| Común de llave | GND |
| Pull-ups externos del adaptador | 3,3 V de CN1 |
| Jack TRS de 3,5 mm | Punta = DIT, anillo = DAH, manguito = GND |
| Llave recta | Contacto seco entre DIT y GND; usar modo llave recta |

Comprobar **las etiquetas y orientación** del conector de cada unidad;
los cables comerciales pueden invertir colores. No se suministra un arnés
con orden de colores supuesto. Una llave mono TS puede poner DAH a masa:
usar un cable TRS o asegurar la configuración de llave recta.

Los contactos mecánicos y la llave externa están en paralelo. Se pueden usar
ambos conectores, pero no manipular dos llaves simultáneamente.
GPIO35 es otra entrada documentada, pero carece de pull-up interno; sólo usarlo
como alternativa configurada con pull-up externo. No es el valor predeterminado.

## Audio nativo y externo

- En la CYD habitual de 2,8", GPIO26 llega al amplificador de la placa y al
  conector de altavoz **P4**. P4 no es un GPIO ni una salida de línea.
  La carga e impedancia admisibles deben comprobarse en el esquema de la
  revisión comprada. Comenzar con carga/volumen apropiados al amplificador.
- Para altavoces activos, el adaptador toma **la señal de GPIO26 antes del
  amplificador**, no P4. Esto puede requerir soldar un cable: GPIO26 no figura
  como pin libre de los conectores GPIO. Si no se sabe identificarlo, no hacer
  esa modificación; utilizar la salida nativa.
- R5=10 kΩ y R6=1 kΩ atenúan aproximadamente 11:1 el PWM de 3,3 V.
  C3=1 µF no polar elimina la componente continua hacia la entrada de línea.
  El nivel final depende de la impedancia del equipo conectado.
- Para altavoz pasivo sin amplificador nativo utilizable, conectar J5 a
  **un módulo amplificador de audio ya montado**, con entrada de alta impedancia
  y alimentación de 5 V, por ejemplo PAM8403. Un altavoz pasivo **no** se conecta
  directamente a GPIO26 ni a J4/J5.
- El módulo amplificador no va integrado como chip en este PCB; debe montarse
  aislado y sujeto, con su propia documentación. Emplear sólo un canal, empezar
  a volumen mínimo y limitar el consumo del adaptador a 0,5 A.
  Si se necesita más potencia, alimentar el módulo directamente desde una
  fuente adecuada, no a través de estas pistas.
- En amplificadores BTL los dos terminales del altavoz son activos:
  **no conectar ninguno a GND** ni a la masa de una entrada de línea.

No se ha medido el ruido PWM ni la compatibilidad con cada revisión de amplificador.
La salida es un tono de práctica, no un códec, entrada de micrófono ni salida RF.

## Adaptador cableado, no shield

Archivos en [adapter/](adapter/):

- `bricohams-adapter.kicad_pcb`: placa editable de dos capas, 80 × 40 × 1,6 mm.
- `gerber/`: cobre superior/inferior, máscaras, serigrafía superior, contorno,
  taladro metalizado y taladro no metalizado.
- `bricohams-gerber-r1.zip`: el mismo juego de Gerber y taladros, empaquetado.
- `schematic.svg`: esquema de conectividad por etiquetas de red.
- `assembly.svg`: dibujo de montaje, **vista superior**.
- `BOM.csv`, `design.json`, `validation.json`: componentes, datos y comprobaciones.

Los conectores son **pads para soldar cables**, no footprints de jacks ni un
conector que se enchufe directamente a la CYD. Los jacks son de panel; verificar
sus contactos con multímetro. Los pads de J1 tienen paso de 5 mm; no montar
un header de 2,54 mm sobre ellos.

| Pad | Señal | Destino |
|---|---|---|
| J1.1 | GND | Masa de CYD |
| J1.2 | 3V3 | 3,3 V de CYD |
| J1.3 | GPIO_DIT | GPIO22 en el perfil habitual de 2,8" |
| J1.4 | GPIO_DAH | GPIO27 en el perfil habitual de 2,8" |
| J1.5 | AUDIO_PWM | GPIO26 antes del amplificador, si se accede de forma segura |
| J1.6 | 5V | 5 V regulados y protegidos, nunca la celda Li-ion |
| J2.1 / J2.2 / J2.3 | GND / KEY_DIT / KEY_DAH | Jack de llave externa |
| J3.1 / J3.2 / J3.3 | GND / KEY_DIT / KEY_DAH | Común y contactos de palas integradas |
| J4.1 / J4.2 | LINE_OUT / GND | Altavoz activo |
| J5.1 / J5.2 / J5.3 | 5V / GND / LINE_OUT | Alimentación y entrada de módulo amplificador |

R1/R2=1 kΩ están en serie con las entradas; R3/R4=10 kΩ son pull-ups a 3,3 V;
C1/C2=10 nF son filtros. No ofrecen aislamiento galvánico ni una clasificación
ESD garantizada. **No usar como interfaz de manipulación para un transceptor**:
una salida de radio puede tener tensiones incompatibles.

El generador comprueba continuidad de las diez redes y distancias geométricas
entre cobres de redes distintas. Los resultados no sustituyen ERC/DRC de KiCad,
revisión CAM del fabricante ni ensayos eléctricos. **R1 es un prototipo**:
antes de fabricar, abrirlo en KiCad, ejecutar DRC, revisar la vista Gerber y
encargar sólo una muestra. No declarar el PCB validado para producción.

## Batería para ambos equipos

La opción prevista es un **power bank USB comercial cerrado**, con protección
y salida regulada de 5 V, conectado al USB de la pantalla. La caja dispone de
hueco para una envolvente de hasta **100 × 60 × 22 mm** y paso de cable.
No se recomienda una marca ni se presupone que cualquier power bank cabe.

- No conectar una celda de 3,7 V al pin de 3,3 V ni al USB.
- No se ha diseñado un cargador de litio, BMS, cargador TP4056 ni circuito
  de reparto de carga. El PCB de llave/audio no carga baterías.
- Elegir un power bank que no se apague con la carga real de la pantalla.
  Medir el consumo con la retroiluminación y el audio previstos.
- La carga simultánea mientras se usa sólo es admisible si el fabricante
  declara soporte de *pass-through*. En caso contrario, apagar y retirar/
  desconectar el power bank para cargarlo según sus instrucciones.
- Para flashear, desconectar la alimentación de batería de la pantalla antes
  de conectar el USB del ordenador. No alimentar dos fuentes en paralelo ni
  unir sus salidas de 5 V sin un circuito de conmutación apropiado.
- Fijar el power bank sin presionar su envolvente, dejando espacio para cables
  y ventilación. No usar baterías hinchadas, dañadas o sin protección.
  No cargar el equipo encerrado ni dejarlo expuesto al sol o dentro de un coche.
- No hay medida de porcentaje de batería: la pantalla no tiene un sensor
  instalado para ello y el firmware no inventa un nivel de carga.

Autonomía orientativa, no medida:
`horas ≈ capacidad_Wh × eficiencia / (5 V × consumo_A)`.
Un pack etiquetado 3000 mAh a 3,7 V contiene unos 11,1 Wh; con eficiencia
0,8 y un consumo **supuesto** de 0,35 A, el cálculo da unas 5,1 horas.
Cambiar esos valores por medidas reales; mAh a 3,7 V no son mAh a 5 V.
