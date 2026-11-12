# E.D-OS — Arquitectura del núcleo propio (borrador 0.3)

## Objetivo y decisión

E.D-OS tendrá un **núcleo propio**. No arrancará ni dependerá del kernel Linux para funcionar.

Estudiaremos el código fuente de Linux para comprender soluciones probadas, comparar diseños y localizar componentes que puedan reutilizarse o adaptarse legal y técnicamente. Linux será una referencia de ingeniería y una posible fuente de componentes seleccionados; no será el kernel de E.D-OS ni su base de ejecución.

No podemos copiar subsistemas completos y pegarlos sin más: Linux está fuertemente integrado y cada pieza requiere estudiar dependencias, interfaces, arquitectura y licencia. Los fragmentos derivados mantienen sus obligaciones de licencia. La procedencia y las obligaciones se documentarán antes de incorporar código.

## Principios

- **Independencia de arranque:** E.D-OS no requerirá el kernel Linux.
- **Diseño propio:** definir interfaces, estructuras de datos y límites entre subsistemas.
- **Estudio antes de copiar:** analizar qué problema resuelve cada subsistema y decidir si conviene una implementación propia.
- **Reutilización selectiva:** incorporar código solo tras revisar viabilidad técnica y licencia.
- **Trazabilidad:** registrar origen, autores, licencia, modificaciones y dependencias.
- **Pruebas reproducibles:** no afirmar que el núcleo funciona hasta compilarlo y probarlo en un entorno controlado.

## Arquitectura objetivo

1. Firmware y arranque: UEFI y protocolo/cargador seleccionado.
2. Núcleo E.D-OS: interrupciones, memoria física y virtual, planificación, llamadas al sistema, IPC y temporizadores.
3. Controladores: modelo de drivers y control de acceso a dispositivos.
4. Servicios fundamentales: procesos, sistema de archivos, red, registro y configuración.
5. Seguridad: aislamiento, permisos, capacidades, claves, actualizaciones verificadas y recuperación.
6. Plataforma: runtime de aplicaciones, servicios, interfaz y accesibilidad.
7. Ecosistema: E.D. Connect, cuentas, sincronización y E.D. Cloud. La nube no será requisito para arrancar.
8. E.D.: IA integrada con permisos explícitos; sin acceso implícito a todos los datos o dispositivos.

La arquitectura es una propuesta, no una implementación completa.

## Estado de implementación — 12 de noviembre de 2026

Se ha creado una primera base de código original en kernel/src:
- entry.S: entrada x86_64 y pila inicial de 16 KiB.
- kernel.c: núcleo freestanding mínimo con diagnóstico por puerto serie COM1.
- linker.ld: distribución ELF64 inicial.
- Makefile: compilación con Clang/LLD.
- .github/workflows/kernel-build.yml: compilación automatizada prevista.

**Verificación local realizada:** Clang y LLD generaron un ejecutable ELF64 estático para x86-64. Esto solo confirma compilación y enlace.

**Todavía no es arrancable:** no se ha seleccionado ni implementado el protocolo/cargador de arranque. El punto de entrada espera que un cargador futuro transfiera el control en modo long 64-bit. No se ha probado en QEMU ni en hardware físico. No hay aún gestión de memoria, interrupciones, procesos ni drivers reales.

## Plan de trabajo

### N0 — Investigación y diseño
- [ ] Estudiar la estructura general del kernel Linux y sus subsistemas.
- [ ] Crear una matriz de subsistemas: función, interfaces, dependencias, complejidad y licencia.
- [ ] Definir requisitos y objetivos de hardware iniciales.
- [ ] Especificar memoria, interrupciones y llamadas al sistema.
- [ ] Seleccionar protocolo de arranque, toolchain y entorno reproducible.

### N1 — Núcleo mínimo independiente
- [x] Crear un proyecto de kernel E.D-OS separado del código Linux.
- [x] Crear una entrada x86_64, pila inicial y función C freestanding.
- [x] Generar localmente un ELF64 x86-64 con Clang/LLD.
- [ ] Elegir e implementar el protocolo/cargador de arranque.
- [ ] Arrancar el núcleo mínimo propio en QEMU.
- [ ] Verificar salida de diagnóstico en el emulador.
- [ ] Implementar gestión básica de memoria e interrupciones.

### N2 — Subsistemas esenciales
- [ ] Diseñar e implementar planificación de tareas y cambios de contexto.
- [ ] Diseñar memoria virtual y aislamiento entre procesos.
- [ ] Definir llamadas al sistema e IPC.
- [ ] Crear pruebas unitarias y de integración en QEMU.

### N3 — Reutilización evaluada
- [ ] Evaluar componentes concretos de Linux u otros proyectos por separado.
- [ ] Revisar licencia, dependencias, acoplamiento y coste de mantenimiento.
- [ ] Adaptar únicamente componentes aprobados, conservando avisos y trazabilidad.
- [ ] Mantener separado el código original y el derivado de terceros.

### N4 — Plataforma arrancable
- [ ] Añadir drivers y sistema de archivos iniciales.
- [ ] Crear servicios mínimos y entorno de usuario.
- [ ] Probar instalación, actualización y recuperación en máquina virtual.
- [ ] Validar cada modelo de hardware antes de anunciar compatibilidad.

## Licencias y procedencia

El kernel Linux se distribuye principalmente bajo GPL-2.0-only, con excepciones y archivos que pueden tener condiciones específicas. Antes de incorporar cualquier código se comprobarán los avisos SPDX, la licencia exacta, los derechos de autor, las dependencias y las obligaciones de redistribución. No se asumirá que todo el árbol tiene una única licencia ni que estudiar código obliga a copiarlo.

## Criterio de finalización

Una fase solo se marca como terminada cuando hay código, instrucciones reproducibles, versiones, configuración, logs y resultados de pruebas. Un commit, un documento o un ELF enlazado no demuestran por sí solos que el núcleo arranque.
