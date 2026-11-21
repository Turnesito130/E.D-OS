# E.D-OS — kernel propio x86-64

E.D-OS utiliza un kernel propio. Limine es el bootloader; Linux se estudia como referencia, no se copia como kernel de E.D-OS.

## Verificación confirmada

La ejecución completa de GitHub Actions en la rama `arquitectura/0.1` (commit `40163f25`) pasó las pruebas host, compiló la ISO híbrida y arrancó el kernel en QEMU BIOS y UEFI con OVMF. También arrancó en una máquina QEMU tipo PC con disco IDE y verificó lectura/escritura ATA PIO.

En BIOS y UEFI se verificó:
- PMM con registro de propiedad de páginas y heap slab.
- GDT/TSS propios, TR activo y pilas IST para doble fallo, NMI y page fault.
- Mapeo/desmapeo sobre el CR3 activo y ticks reales del PIT.
- Planificador preemptivo de kernel: dos tareas de carga y una tarea monitor; se comprobó que ambas tareas recibieron CPU y que el estado final del scheduler era válido.
- IPC con endpoints por tarea, recepción bloqueante, despertar y cambio de contexto.
- VFS montado sobre RAMFS y EDFS, además de formato/remontaje de EDFS.
- Enumeración PCI y decodificación de nueve BAR en QEMU.
- Page fault deliberado en una dirección desmapeada, con registro de CR2.

En la prueba ATA PIO se utilizó un disco virtual IDE de 16 MiB: lectura del sector 0, escritura y verificación de un sector de prueba, formato EDFS, escritura de un archivo mediante VFS, remontaje y lectura de vuelta.

## Memoria

- PMM de páginas físicas de 4 KiB a partir del mapa Limine y HHDM.
- Lista de páginas libres con validación de nodos e invariantes.
- Tabla hash estática de propiedad de 262.144 entradas (2 MiB de metadatos; límite teórico de 1 GiB de páginas simultáneamente asignadas). Rechaza páginas no asignadas y dobles liberaciones.
- Heap slab con clases de 16 a 4032 bytes, alineación, zero-on-alloc/free, validación de punteros y devolución de slabs vacíos al PMM.

**Límites:** no hay coalescencia general, asignaciones contiguas grandes, bloqueo SMP ni integración completa con buddy.

## CPU, interrupciones y planificación

- IDT de 256 entradas y stubs de excepciones.
- GDT propia con selectores de kernel y usuario; TSS de 64 bits con RSP0 y tres pilas IST (doble fallo, NMI y page fault).
- PIC 8259 remapeado y PIT a aproximadamente 100 Hz. IRQ0 alimenta el planificador preemptivo; IRQ1 recibe scancodes del teclado PS/2.
- Scheduler de hasta 32 tareas: estados READY/RUNNING/BLOCKED/EXITED, cambios cooperativos y preemptivos por IRQ0. Los marcos restauran registros generales y el frame completo de IRETQ en long mode.

**Límites:** la preempción solo funciona entre tareas de kernel ring 0. No hay ring 3, aislamiento por proceso, guardado FPU/SIMD, APIC/IOAPIC ni SMP.

## IPC y syscalls

- IPC: colas de 16 mensajes, payload de hasta 64 bytes y endpoints asociados a una tarea propietaria. El emisor debe ser la tarea actual; la recepción puede bloquear y el envío despierta al receptor.
- Dispatcher experimental con `WRITE`, `EXIT`, `YIELD` y `GETPID`. `YIELD` realiza un cambio real de contexto si las tareas están preparadas.

**Límites:** no existe todavía una entrada de syscall invocable desde ring 3, copia segura de memoria de usuario ni aislamiento entre procesos.

## Memoria virtual y fallos

- Tablas x86-64 de cuatro niveles, traducción, mapeo y desmapeo de páginas de 4 KiB.
- Prueba QEMU adopta el CR3 de Limine, mapea una dirección libre, verifica un alias físico, desmapea e invalida TLB con `invlpg`.
- Page fault controlado registra vector, error, RIP y CR2.

**Límites:** no se instala todavía un CR3 propio ni espacios de direcciones aislados. El page fault es diagnóstico; no hay recuperación ni demanda de páginas.

## Almacenamiento y sistemas de archivos

- API de dispositivos de bloques con comprobación de rangos, modo solo lectura y ramdisk.
- ATA PIO del canal primario, master/slave, IDENTIFY y LBA28, sectores de 512 bytes; lectura/escritura por sondeo con timeout. QEMU valida lectura, escritura y verificación.
- EDFS: sistema plano sobre bloques de 512 bytes, superblock versionado, directorio fijo de 16 entradas y un sector por archivo (máximo 512 bytes). Formato, montaje, creación, lectura, escritura, borrado, stat y readdir.
- RAMFS temporal de hasta 16 archivos de 256 bytes.
- VFS con backends RAMFS y EDFS: descriptores, rutas absolutas, creación/truncado, lectura/escritura por posición, seek, stat y readdir. QEMU prueba EDFS y VFS sobre un disco ATA de test tras remontar el volumen.

**Límites:** EDFS no tiene directorios jerárquicos, permisos, journal ni recuperación ante corte de energía. El disco ATA de test solo se formatea si el sector 1 contiene la firma explícita `EDOSATA1`; así se evita escribir en discos normales. ATA PIO no incluye DMA, LBA48, ATAPI, hotplug, AHCI/SATA ni NVMe.

## Drivers y buses

- COM1: UART 16550 por sondeo para diagnósticos.
- Teclado PS/2: IRQ1, cola circular de 64 scancodes y decodificación básica set 1 con Shift y Caps Lock.
- PCI legacy CF8/CFC: enumeración de buses, slots y funciones multifunción; decodifica BAR de E/S, memoria 32/64 bits y prefetchable. QEMU detecta seis dispositivos y nueve BAR.

**Límites:** no hay drivers de red, USB, audio, GPU, AHCI ni PCIe MSI/MSI-X. El teclado aún no está conectado a una shell y utiliza una distribución básica tipo US.

## Pendiente principal

Modo usuario ring 3 y transición segura, espacios de direcciones independientes, entrada real de syscalls, recuperación de page faults, APIC/IOAPIC, SMP, drivers DMA/AHCI/NVMe, red, directorios/permisos/journal, pruebas de estrés y hardware físico.

## Construcción

```sh
make -C kernel clean all
make -C kernel iso LIMINE_DIR=/ruta/a/limine
```

GitHub Actions ejecuta las pruebas unitarias, construye ELF e ISO híbrida y valida los arranques BIOS/UEFI y el disco ATA PIO en QEMU. Un resultado verde demuestra esas rutas concretas; no equivale a un sistema operativo de producción.
