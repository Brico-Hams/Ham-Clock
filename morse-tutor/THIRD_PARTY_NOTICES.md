# Procedencia, autoria y licencias

## Firmware Bricohams v2

La implementacion actual de `src/main.cpp`, sus modulos y los nuevos componentes
de `include/` es una nueva implementacion de Bricohams bajo [MIT](LICENSE).
El sketch derivado `MorseTutor_ESP32.ino` no forma parte del firmware v2.
Se han escrito una nueva maquina de estados de interfaz, generadores de
ejercicios, codec Morse, temporizacion, decodificador de llave, manipulador
iambico y protocolo de difusion.

El alfabeto Morse internacional, los tiempos estandar y las funciones generales
de un tutor no se presentan como una invencion de Bricohams.
La idea de las tarjetas RECIBIR/ENVIAR/CONFIG se contrasto con
[el prototipo Wokwi facilitado por el usuario](https://wokwi.com/projects/479153720097609729).
No se ha incorporado su sketch, `logo.h` ni sus bibliotecas Adafruit de pantalla
o FT6206. El prototipo no implementaba los ejercicios.

Esto **no borra derechos de autor anteriores** ni convierte las bibliotecas
en obras propias. No se garantiza una conclusion juridica de independencia
absoluta; no se ha realizado una auditoria legal.

## Adaptacion historica v1

La version v1, disponible en el historial Git anterior a v2, era una adaptacion
del [Morse Tutor de Bruce E. Hall, W8BH](https://github.com/bhall66/morse-tutor),
commit `682b3b5e614bda9ff282fe0c1519cd0f2e25eb6b`.
Los avisos y licencia de esa version siguen siendo aplicables a sus copias
y a cualquier porcion reutilizada. Se conserva aqui su aviso completo:

```text
MIT License

Copyright (c) 2019 Bruce E. Hall
Copyright (c) 2026 Bricohams (CYD adaptation and new hardware/mechanical designs)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Bibliotecas y plataforma

Las dependencias no se atribuyen a Bricohams:

- [TFT_eSPI 2.5.43](https://github.com/Bodmer/TFT_eSPI/tree/v2.5.43):
  proyecto de Bodmer, con componentes Adafruit MIT/BSD. Sus avisos se distribuyen
  con el paquete; el empaquetador los copia junto a los binarios.
- [Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17):
  core Arduino LGPL-2.1 y componentes con licencias individuales. Incluye
  Preferences, SD, WebServer, WiFi y API ESP-NOW/LEDC.
- [ESP-IDF](https://github.com/espressif/esp-idf), utilizado por el core:
  licencias de Espressif y sus componentes; consultar los avisos del SDK.
- [ESP Web Tools 10.1.1](https://github.com/esphome/esp-web-tools/tree/10.1.1):
  Apache-2.0, utilizado por el instalador web.

El codigo fuente del tutor, configuracion fijada de PlatformIO y comandos de
construccion estan en este repositorio. Los paquetes fuente y avisos exactos
de las dependencias se obtienen mediante PlatformIO; no se modifican las
bibliotecas. Las fuentes y objetos intermedios que genera una compilacion local
permiten reconstruir/enlazar el firmware con un core compatible modificado.
Los avisos del framework y de TFT_eSPI se incluyen en la distribucion web.

No se ha copiado firmware, PCB ni caja de Morserino-32. No se declara
compatibilidad con su protocolo, LoRa o hardware. `BRICOHAMS` es un wordmark
textual; no se ha inventado un logotipo grafico oficial.
