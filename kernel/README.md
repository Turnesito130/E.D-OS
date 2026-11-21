# E.D-OS — desarrollo del núcleo propio

E.D-OS tendrá un kernel propio e independiente. Linux se estudia como referencia; cualquier reutilización exige revisar procedencia, dependencias y licencia.

## Kernel 0.0.5 — memoria y dispositivos iniciales

- Entrada x86_64, pila inicial y arranque mediante Limine.
- Mapa de memoria Limine y HHDM.
- Asignador físico bootstrap de páginas de 4 KiB.
- Lista enlazada de páginas liberadas para reutilización, comprobación segura de nodos y validador de invariantes del PMM.
- Rechazo de direcciones desalineadas/no utilizables y doble liberación detectada; tabla hash de propiedad que registra las páginas asignadas y evita liberar páginas que no pertenecen al PMM.
- IDT de 256 entradas y stubs para excepciones.
- Pruebas QEMU superadas: asignar, liberar, rechazar doble liberación, reutilizar la misma página, comprobar que vuelve a cero y validar la lista libre. El handler de page fault identifica el vector 14 y registra CR2; una prueba de arranque provoca un fallo controlado en una dirección virtual desmapeada.

## Límites conocidos

El registro de propiedad es una tabla hash estática de 65.536 entradas (límite actual de páginas simultáneamente asignadas); no hay bloqueo SMP, coalescencia ni integración completa con el buddy allocator. Esto sigue siendo un asignador bootstrap, no uno de producción.

## Construcción y verificación

La ejecución completa confirmada en GitHub Actions (commit `0326c71e`) pasó pruebas host, compiló la ISO híbrida y arrancó en QEMU BIOS y UEFI con OVMF. En ambos firmwares se comprobaron propiedad de páginas PMM, GDT/TSS con TR activo, pilas IST, mapeo sobre CR3 activo, ticks PIT, cambio cooperativo entre dos tareas, VFS sobre RAMFS y EDFS, ramdisk, remontaje EDFS y page fault controlado con lectura de CR2. No demuestra ring 3, multitarea preemptiva ni aislamiento de procesos.

\`\`\`sh
make -C kernel clean all
make -C kernel iso LIMINE_DIR=/ruta/a/limine
\`\`\`

El workflow compila ELF e ISO híbrida y arranca en QEMU BIOS y UEFI. El hito solo se considera verificado cuando ambos registros serie contienen los marcadores de todos los subsistemas y el page fault controlado.

## Nuevos subsistemas en fase inicial (0.1)

- Planificador cooperativo: tabla fija de 32 tareas, estados READY/RUNNING/BLOCKED/EXITED, preparación de pilas y contextos, cambio real de pila/registros entre tareas, transición de estados y validación de invariantes. La prueba host alterna dos tareas y el arranque QEMU repite el cambio entre dos tareas sobre pilas estáticas dedicadas. El tick PIT sigue siendo independiente y no realiza preempción.
- IPC: colas circulares acotadas de 16 mensajes con payload máximo de 64 bytes, validación de tamaño, índices y contador internos, y detección de cola llena/vacía/corrupta. Todavía no hay aislamiento entre procesos, bloqueo, sincronización SMP ni llamadas desde modo usuario.
- RAMFS: sistema de archivos temporal en memoria con hasta 16 archivos de 256 bytes, creación, lectura, escritura completa y borrado; rechaza nombres con separadores de ruta y límites excedidos. No es persistente y no tiene directorios jerárquicos, permisos, caché ni integración con dispositivos de bloques.
- VFS: capa de sistema de archivos virtual montada sobre RAMFS, con rutas absolutas de raíz, descriptores limitados, apertura/creación/truncado, lectura/escritura por posición, seek, stat y readdir. Rechaza rutas relativas, traversal (`..`), subdirectorios aún no soportados y operaciones incompatibles con los permisos del descriptor. Tiene pruebas host y prueba de arranque en QEMU.
- Dispositivos de bloques: API común de lectura/escritura por LBA, validación de rangos y modo de solo lectura, más un ramdisk sectorial para pruebas. Se verifica lectura/escritura de sectores durante el arranque QEMU; todavía no es un driver SATA/AHCI/NVMe.
- EDFS: sistema de archivos plano propio sobre bloques de 512 bytes, con superblock, directorio fijo de 16 entradas y un sector de datos por archivo (hasta 512 bytes). Formato, montaje, creación, lectura, escritura y borrado; las pruebas remontan el volumen y comprueban que el contenido sigue ahí. En QEMU se prueba sobre ramdisk, por lo que no sobrevive al apagado de la máquina.
- Los módulos scheduler, IPC, RAMFS, VFS, buddy y syscalls se compilan como parte del ELF y tienen pruebas unitarias host en CI.

## Pendiente

raíz CR3 propia y recuperación de page faults, APIC/IOAPIC, planificador preemptivo y tarea idle, gestión de procesos/hilos, entrada de syscalls desde ring 3, drivers físicos SATA/AHCI/NVMe y red, driver físico de almacenamiento no volátil, journal, directorios y permisos, SMP, pruebas de estrés y hardware físico.


## GDT, TSS e IST x86-64

- GDT propia con selectores de kernel y usuario, descriptor TSS de 64 bits y registro TR verificado.
- TSS configura RSP0 y tres pilas IST independientes para doble fallo, NMI y page fault. La IDT conecta los vectores 8, 2 y 14 a sus IST.

**Límites:** no se ha ejecutado código en ring 3, no existe todavía cambio de privilegio ni aislamiento de direcciones de usuario.

## Interrupciones y temporizador PIT

- PIC 8259 remapeado a los vectores 32–47; se habilita IRQ0 y, si se detecta controlador PS/2, IRQ1. Las demás líneas permanecen enmascaradas.
- PIT canal 0 configurado aproximadamente a 100 Hz; stub de IRQ0 guarda/restaura registros generales, despacha el tick y envía EOI al PIC.
- La prueba QEMU espera tres ticks reales antes de continuar. IRQ1 se habilita únicamente si se detecta el controlador PS/2.

**Límites:** no hay APIC/IOAPIC, temporizador HPET, prioridades ni integración del tick con cambio de contexto del scheduler. El handler no realiza todavía preempción.

## Drivers básicos: COM1 y teclado PS/2

- COM1: detección por registro scratch, inicialización UART y escritura con espera acotada; usado para logs de arranque.
- Teclado PS/2: IRQ1 lee scancodes set 1 y los deposita en una cola circular de 64 posiciones; `edos_keyboard_poll_char` consume la cola y traduce Shift, Caps Lock, letras, números y teclas comunes. Se prueban decodificador, modificadores y desbordamiento de cola en host; QEMU comprueba la disponibilidad del controlador.

**Límites:** el teclado todavía no se integra con una shell o aplicaciones; la tabla de caracteres es básica, usa disposición tipo US y no cubre todos los teclados/idiomas ni repetición avanzada.

## Paginación x86-64 (fase inicial)

- Construcción de tablas de cuatro niveles (PML4, PDPT, PD y PT) con páginas asignadas por un callback.
- Mapeo, traducción y desmapeo de páginas de 4 KiB; comprueba canonicalidad de direcciones, alineación, flags admitidos, entradas duplicadas y páginas enormes no soportadas.
- Pruebas host para direcciones altas, offsets, flags, errores, adopción y desmapeo. El arranque QEMU crea una raíz de pruebas y, además, adopta el CR3 activo de Limine: busca un slot PML4 libre, mapea una página física en una dirección virtual nueva, verifica la escritura/lectura del alias, la desmapea y ejecuta `invlpg`.

**Límites:** todavía no se cambia a una raíz CR3 propia ni se crean espacios de direcciones aislados. `invlpg` se ejecuta explícitamente en la prueba, pero no hay invalidación automática ni TLB shootdown SMP, páginas enormes, copy-on-write, protección real de espacios de usuario ni recuperación de fallos de página.

## Driver serie y ABI de syscalls (fase experimental)

- Driver COM1 separado en `serial.c`: comprobación del registro scratch para detectar la UART, configuración 16550, transmisión con espera acotada y salida de cadenas. El kernel lo usa para sus diagnósticos de arranque.
- Dispatcher de syscalls con números estables para `WRITE`, `EXIT`, `YIELD` y `GETPID`. Valida el contexto y, para `WRITE`, limita el tamaño a 4096 bytes y comprueba el rango virtual declarado antes de invocar el backend.
- Pruebas host para resultados, límites, punteros fuera de rango, callbacks y números desconocidos.

**Límites importantes:** el planificador ya guarda/restaura registros callee-saved y pilas en cambios cooperativos de contexto del kernel, pero todavía no hay preempción ni ejecución en ring 3. Todavía no existe transición a ring 3, TSS/espacio de usuario, instrucción `syscall/sysret`, copia segura de memoria entre privilegios ni conexión del dispatcher a una entrada de interrupción. El dispatcher conecta `YIELD` y `EXIT` con el planificador y `GETPID` con la tarea actual. Cuando ambas tareas tienen contexto preparado, `YIELD` ejecuta el cambio cooperativo real de pila; se prueba con dos tareas en host. Si no hay contextos preparados, conserva el modo de transición de estados para pruebas de la ABI. Sigue siendo una capa ABI invocada por el kernel de pruebas, no un mecanismo de llamadas desde programas de usuario. El driver COM1 no tiene recepción IRQ ni gestión de interrupciones serie.

## Verificación

El workflow ejecuta pruebas unitarias de teclado, paginación, buddy, cambio de contexto, scheduler, IPC, syscalls, RAMFS y VFS, compila el ELF, genera la ISO y comprueba el arranque BIOS y UEFI en QEMU mediante el registro serie, incluido un page fault deliberado y su dirección CR2. Un resultado verde demuestra esas rutas concretas; demuestra un cambio cooperativo real entre dos contextos de kernel, pero no multitarea preemptiva, modo usuario, aislamiento de procesos, persistencia ni compatibilidad con hardware físico.
