# E.D-OS — desarrollo del núcleo propio

E.D-OS tendrá un **kernel propio e independiente**. No utilizará el kernel Linux para arrancar ni para funcionar. Linux se estudia como referencia de ingeniería; cualquier reutilización de código requiere revisar procedencia, dependencias y licencia.

## Kernel 0.0.3 — descubrimiento de memoria y asignación inicial

Código propio:
- `src/entry.S`: entrada x86_64 y pila inicial de 16 KiB.
- `src/kernel.c`: comprobación del protocolo, diagnóstico serie y prueba real de asignación de una página.
- `src/memory.c`: solicitudes Limine para mapa de memoria y HHDM; validación básica de respuestas; suma de RAM utilizable; asignador físico bootstrap de páginas de 4 KiB.
- `include/memory.h`: interfaz del subsistema de memoria.
- `linker.ld`: ELF64 higher-half.
- `scripts/build-iso.sh`: imagen híbrida BIOS/UEFI con Limine externo.

El asignador actual es deliberadamente **monotónico**: entrega páginas utilizables, las pone a cero y no las libera ni reutiliza. Evita los primeros 1 MiB y no toca regiones reservadas, ACPI, framebuffer, kernel/módulos ni memoria recuperable del cargador. Es una base de arranque, no un gestor de memoria completo.

## Cargador y protocolo

Limine v12.9.6 se utiliza solo como cargador externo/protocolo. El kernel solicita revisión base 3, mapa de memoria y HHDM; no presupone un offset HHDM fijo. Referencia: https://github.com/limine-bootloader/limine-protocol/blob/trunk/PROTOCOL.md

## Construcción y prueba

```sh
make -C kernel clean all
make -C kernel iso LIMINE_DIR=/ruta/a/limine
```

GitHub Actions compila la imagen y debe arrancarla en QEMU para comprobar `KERNEL_MEMORY_MILESTONE_OK`. Hasta que exista un resultado verde y un log serial de esa ejecución, el arranque no se considera verificado.

## Límites actuales

Todavía faltan: liberación/coalescencia de páginas, bitmap o buddy allocator, protección de memoria, GDT/IDT y excepciones, paginación propia, interrupciones, planificador, procesos, syscalls, drivers, sistema de archivos, red y soporte ARM64/móvil. La prueba QEMU no demuestra compatibilidad con hardware físico.
