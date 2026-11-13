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

## Pendiente

Propiedad/bitmap de páginas, paginación y page faults, GDT/TSS/IST, temporizador e IRQ, planificador, cambio de contexto, procesos/hilos, syscalls y modo usuario, drivers, sistema de archivos, red, SMP, pruebas de estrés y hardware físico.
