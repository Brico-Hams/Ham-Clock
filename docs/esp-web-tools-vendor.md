# ESP Web Tools: copia de distribución verificada para GitHub Pages

Los instaladores de Ham-Clock y Morse Tutor usan la **misma copia en el mismo origen**:
`webflasher/vendor/esp-web-tools/10.1.1/`. Se publica como parte del artefacto de GitHub Pages.
No se carga JavaScript desde UNPKG en el navegador.

## Procedencia y verificación

- Paquete: `esp-web-tools@10.1.1`, licencia Apache-2.0.
- Procedencia: registro npm oficial, archivo `esp-web-tools-10.1.1.tgz`.
- SHA-512 fijado en `tools/esp-web-tools.lock.json` (comprobado en archivos de bloqueo de npm de otros proyectos).
- Preparación: `node tools/vendor-esp-web-tools.mjs`.
- El script **rechaza** cambios en el archivo npm si no coincide el SHA-512, copia toda la carpeta `dist/web` y su licencia, y crea `SHA256SUMS` para cada recurso.
- En GitHub Actions se vuelve a comprobar el listado SHA-256. El paquete se adquiere **durante la compilación**, no durante la visita a la web; los archivos generados están en el artefacto de Pages, no en Git.
- Si el registro npm no responde, el flujo falla y no se distribuye una biblioteca sin verificar.

El archivo `tools/esp-web-tools.lock.json` se debe actualizar **solo con revisión deliberada** de versión y SHA-512. No sustituirlo por una versión flotante.

## Rutas

- Ham-Clock: `vendor/esp-web-tools/10.1.1/install-button.js`
- Morse Tutor: `../vendor/esp-web-tools/10.1.1/install-button.js`

La copia íntegra es necesaria porque los módulos pueden importar fragmentos JavaScript con nombres de hash. Los firmwares y sus manifiestos no se modifican.

## Validación antes de publicar

1. Revisar que el job `Verify vendored ESP Web Tools` termine correctamente en la PR.
2. Revisar la distribución resultante y ausencia de llamadas JavaScript a CDN en la pestaña Network del navegador.
3. Verificar que la selección de los perfiles de las dos aplicaciones funciona.
4. Probar una instalación con placa real en Chrome/Edge (Web Serial).
5. Solo entonces fusionar con `main`; el flujo principal compila y publica GitHub Pages automáticamente.

**Limitaciones:** Esta mejora evita la carga del código del CDN en tiempo de uso, pero la compilación requiere npm. La verificación de módulos externos es estática; no sustituye a una revisión completa del JavaScript del proveedor. El artefacto de Pages (y no el árbol Git) conserva la copia de los ficheros.
