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
- IPC: colas circulares acotadas de 16 mensajes con payload máximo de 64 bytes, validación de tamaño y detección de cola llena/vacía. Todavía no hay aislamiento entre procesos, bloqueo, sincronización SMP ni llamadas desde modo usuario.
- Ambos módulos se compilan como parte del ELF y tienen pruebas unitarias host en CI.

## Pendiente

Propiedad/bitmap de páginas, paginación y page faults, GDT/TSS/IST, temporizador e IRQ, planificador, cambio de contexto, procesos/hilos, syscalls y modo usuario, drivers, sistema de archivos, red, SMP, pruebas de estrés y hardware físico.
