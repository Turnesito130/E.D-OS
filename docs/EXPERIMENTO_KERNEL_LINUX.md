# E.D-OS: experimento con kernel Linux como base

## Estado de esta decisión

Esta rama es un experimento aislado. No sustituye ni borra el trabajo del kernel propio que continúa en `arquitectura/0.1`. No se modifica `main`.

La hipótesis que se va a evaluar es construir una edición de E.D-OS usando el kernel Linux como base técnica, con identidad, experiencia de usuario, servicios, configuración y aplicaciones propias.

## Qué significa «hacerlo nuestro»

Podemos mantener una distribución/fork propio: conservar una copia del código fuente, mantener nuestros parches, decidir cuándo integrar cambios de upstream y continuar desarrollándolo aunque el proyecto original deje de publicarse.

No significa adquirir propiedad exclusiva sobre el código Linux ni poder impedir que otros usen el código cubierto por GPL. Al distribuir un kernel Linux modificado debemos cumplir GPL-2.0, conservar avisos y proporcionar el código fuente correspondiente y los derechos de modificación/redistribución que exige la licencia. La licencia exacta de cada componente adicional debe revisarse por separado.

## Principios obligatorios

1. **Control del producto:** repositorio, marca, interfaz, configuración, servicios y hoja de ruta de E.D-OS bajo nuestro control.
2. **Independencia operativa:** mantener una copia verificable del código fuente, parches, configuración, herramientas de compilación y documentación necesaria para reconstruir nuestra versión.
3. **Sin dependencia de una empresa única:** no depender de una distribución comercial, tienda propietaria, servicio cloud único o sistema de compilación cerrado.
4. **Transparencia de licencias:** inventario de licencias y avisos antes de redistribuir.
5. **Reversibilidad:** el kernel propio de E.D-OS permanece conservado en `arquitectura/0.1`; este experimento no lo reemplaza.
6. **Pruebas antes de promesas:** distinguir compilación, arranque en emulador, pruebas de funciones y compatibilidad con hardware real.

## Plan de validación

### Fase 0 — Preparación
- Confirmar arquitectura de CPU objetivo (inicialmente x86_64).
- **Versión elegida para el primer prototipo: Linux 6.18.63 LTS**, publicada como longterm en kernel.org el 10-11-2026; la página oficial indica una fecha proyectada de fin de vida en diciembre de 2028. Esa fecha es una proyección que puede cambiar, no una garantía de soporte. Fuente: https://www.kernel.org/releases.html
- Registrar y verificar criptográficamente el archivo fuente antes de usarlo en una compilación de distribución.
- Preparar un entorno de compilación reproducible.
- Definir configuración mínima y un initramfs de prueba.

### Fase 1 — Primer arranque
- Compilar el kernel seleccionado.
- Generar una imagen de arranque mínima con initramfs.
- Arrancar en QEMU mediante CI.
- Comprobar en consola serie que el kernel inicia y ejecuta el proceso init de prueba.

### Fase 2 — Identidad E.D-OS
- Crear el sistema de archivos raíz y los servicios iniciales propios.
- Diseñar la sesión y el escritorio E.D-OS.
- Registrar paquetes, componentes y licencias.

### Fase 3 — Independencia de mantenimiento
- Mantener un repositorio/fork y parches versionados.
- Documentar cómo reconstruir una imagen desde cero.
- Probar que el proceso funciona sin depender de los servidores de una distribución concreta.

## Criterio para decidir

No se declara que la estrategia sea mejor hasta comparar:
- tiempo real para conseguir un arranque funcional;
- complejidad de mantenimiento y actualizaciones de seguridad;
- compatibilidad de hardware;
- tamaño y rendimiento;
- obligaciones de licencias;
- cuánto control real conserva el equipo E.D-OS.

## Situación actual

El workflow `.github/workflows/linux-base-boot.yml` intenta compilar Linux 6.18.63 x86_64, crear un initramfs mínimo con BusyBox y arrancarlo en QEMU. La descarga está fijada a una versión, pero la verificación criptográfica del tarball todavía debe añadirse antes de considerar el proceso reproducible y apto para distribución. El arranque solo se considerará demostrado cuando CI termine correctamente y el log serie confirme que se ejecutó `/init`.
