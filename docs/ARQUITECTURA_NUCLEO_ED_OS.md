# E.D-OS — Arquitectura del núcleo propio (borrador 0.4)

## Objetivo y decisión

E.D-OS tendrá un **núcleo propio**. No arrancará ni dependerá del kernel Linux para funcionar.

Estudiaremos el código fuente de Linux para comprender soluciones probadas, comparar diseños y localizar componentes que puedan reutilizarse o adaptarse legal y técnicamente. Linux será una referencia de ingeniería y una posible fuente de componentes seleccionados; no será el kernel de E.D-OS ni su base de ejecución.

No podemos copiar subsistemas completos y pegarlos sin más: Linux está fuertemente integrado y cada pieza requiere estudiar dependencias, interfaces, arquitectura y licencia. Los fragmentos derivados mantienen sus obligaciones de licencia. La procedencia y las obligaciones se documentarán antes de incorporar código.

## Arquitectura objetivo

1. Firmware y arranque: UEFI/BIOS y un cargador de arranque.
2. Núcleo E.D-OS: interrupciones, memoria física y virtual, planificación, llamadas al sistema, IPC y temporizadores.
3. Controladores: modelo de drivers y control de acceso a dispositivos.
4. Servicios fundamentales: procesos, sistema de archivos, red, registro y configuración.
5. Seguridad: aislamiento, permisos, capacidades, claves, actualizaciones verificadas y recuperación.
6. Plataforma: runtime de aplicaciones, servicios, interfaz y accesibilidad.
7. Ecosistema: E.D. Connect, cuentas, sincronización y E.D. Cloud. La nube no será requisito para arrancar.
8. E.D.: IA integrada con permisos explícitos; sin acceso implícito a todos los datos o dispositivos.

## Primer contrato de arranque

Para evitar escribir un bootloader y un kernel a la vez, el primer prototipo usa **Limine v12.9.6** como cargador externo y su protocolo como contrato de hand-off. Esto NO significa usar Linux: Limine carga nuestro ELF y transfiere el control a nuestro propio punto de entrada.

El kernel solicita la revisión base 3 del protocolo, está enlazado en el espacio virtual higher-half x86_64 y configura una pila propia de 16 KiB antes de entrar en C. La inicialización escribe el marcador KERNEL_BOOT_OK por COM1.

La imagen ISO se prepara como híbrida BIOS/UEFI. La automatización pretende arrancar la imagen en QEMU por BIOS y comprobar el marcador serie. Limine es BSD-2-Clause; se fija la release y se verifica el SHA-256 de su archivo binario antes de usarlo.

## Estado de implementación — 12 de noviembre de 2026

Implementado en código:
- kernel/src/entry.S: entrada x86_64, pila y salto a kernel_main.
- kernel/src/kernel.c: inicialización COM1 y mensaje KERNEL_BOOT_OK.
- kernel/linker.ld: segmentos ELF higher-half RX/R/RW.
- kernel/Makefile: compilación Clang/LLD y objetivo ISO.
- kernel/limine.conf: entrada de arranque.
- kernel/scripts/build-iso.sh: construcción de ISO híbrida.
- .github/workflows/kernel-build.yml: pipeline de build y prueba en QEMU.

Verificación local: Clang/LLD generaron un ELF64 x86-64 estático con entry point 0xffffffff80000000. La construcción de ISO y el arranque QEMU están pendientes del resultado de GitHub Actions.

**No declarar todavía el arranque como conseguido.** El marcador KERNEL_BOOT_OK solo contará como éxito cuando aparezca en el log serie producido por QEMU en CI.

## Siguientes hitos técnicos

### N1 — Arranque mínimo independiente
- [x] Crear kernel propio separado de Linux.
- [x] Crear entrada x86_64 y pila inicial.
- [x] Compilar ELF higher-half.
- [x] Integrar cargador externo Limine y protocolo de hand-off.
- [x] Preparar construcción de ISO híbrida BIOS/UEFI.
- [ ] Confirmar construcción de ISO en CI.
- [ ] Confirmar arranque BIOS en QEMU mediante KERNEL_BOOT_OK.
- [ ] Confirmar arranque UEFI en QEMU.
- [ ] Añadir salida de diagnóstico y tratamiento de fallos de arranque.

### N2 — Control del hardware básico
- [ ] Solicitar y leer el mapa de memoria Limine.
- [ ] Crear gestor de memoria física (bitmap o estructura equivalente).
- [ ] Construir y cargar una IDT propia.
- [ ] Implementar excepciones de CPU y diagnóstico de fallos.
- [ ] Preparar asignador de memoria de kernel.
- [ ] Diseñar paginación propia y transición controlada desde las tablas del cargador.

### N3 — Ejecución
- [ ] Planificador y cambios de contexto.
- [ ] Procesos, aislamiento, syscalls e IPC.
- [ ] Temporizadores y gestión de interrupciones externas.

### N4 — Plataforma
- [ ] Drivers y sistema de archivos iniciales.
- [ ] Servicios de usuario, red y seguridad.
- [ ] Instalación, actualización y recuperación.
- [ ] Validación por modelo de hardware.

## Licencias y procedencia

El kernel Linux se distribuye principalmente bajo GPL-2.0-only, con excepciones y archivos que pueden tener condiciones específicas. Antes de incorporar código se comprobarán los avisos SPDX, licencia exacta, derechos de autor, dependencias y obligaciones de redistribución.

Limine se usa como cargador externo en el prototipo, no como kernel. Se conservarán su licencia BSD-2-Clause y avisos de copyright cuando se distribuyan sus binarios.

## Criterio de finalización

Una fase solo se marca como terminada cuando hay código, instrucciones reproducibles, versiones, configuración, logs y resultados de pruebas. Un commit, un documento o un ELF enlazado no demuestran por sí solos que el núcleo arranque.
