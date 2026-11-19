# E.D-OS — arquitectura de RAM y memoria virtual (inspirada en Linux)

Estado: diseño objetivo; **no implica que estas funciones estén implementadas**.

## Objetivo

Mantener un kernel propio x86-64, independiente de Linux en tiempo de ejecución. Estudiar Linux para adoptar ideas probadas (Buddy, SLUB, gestión de páginas y espacios virtuales), pero escribir una implementación adaptada a E.D-OS. No copiar código fuente Linux sin fijar versión/commit, conservar avisos SPDX y revisar GPL-2.0-only y dependencias de cada archivo.

## Estado real de la rama arquitectura/0.1

El archivo kernel/src/memory.c ya obtiene el mapa de memoria Limine y el HHDM, suma las regiones utilizables y ofrece páginas físicas de 4 KiB puestas a cero. También mantiene una lista enlazada de páginas liberadas y detecta una doble liberación sencilla.

Esto **no es todavía un gestor completo**: no hay Buddy allocator, propiedad verificable por PFN, zonas, paginación propia, espacios virtuales por proceso, heap del kernel ni recuperación de page faults. El resultado QEMU debe verificarse en CI antes de considerar probado el hito.

## Diseño propuesto

### 1. Descubrimiento y reserva de RAM

- Convertir el mapa Limine en intervalos físicos validados, con comprobación de overflow y alineación.
- Reservar explícitamente kernel/ELF, módulos, estructuras del cargador, framebuffer, ACPI, MMIO y metadatos del propio gestor.
- No liberar memoria BOOTLOADER_RECLAIMABLE hasta copiar/consumir las estructuras de Limine que sigan siendo necesarias.
- Representar cada marco físico de 4 KiB mediante PFN (physical frame number) y metadatos: estado, zona, orden y referencias.
- Mantener contadores separados: RAM instalada detectada, RAM utilizable, reservada, libre y asignada. No confundir bytes utilizables con memoria libre.

### 2. Asignador físico Buddy

Adoptar el algoritmo Buddy de Linux, con implementación original para E.D-OS:

- Listas libres por orden; orden 0 = una página de 4 KiB, orden N = 2^N páginas contiguas.
- Separar zonas físicas según límites que realmente necesite el hardware (por ejemplo DMA, DMA32 y NORMAL); no inventar zonas si la plataforma no las necesita.
- Inicializar cada intervalo utilizable dividiéndolo en bloques alineados máximos que no crucen límites reservados.
- alloc_pages(order, flags): buscar el menor orden disponible, dividir bloques mayores y actualizar metadatos/contadores.
- free_pages(pfn, order): validar propiedad y orden, encontrar el buddy mediante XOR del PFN, fusionar mientras el buddy esté libre y tenga el mismo orden.
- Detectar doble liberación, órdenes inválidos, páginas reservadas y liberaciones fuera de rango.
- Añadir sincronización con spinlocks y secciones críticas seguras frente a interrupciones antes de permitir concurrencia/SMP.
- Política inicial conservadora: poner a cero páginas destinadas a procesos o tablas de páginas; no hacer el borrado completo dentro de una sección con interrupciones deshabilitadas.

### 3. Heap del kernel inspirado en SLUB

Construirlo encima del Buddy, no directamente encima del mapa Limine:

- kmalloc/kfree para objetos pequeños y medianos, con caches por tamaños.
- Páginas grandes asignadas mediante Buddy.
- Canarios/redzones y comprobaciones de estado en compilaciones de depuración.
- Alineación explícita y detección de overflow en tamaños.
- No habilitar el heap hasta que el asignador físico y sus pruebas estén estables.

### 4. Memoria virtual x86-64

Separar claramente memoria física y virtual:

- Implementar tablas de páginas de cuatro niveles (PML4, PDPT, PD, PT) para el modo x86-64 de cuatro niveles; detectar y tratar LA57 aparte si se soporta.
- Crear primitivas map_page, unmap_page, translate_page y switch_address_space.
- Validar canonicalidad, alineación, permisos y rangos; no permitir que páginas de usuario mapeen memoria del kernel.
- Mantener un mapa virtual del kernel y un espacio de direcciones por proceso.
- Invalidar TLB con invlpg para cambios locales y diseñar shootdown para SMP antes de usar varios núcleos.
- Usar page faults (#PF) para demanda de páginas y reportar fallos irrecuperables con dirección CR2 y bits de error.
- Introducir Copy-on-Write solo después de contar con referencias por página, permisos y page-fault handler correctos.

### 5. Espacios virtuales de procesos

Inspiración conceptual en mm_struct/VMA de Linux, con tipos y estructuras propios:

- Cada proceso mantiene regiones virtuales (código, datos, heap, stack, mmap).
- Las regiones describen inicio/fin, permisos y política de respaldo; no se asignan todas las páginas físicas de golpe.
- El fallo de página valida la región y materializa una página bajo demanda.
- fork/Copy-on-Write y memoria compartida quedan para una fase posterior.

### 6. Recuperación y presión de memoria

- Primero devolver páginas liberables a Buddy y vaciar caches.
- Mantener reservas mínimas para operaciones críticas del kernel.
- Implementar OOM handling solo cuando existan procesos, contabilidad por proceso y mecanismos seguros de terminación.
- Swap/pagefile es posterior: requiere almacenamiento, sistema de archivos, E/S asíncrona y política de reemplazo. No es requisito del primer kernel arrancable.

## Orden de implementación y criterios de aceptación

1. **PMM seguro**: mapa normalizado, reservas, bitmap/PFN metadata, contadores y pruebas de límites.
2. **Buddy**: asignar/liberar órdenes 0–N, dividir/fusionar, fragmentación y pruebas de doble liberación.
3. **Paginación**: mapear/desmapear/traducir, permisos, páginas no presentes y fallos #PF controlados.
4. **Heap**: caches, alineación, tamaños extremos, agotamiento y liberación.
5. **Espacios de proceso**: aislamiento kernel/usuario, cambio de CR3 y TLB.
6. **COW y demanda**: pruebas de aislamiento y referencias.
7. **SMP/OOM/swap**: solo después de locks, shootdown y E/S fiables.

Cada etapa debe tener pruebas unitarias del kernel y pruebas QEMU automatizadas. Un ELF compilado no demuestra que el kernel arranque; un arranque QEMU tampoco demuestra compatibilidad con hardware físico.

## Referencias Linux para estudiar

- mm/page_alloc.c y include/linux/mmzone.h: Buddy allocator y zonas.
- mm/slub.c: caches de objetos.
- mm/memory.c: gestión de fallos de página y mapeos.
- mm/vmalloc.c: espacio virtual del kernel.
- Documentation/mm/: documentación del subsistema de memoria.

Las rutas son referencias de estudio, no una afirmación de que E.D-OS ya incorpore esos archivos ni sus implementaciones.
