# E.D-OS — desarrollo del núcleo propio

E.D-OS tendrá un kernel propio e independiente. Linux se estudia como referencia; cualquier reutilización exige revisar procedencia, dependencias y licencia.

## Kernel 0.0.4 — liberar y reutilizar páginas

- Entrada x86_64, pila inicial y arranque mediante Limine.
- Mapa de memoria Limine y HHDM.
- Asignador físico bootstrap de páginas de 4 KiB.
- Lista enlazada de páginas liberadas para reutilización.
- Rechazo de direcciones desalineadas/no utilizables y doble liberación detectada.
- IDT de 256 entradas y stubs para excepciones.
- Pruebas QEMU previstas: asignar, liberar, rechazar doble liberación, reutilizar la misma página y comprobar que vuelve a cero.

## Límites conocidos

pmm_free_page exige que el llamador entregue una página obtenida previamente mediante pmm_alloc_page; todavía no existe un mapa de propiedad por página que pueda demostrar esa procedencia. No hay bloqueo para concurrencia, coalescencia, memoria virtual propia, gestor de bloques de tamaño variable ni recuperación de excepciones. Esto sigue siendo un asignador bootstrap, no uno de producción.

## Construcción y verificación

\`\`\`sh
make -C kernel clean all
make -C kernel iso LIMINE_DIR=/ruta/a/limine
\`\`\`

El workflow debe compilar y arrancar QEMU. El hito solo se considera verificado cuando el log serial muestra todos los marcadores de memoria y la excepción de breakpoint.

## Nuevos subsistemas en fase inicial (0.1)

- Planificador cooperativo: tabla fija de 32 tareas, estados READY/RUNNING/BLOCKED/EXITED, creación y selección round-robin. Todavía no realiza cambio de contexto, no crea pilas ni cambia privilegios; es una base de planificación, no multitarea ejecutable.
- IPC: colas circulares acotadas de 16 mensajes con payload máximo de 64 bytes, validación de tamaño, índices y contador internos, y detección de cola llena/vacía/corrupta. Todavía no hay aislamiento entre procesos, bloqueo, sincronización SMP ni llamadas desde modo usuario.
- RAMFS: sistema de archivos temporal en memoria con hasta 16 archivos de 256 bytes, creación, lectura, escritura completa y borrado; rechaza nombres con separadores de ruta y límites excedidos. No es persistente, no usa VFS, no tiene directorios, permisos, caché ni integración con dispositivos de bloques.
- Ambos módulos se compilan como parte del ELF y tienen pruebas unitarias host en CI.

## Pendiente

Propiedad/bitmap de páginas, paginación y page faults, GDT/TSS/IST, temporizador e IRQ, planificador, cambio de contexto, procesos/hilos, syscalls y modo usuario, drivers, sistema de archivos, red, SMP, pruebas de estrés y hardware físico.


## Driver serie y ABI de syscalls (fase experimental)

- Driver COM1 separado en `serial.c`: configuración UART 16550, transmisión con espera acotada y salida de cadenas. El kernel lo usa para sus diagnósticos de arranque.
- Dispatcher de syscalls con números estables para `WRITE`, `EXIT`, `YIELD` y `GETPID`. La llamada `WRITE` limita el tamaño a 4096 bytes y comprueba el rango virtual declarado antes de invocar el backend.
- Pruebas host para resultados, límites, punteros fuera de rango, callbacks y números desconocidos.

**Límites importantes:** todavía no existe transición a ring 3, TSS/espacio de usuario, instrucción `syscall/sysret`, copia segura de memoria entre privilegios ni conexión del dispatcher a una entrada de interrupción. El dispatcher es una capa ABI probada aisladamente, no un mecanismo de llamadas desde programas de usuario. El driver COM1 no tiene detección fiable de hardware ausente ni recepción IRQ.

## Verificación

El workflow ejecuta pruebas unitarias de buddy, scheduler, IPC, RAMFS y dispatcher de syscalls, compila el ELF, genera la ISO y comprueba el arranque BIOS en QEMU mediante el registro serie. Un resultado verde demuestra esas rutas concretas; no demuestra multitarea real, aislamiento de procesos, persistencia ni compatibilidad con hardware físico.
