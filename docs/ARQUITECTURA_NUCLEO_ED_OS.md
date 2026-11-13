# E.D-OS — Arquitectura del núcleo propio (borrador 0.5)

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

## Contrato de arranque actual

Para evitar escribir un bootloader y un kernel a la vez, el prototipo usa **Limine v12.9.6** como cargador externo y su protocolo como contrato de hand-off. Esto NO significa usar Linux: Limine carga nuestro ELF y transfiere el control a nuestro propio punto de entrada.

El kernel solicita la revisión base 3 del protocolo, está enlazado en el espacio virtual higher-half x86_64 y configura una pila propia de 16 KiB antes de entrar en C. La inicialización escribe diagnósticos por COM1.

La ISO híbrida BIOS/UEFI y la prueba de arranque BIOS en QEMU están automatizadas en GitHub Actions. El criterio de éxito no es que el ELF compile: el log serie de QEMU debe contener todos los marcadores esperados, incluida la prueba de excepción.

Limine es BSD-2-Clause; se fija la release y se verifica el SHA-256 del archivo binario antes de usarlo.

## Estado de implementación — rama `arquitectura/0.1`

El código actual identifica el kernel como **0.0.4**. Hay más que una entrada mínima:

- `kernel/src/entry.S`: entrada x86_64, pila inicial y salto a `kernel_main`.
- `kernel/src/kernel.c`: inicialización COM1, validación del protocolo, secuencia de pruebas y parada controlada ante errores.
- `kernel/src/memory.c`: lectura del mapa de memoria Limine y HHDM; asignador físico bootstrap de páginas de 4 KiB; puesta a cero al asignar; lista enlazada de páginas liberadas y reutilización.
- `kernel/src/idt.c` y `kernel/src/interrupts.S`: IDT de 256 entradas y stubs para las primeras 32 excepciones.
- `kernel/linker.ld`: segmentos ELF higher-half RX/R/RW.
- `kernel/Makefile`: compilación freestanding con Clang/LLD.
- `kernel/scripts/build-iso.sh`: construcción de ISO híbrida.
- `.github/workflows/kernel-build.yml`: compilación, arranque BIOS en QEMU y comprobación de marcadores por puerto serie.

### Pruebas que el workflow exige

- Mapa de memoria disponible y tabla IDT instalada.
- Asignación de una página inicializada a cero.
- Liberación y rechazo de doble liberación.
- Reutilización de la misma página y comprobación de puesta a cero.
- Excepción breakpoint (vector 3) capturada y parada controlada.

**Estado de verificación:** el repositorio contiene el workflow que ejecuta estas pruebas, pero en esta revisión no he confirmado un resultado verde reciente de GitHub Actions ni he inspeccionado un log de QEMU satisfactorio. Por tanto, el arranque todavía no se declara verificado.

## Límites técnicos conocidos

- El asignador no mantiene todavía un bitmap/tabla de propiedad por página. `pmm_free_page` comprueba alineación, región usable y doble liberación dentro de su lista, pero depende del contrato del llamador de liberar solo páginas obtenidas mediante `pmm_alloc_page`.
- No hay bloqueo para concurrencia ni soporte SMP.
- La IDT instala stubs para excepciones; todavía no hay GDT/TSS/IST propios ni controladores de IRQ externos.
- No existe aún gestor de memoria virtual propio, manejo completo de page faults, heap del kernel, planificador, procesos, hilos, syscalls ni modo usuario.
- El prototipo sigue en ring 0 y no es un sistema operativo de uso diario ni seguro para datos personales.

## Siguientes hitos técnicos

### N1 — Arranque mínimo independiente
- [x] Kernel propio separado de Linux.
- [x] Entrada x86_64 y pila inicial.
- [x] ELF higher-half y compilación freestanding.
- [x] Cargador externo Limine y protocolo de hand-off.
- [x] Construcción automatizada de ISO híbrida.
- [ ] Confirmar build de ISO en CI.
- [ ] Confirmar arranque BIOS en QEMU con todos los marcadores.
- [ ] Añadir y confirmar arranque UEFI en QEMU.
- [ ] Diagnóstico y tratamiento de fallos de arranque.

### N2 — Memoria e interrupciones robustas
- [x] Leer mapa de memoria y HHDM.
- [x] Asignador físico bootstrap de páginas de 4 KiB.
- [x] IDT básica y stubs de excepciones CPU.
- [ ] Añadir bitmap/tabla de propiedad para validar cada página liberada.
- [ ] Proteger el asignador ante concurrencia antes de habilitar SMP.
- [ ] GDT/TSS/IST propios y pilas de excepción seguras.
- [ ] Manejo de page faults y diagnóstico de direcciones inválidas.
- [ ] Gestor de memoria virtual y mapeos propios.
- [ ] Heap del kernel y pruebas de estrés del gestor de memoria.

### N3 — Ejecución
- [ ] Temporizador e interrupciones externas.
- [ ] Planificador y cambios de contexto.
- [ ] Procesos, hilos, aislamiento, syscalls e IPC.
- [ ] Entrada a modo usuario y retorno seguro al kernel.

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
