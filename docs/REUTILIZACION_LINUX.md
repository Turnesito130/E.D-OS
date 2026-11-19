# Auditoría de reutilización del kernel Linux para E.D-OS

Fecha de revisión: 2026-11-19

## Decisión de arquitectura que se conserva

E.D-OS sigue teniendo un kernel propio x86_64 y no depende del kernel Linux para arrancar. Linux se usa como referencia y como posible origen de componentes seleccionados. Esta auditoría no cambia esa decisión.

## Fuentes upstream inspeccionadas

Repositorio: https://github.com/torvalds/linux

- https://github.com/torvalds/linux/blob/master/mm/page_alloc.c
- https://github.com/torvalds/linux/blob/master/include/linux/bitmap.h
- https://github.com/torvalds/linux/blob/master/lib/bitmap.c
- https://github.com/torvalds/linux/blob/master/COPYING

El árbol upstream contiene decenas de miles de rutas. La inspección de un árbol o de un archivo no equivale a una auditoría completa de todo Linux.

## Hallazgos

### 1. Asignador de páginas físicas: no copiar mm/page_alloc.c directamente

El archivo upstream mm/page_alloc.c es grande (más de 230 KB en la revisión consultada) y depende de numerosas interfaces internas: zonas NUMA, nodos, folios, listas por CPU, interrupciones, locks, contabilidad, trazas, migración, compaction y otros subsistemas Linux.

El PMM actual de E.D-OS es un asignador bootstrap pequeño que usa el mapa de memoria de Limine y HHDM. Pegar page_alloc.c sin portar todo ese entorno no compilaría ni sería seguro.

Uso recomendado: estudiar sus invariantes y diseño por etapas; no importar el archivo entero.

### 2. Bitmaps: candidato conceptual para el siguiente hito

Linux proporciona operaciones de bitmap en include/linux/bitmap.h y lib/bitmap.c. La implementación genérica depende de tipos, macros y cabeceras internas de Linux; no es un módulo autónomo listo para pegar en E.D-OS.

Uso recomendado: implementar en E.D-OS un bitmap pequeño y freestanding, con interfaz propia y pruebas, para registrar el estado/propiedad de cada página física. Si en el futuro se copia código literal, registrar el archivo de origen, SPDX, copyright y dependencias.

### 3. Otros subsistemas

Planificar una evaluación separada antes de incorporar código de:
- planificador y cambios de contexto (kernel/sched);
- tablas de páginas y memoria virtual (arch/x86/mm y mm);
- sistemas de archivos (fs);
- red (net);
- controladores (drivers).

Estos subsistemas están integrados con APIs, tipos, configuración y mecanismos internos de Linux. No son bloques intercambiables sin un trabajo de portabilidad considerable.

## Licencia y cumplimiento

El archivo COPYING upstream indica GPL-2.0-only con la excepción Linux syscall note; además, pueden aplicarse otras licencias y excepciones por archivo. La licencia exacta debe comprobarse mediante el identificador SPDX y los avisos de cada archivo, junto con sus dependencias.

Antes de distribuir cualquier componente copiado o derivado:
1. conservar avisos de copyright y SPDX;
2. cumplir las condiciones de la licencia aplicable;
3. mantener un registro de origen y modificaciones;
4. proporcionar el código fuente correspondiente y los avisos exigidos cuando corresponda;
5. revisar las obligaciones de todos los archivos incorporados, no asumir que todo el repositorio tiene idéntica licencia.

## Estado tras esta auditoría

- [x] Inspeccionado el árbol del repositorio upstream y los archivos de memoria/bitmap seleccionados.
- [x] Confirmado que page_alloc.c no es un bloque independiente que pueda pegarse en el PMM actual.
- [x] Identificado bitmap como patrón útil para reforzar la propiedad de páginas.
- [ ] No se ha copiado código fuente de Linux en esta fase.
- [ ] No se ha modificado el kernel ni se ha ejecutado una compilación nueva.
- [ ] Próximo paso propuesto: diseñar un bitmap PMM freestanding propio, añadir pruebas de asignación/liberación inválida y doble liberación, y verificar build/arranque QEMU antes de dar el hito por terminado.

## Estado del kernel E.D-OS revisado

La rama revisada contiene el kernel experimental 0.0.4: entrada x86_64, arranque mediante Limine, asignador físico bootstrap de páginas de 4 KiB e IDT básica. El propio README del kernel reconoce que todavía no existe una tabla de propiedad por página. El workflow automatiza pruebas QEMU, pero esta auditoría no ha confirmado un log reciente satisfactorio de QEMU.
