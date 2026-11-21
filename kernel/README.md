# E.D-OS — desarrollo del núcleo propio

E.D-OS tendrá un kernel propio e independiente. Linux se estudia como referencia; cualquier reutilización exige revisar procedencia, dependencias y licencia.

## Kernel 0.0.5 — memoria y dispositivos iniciales

- Entrada x86_64, pila inicial y arranque mediante Limine.
- Mapa de memoria Limine y HHDM.
- Asignador físico bootstrap de páginas de 4 KiB.
- Lista enlazada de páginas liberadas para reutilización, comprobación segura de nodos y validador de invariantes del PMM.
- Rechazo de direcciones desalineadas/no utilizables y doble liberación detectada.
- IDT de 256 entradas y stubs para excepciones.
- Pruebas QEMU superadas: asignar, liberar, rechazar doble liberación, reutilizar la misma página, comprobar que vuelve a cero y validar la lista libre.

## Límites conocidos

pmm_free_page exige que el llamador entregue una página obtenida previamente mediante pmm_alloc_page; todavía no existe un mapa de propiedad por página que pueda demostrar esa procedencia. No hay bloqueo para concurrencia, coalescencia, memoria virtual propia, gestor de bloques de tamaño variable ni recuperación de excepciones. Esto sigue siendo un asignador bootstrap, no uno de producción.

## Construcción y verificación

La ejecución completa confirmada en GitHub Actions (commit `5563c878`) pasó las pruebas host, compiló ELF e ISO y arrancó en QEMU BIOS. Se comprobaron por puerto serie PMM, tablas de páginas, disponibilidad PS/2 y el manejador de excepción. La prueba no activa CR3 ni demuestra ring 3 o multitarea real.

\`\`\`sh
make -C kernel clean all
make -C kernel iso LIMINE_DIR=/ruta/a/limine
\`\`\`

El workflow debe compilar y arrancar QEMU. El hito solo se considera verificado cuando el log serial muestra todos los marcadores de memoria y la excepción de breakpoint.

## Nuevos subsistemas en fase inicial (0.1)

- Planificador: tabla fija de 32 tareas, estados READY/RUNNING/BLOCKED/EXITED, creación, selección round-robin, transición de estado actual y validación de invariantes. No guarda/restaura registros ni cambia pilas; aún no es multitarea ejecutable.
- IPC: colas circulares acotadas de 16 mensajes con payload máximo de 64 bytes, validación de tamaño, índices y contador internos, y detección de cola llena/vacía/corrupta. Todavía no hay aislamiento entre procesos, bloqueo, sincronización SMP ni llamadas desde modo usuario.
- RAMFS: sistema de archivos temporal en memoria con hasta 16 archivos de 256 bytes, creación, lectura, escritura completa y borrado; rechaza nombres con separadores de ruta y límites excedidos. No es persistente, no usa VFS, no tiene directorios, permisos, caché ni integración con dispositivos de bloques.
- Los módulos scheduler, IPC, RAMFS, buddy y syscalls se compilan como parte del ELF y tienen pruebas unitarias host en CI.

## Pendiente

Propiedad/bitmap de páginas, activación de CR3 y page faults, GDT/TSS/IST, temporizador e IRQ, cambio de contexto real, procesos/hilos, entrada de syscalls desde ring 3, drivers de almacenamiento/red, VFS y sistema de archivos persistente, SMP, pruebas de estrés y hardware físico.


## Interrupciones y temporizador PIT

- PIC 8259 remapeado a los vectores 32–47; por ahora solo se habilita IRQ0 y el resto permanece enmascarado.
- PIT canal 0 configurado aproximadamente a 100 Hz; stub de IRQ0 guarda/restaura registros generales, despacha el tick y envía EOI al PIC.
- La prueba QEMU espera tres ticks reales antes de continuar. No se habilita IRQ1 todavía.

**Límites:** no hay APIC/IOAPIC, temporizador HPET, prioridades, IRQ de teclado ni integración del tick con cambio de contexto del scheduler. El handler no realiza todavía preempción.

## Drivers básicos: COM1 y teclado PS/2

- COM1: detección por registro scratch, inicialización UART y escritura con espera acotada; usado para logs de arranque.
- Teclado PS/2: lectura por sondeo del puerto 0x60/0x64 y traducción básica de scancodes set 1; incluye Shift, Caps Lock, letras, números y teclas comunes. Su decodificador se prueba en host y QEMU comprueba la disponibilidad del controlador.

**Límites:** el teclado todavía no usa IRQ, no hay cola de eventos de entrada ni distribución a aplicaciones; la tabla de caracteres es básica y no cubre todos los teclados/idiomas.

## Paginación x86-64 (fase inicial)

- Construcción de tablas de cuatro niveles (PML4, PDPT, PD y PT) con páginas asignadas por un callback.
- Mapeo, traducción y desmapeo de páginas de 4 KiB; comprueba canonicalidad de direcciones, alineación, flags admitidos, entradas duplicadas y páginas enormes no soportadas.
- Pruebas host para direcciones altas, offsets, flags, errores, adopción y desmapeo. El arranque QEMU crea una raíz de pruebas y, además, adopta el CR3 activo de Limine: busca un slot PML4 libre, mapea una página física en una dirección virtual nueva, verifica la escritura/lectura del alias, la desmapea y ejecuta `invlpg`.

**Límites:** todavía no se cambia a una raíz CR3 propia ni se crean espacios de direcciones aislados. `invlpg` se ejecuta explícitamente en la prueba, pero no hay invalidación automática ni TLB shootdown SMP, páginas enormes, copy-on-write, protección real de espacios de usuario ni recuperación de fallos de página.

## Driver serie y ABI de syscalls (fase experimental)

- Driver COM1 separado en `serial.c`: comprobación del registro scratch para detectar la UART, configuración 16550, transmisión con espera acotada y salida de cadenas. El kernel lo usa para sus diagnósticos de arranque.
- Dispatcher de syscalls con números estables para `WRITE`, `EXIT`, `YIELD` y `GETPID`. Valida el contexto y, para `WRITE`, limita el tamaño a 4096 bytes y comprueba el rango virtual declarado antes de invocar el backend.
- Pruebas host para resultados, límites, punteros fuera de rango, callbacks y números desconocidos.

**Límites importantes:** el planificador aún no guarda/restaura registros ni pilas y no produce multitarea real. Todavía no existe transición a ring 3, TSS/espacio de usuario, instrucción `syscall/sysret`, copia segura de memoria entre privilegios ni conexión del dispatcher a una entrada de interrupción. El dispatcher conecta `YIELD` y `EXIT` con el planificador y `GETPID` con la tarea actual; su integración está probada en host. Sigue siendo una capa ABI invocada por el kernel de pruebas, no un mecanismo de llamadas desde programas de usuario. El driver COM1 no tiene recepción IRQ ni gestión de interrupciones serie.

## Verificación

El workflow ejecuta pruebas unitarias de buddy, scheduler, IPC, RAMFS y dispatcher de syscalls, compila el ELF, genera la ISO y comprueba el arranque BIOS en QEMU mediante el registro serie. Un resultado verde demuestra esas rutas concretas; no demuestra multitarea real, aislamiento de procesos, persistencia ni compatibilidad con hardware físico.
